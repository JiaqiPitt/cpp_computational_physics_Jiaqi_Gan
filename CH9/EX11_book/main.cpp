// Exercise 9.11: the primary rainbow, on all the cores of the machine.
// Usage: ./build/rainbow [photons per timing run] [photons for the plots]
#include "Photoreceptor.h"
#include "Rainbow.h"
#include "QatGenericFunctions/InterpolatingFunction.h"
#include "QatGenericFunctions/Variable.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotHist1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotProfile.h"
#include "QatPlotting/PlotStream.h"
#include <QApplication>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QStyle>
#include <QStyleHints>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <numeric>
#include <string>
#include <thread>

using namespace Genfun;

const char* BAND_NAME[Tally::BANDS]   = {"400-450 nm", "450-500 nm", "500-550 nm", "550-600 nm", "600-650 nm", "650-700 nm"};
const char* BAND_COLOUR[Tally::BANDS] = {"darkviolet", "blue", "darkcyan", "green", "darkorange", "red"};

// The data of Chapter 4: water from Table 4.3 (Hale 1973, Exercise 7) and
// the cone curves digitized in Exercise 8.  Each cone curve is divided by its
// average over 400-700 nm, so a flat spectrum (white light) gives equal red,
// green and blue.
Tables makeTables() {
    std::vector<double> lambda = {200, 250, 300, 350, 400, 450, 500, 550, 600,
                                  650, 700, 750, 800, 850, 900, 950, 1000};
    std::vector<double> index = {1.396, 1.362, 1.349, 1.343, 1.339, 1.337, 1.335, 1.333, 1.332,
                                 1.331, 1.331, 1.330, 1.329, 1.329, 1.328, 1.327, 1.327};
    InterpolatingFunction water(CUBIC_SPLINE);
    for (size_t i = 0; i < lambda.size(); ++i) water.addPoint(lambda[i], index[i]);
    Photoreceptor red(CURVE_DIR "curve_red.csv"), green(CURVE_DIR "curve_green.csv"), blue(CURVE_DIR "curve_blue.csv");

    Tables t;
    int points = int(std::lround((Tables::LMAX - Tables::LMIN) / Tables::STEP)) + 1;
    for (int i = 0; i < points; ++i) {
        double l = Tables::LMIN + i * Tables::STEP;
        t.n.push_back(water(l));
        t.red.push_back(red(l));
        t.green.push_back(green(l));
        t.blue.push_back(blue(l));
    }
    for (std::vector<double>* v : {&t.red, &t.green, &t.blue}) {
        double mean = std::accumulate(v->begin(), v->end(), 0.0) / double(v->size());
        for (double& x : *v) x /= mean;
    }
    return t;
}

// Descartes: the largest scattering angle (deg) for index of refraction n,
// reached at cos(theta_i) = sqrt((n^2 - 1) / 3)
double rainbowAngle(double n) {
    double ti = std::acos(std::sqrt((n * n - 1) / 3)), tr = std::asin(std::sin(ti) / n);
    return (4 * tr - 2 * ti) * 180.0 / M_PI;
}

// Part (a): the scattering angle for each band of wavelengths
void plotAngles(const Tally& tally, bool zoom, const std::string& file) {
    // in percent of all photons per degree, so runs of any size compare
    std::vector<Hist1D> h = zoom ? tally.narrow : tally.wide;
    double top = 0;
    for (Hist1D& x : h) {
        x *= 100.0 / (double(tally.photons) * x.binWidth());
        top = std::max(top, x.maxContents());
    }
    PRectF rect;
    rect.setXmin(h[0].min());
    rect.setXmax(h[0].max());
    rect.setYmin(0.0);
    rect.setYmax(1.1 * top);
    PlotView view(rect);

    std::vector<PlotHist1D> plots;
    plots.reserve(Tally::BANDS);
    PlotKey key(zoom ? 39.65 : 2.0, 1.0 * top);
    for (int k = 0; k < Tally::BANDS; ++k) {
        plots.emplace_back(h[k]);
        PlotHist1D::Properties prop;
        prop.pen.setColor(BAND_COLOUR[k]);
        prop.pen.setWidth(2);
        plots.back().setProperties(prop);
    }
    for (int k = 0; k < Tally::BANDS; ++k) {
        view.add(&plots[k]);
        key.add(&plots[k], BAND_NAME[k]);
    }
    view.add(&key);

    char title[200];
    std::snprintf(title, sizeof title, zoom ? "Near the peak, %.0f million photons" : "Primary rainbow, %.0f million photons", tally.photons / 1e6);
    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                << PlotStream::Size(16) << title << PlotStream::EndP();
    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                 << PlotStream::Size(14) << "angle between incident and scattered beam (degrees)" << PlotStream::EndP();
    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                 << PlotStream::Size(14) << "% of photons per degree" << PlotStream::EndP();
    view.save(file);
}

// Part (b): the sky, coloured by the summed cone responses
void drawSky(const Tally& tally, const std::string& file) {
    // brightness: the brightest 0.5% of the lit pixels (on the bow) saturate
    std::vector<float> level;
    for (size_t i = 0; i < tally.rgb.size(); i += 3) {
        float m = std::max({tally.rgb[i], tally.rgb[i + 1], tally.rgb[i + 2]});
        if (m > 0) level.push_back(m);
    }
    size_t k = size_t(0.995 * double(level.size()));
    std::nth_element(level.begin(), level.begin() + k, level.end());
    double scale = level[k];

    QImage image(Sky::W, Sky::H, QImage::Format_RGB32);
    for (int y = 0; y < Sky::H; ++y)
        for (int x = 0; x < Sky::W; ++x) {
            const float* c = &tally.rgb[3 * (size_t(y) * Sky::W + x)];
            int v[3];
            for (int j = 0; j < 3; ++j) v[j] = int(255 * std::min(1.0, c[j] / scale) + 0.5);
            image.setPixel(x, y, qRgb(v[0], v[1], v[2]));
        }

    QPainter p(&image);
    p.setPen(QColor(200, 200, 200));
    p.setFont(QFont("Arial", 14));
    p.drawLine(Sky::X0 - 8, Sky::Y0, Sky::X0 + 8, Sky::Y0);
    p.drawLine(Sky::X0, Sky::Y0 - 8, Sky::X0, Sky::Y0 + 8);
    p.drawText(Sky::X0 + 12, Sky::Y0 + 20, "antisolar point");
    int bar = int(10 / Sky::DEG_PER_PIXEL);   // 10 degrees
    p.drawLine(30, Sky::H - 30, 30 + bar, Sky::H - 30);
    p.drawText(30, Sky::H - 38, "10 degrees");
    char caption[200];
    std::snprintf(caption, sizeof caption, "primary rainbow, %.0f million photons", tally.photons / 1e6);
    p.drawText(20, 30, caption);
    p.end();
    image.save(QString::fromStdString(file));
}

// Part (b), closer: the colour across the bow at each angle, all azimuths
// together.  Top: as it looks; bottom: scaled to the strongest channel at
// each angle, so only the hue is left (as in Exercise 4.9).
void drawBand(const Tally& tally, const std::string& file) {
    const int bins = int(tally.profile.size() / 3), PX = 2;   // 2 pixels per bin
    const int BW = PX * bins, BH = 100, M = 60, top1 = 70, top2 = top1 + BH + 70;
    double brightest = *std::max_element(tally.profile.begin(), tally.profile.end());
    QImage image(BW + 2 * M, top2 + BH + 70, QImage::Format_RGB32);
    image.fill(Qt::white);
    for (int i = 0; i < bins; ++i) {
        const double* c = &tally.profile[3 * size_t(i)];
        double m = std::max({c[0], c[1], c[2]});
        QRgb raw = qRgb(int(255 * c[0] / brightest + 0.5), int(255 * c[1] / brightest + 0.5), int(255 * c[2] / brightest + 0.5));
        QRgb hue = m > 0 ? qRgb(int(255 * c[0] / m + 0.5), int(255 * c[1] / m + 0.5), int(255 * c[2] / m + 0.5)) : qRgb(0, 0, 0);
        for (int dx = 0; dx < PX; ++dx)
            for (int y = 0; y < BH; ++y) {
                image.setPixel(M + PX * i + dx, top1 + y, raw);
                image.setPixel(M + PX * i + dx, top2 + y, hue);
            }
    }
    QPainter p(&image);
    p.setPen(Qt::black);
    p.setFont(QFont("Arial", 16));
    p.drawText(QRect(0, 10, image.width(), 40), Qt::AlignCenter, "Colour across the primary bow");
    p.setFont(QFont("Arial", 12));
    p.drawText(QRect(M, top1 - 25, BW, 20), Qt::AlignLeft, "as it looks (brightness and colour)");
    p.drawText(QRect(M, top2 - 25, BW, 20), Qt::AlignLeft, "scaled to the strongest channel at each angle (colour only)");
    p.drawRect(M - 1, top1 - 1, BW + 1, BH + 1);
    p.drawRect(M - 1, top2 - 1, BW + 1, BH + 1);
    for (int a = 0; a <= 12; ++a) {   // ticks every 0.5 degree
        double angle = Tally::PMIN + 0.5 * a;
        int x = M + int((angle - Tally::PMIN) / Tally::PSTEP * PX + 0.5);
        int tick = (a % 2 == 0) ? 10 : 5;
        for (int yb : {top1 + BH, top2 + BH}) {
            p.drawLine(x, yb, x, yb + tick);
            if (a % 2 == 0) p.drawText(QRect(x - 30, yb + 12, 60, 20), Qt::AlignCenter, QString::number(angle));
        }
    }
    p.drawText(QRect(0, image.height() - 28, image.width(), 20), Qt::AlignCenter, "angle from the antisolar point (degrees)");
    p.end();
    image.save(QString::fromStdString(file));
}

// Part (c): the photons are split evenly over nThreads threads started with
// std::async.  Each thread runs simulate() with its own seed and fills its
// own Tally, so the threads share nothing but the read-only tables and need
// no locks.  The main thread adds the tallies up at the end.
Tally runParallel(long nPhotons, unsigned nThreads, const Tables& tables) {
    std::vector<std::future<Tally>> parts;
    for (unsigned i = 0; i < nThreads; ++i) {
        long n = nPhotons / nThreads + (long(i) < nPhotons % long(nThreads) ? 1 : 0);
        parts.push_back(std::async(std::launch::async, simulate, n, 1000 + i, std::cref(tables)));
    }
    Tally total = parts[0].get();
    for (unsigned i = 1; i < nThreads; ++i) total += parts[i].get();
    return total;
}

// Part (d): the same job with 1, 2, ... threads up to the number the
// hardware has, and then twice that many
void scaling(long nPhotons, const Tables& tables) {
    unsigned cores = std::thread::hardware_concurrency();
    std::vector<unsigned> counts;
    for (unsigned t = 1; t <= cores; ++t) counts.push_back(t);
    counts.push_back(2 * cores);

    std::printf("hardware concurrency: %u\n\n", cores);
    std::printf("threads   time (s)   speedup   efficiency\n");
    PlotProfile measured;
    double t1 = 0;
    for (unsigned n : counts) {
        auto start = std::chrono::steady_clock::now();
        Tally tally = runParallel(nPhotons, n, tables);
        double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (n == 1) t1 = s;
        std::printf("%5u     %7.3f    %6.2f      %4.0f%%\n", n, s, t1 / s, 100 * t1 / s / n);
        measured.addPoint(n, t1 / s);
    }

    PRectF rect;
    rect.setXmin(0.0);
    rect.setXmax(2 * cores + 1);
    rect.setYmin(0.0);
    rect.setYmax(cores + 1);
    PlotView view(rect);
    Variable X;
    PlotFunction1D ideal(X);
    {
        PlotFunction1D::Properties prop;
        prop.pen.setColor("gray");
        prop.pen.setStyle(Qt::DashLine);
        prop.pen.setWidth(2);
        ideal.setProperties(prop);
    }
    {
        PlotProfile::Properties prop;
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor("darkblue");
        prop.symbolSize = 9;
        measured.setProperties(prop);
    }
    view.add(&ideal);
    view.add(&measured);
    PlotKey key(1.0, cores + 0.5);
    key.add(&measured, "measured");
    key.add(&ideal, "speedup = threads");
    view.add(&key);

    char title[200];
    std::snprintf(title, sizeof title, "Speedup, %.0f million photons", nPhotons / 1e6);
    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                << PlotStream::Size(16) << title << PlotStream::EndP();
    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                 << PlotStream::Size(14) << "threads" << PlotStream::EndP();
    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                 << PlotStream::Size(14) << "speedup  t(1 thread) / t(n threads)" << PlotStream::EndP();
    view.save("speedup.png");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    long nTiming = argc > 1 ? std::atol(argv[1]) : 25000000;      // photons in each timing run
    long nPhotons = argc > 2 ? std::atol(argv[2]) : 250000000;    // photons for the plots
    unsigned cores = std::thread::hardware_concurrency();
    Tables tables = makeTables();

    scaling(nTiming, tables);

    auto start = std::chrono::steady_clock::now();
    Tally tally = runParallel(nPhotons, cores, tables);
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    size_t entries = 0;
    for (const Hist1D& h : tally.wide) entries += h.entries();
    std::printf("\n%ld photons on %u threads in %.2f s\n", tally.photons, cores, seconds);
    std::printf("photons in the merged histograms: %zu\n", entries);
    std::printf("3d trace vs 4 theta_r - 2 theta_i: largest difference %.1e degrees\n\n", tally.worst);
    std::printf("band          peak of histogram   Descartes angle at the band edges\n");
    for (int k = 0; k < Tally::BANDS; ++k) {
        const Hist1D& h = tally.narrow[k];
        unsigned int best = 0;
        for (unsigned int i = 0; i < h.nBins(); ++i)
            if (h.bin(i) > h.bin(best)) best = i;
        double l0 = Tables::LMIN + 50 * k, l1 = l0 + 50;
        std::printf("%s    %6.2f deg          %6.2f - %6.2f deg\n", BAND_NAME[k], h.binCenter(best),
                    rainbowAngle(Tables::at(tables.n, l0)), rainbowAngle(Tables::at(tables.n, l1)));
    }

    plotAngles(tally, false, "rainbow_angle.png");
    plotAngles(tally, true, "rainbow_angle_zoom.png");
    drawSky(tally, "rainbow_sky.png");
    drawBand(tally, "rainbow_band.png");
    std::printf("\nwrote speedup.png, rainbow_angle.png, rainbow_angle_zoom.png, rainbow_sky.png, rainbow_band.png\n");
    return 0;
}
