#include <QStyle>
#include <QStyleHints>
#include <QGuiApplication>
#include <QApplication>
#include "QatDataAnalysis/Hist1D.h"
#include "QatGenericFunctions/Variable.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotHist1D.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include "QatPlotting/RealArg.h"
#include <cmath>
#include <cstdio>
#include <functional>
#include <numbers>
#include <random>

using namespace Genfun;

const unsigned int NSAMPLES = 1000000;
const double PI = std::numbers::pi;

// Transformation 1: y = xmin (xmax/xmin)^x
const double XMIN = 1.0, XMAX = 10.0;
// Transformation 2: y = sqrt(M^2 + M Gamma tan(M Gamma x))
const double M = 100.0, GAMMA = 10.0;

void setTitles(PlotView& view, const char* title, const char* xLabel, const char* yLabel) {
    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << title << PlotStream::EndP();
    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << xLabel << PlotStream::EndP();
    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << yLabel << PlotStream::EndP();
}

// chi^2 of a histogram against a cumulative distribution F, normalized to ntotal
void chi2Test(const Hist1D& h, std::function<double(double)> F, double ntotal) {
    double chi2 = 0.0;
    unsigned int ndof = 0;
    for (unsigned int i = 0; i < h.nBins(); ++i) {
        double expected = ntotal * (F(h.binUpperEdge(i)) - F(h.binLowerEdge(i)));
        if (expected < 5) continue;
        chi2 += std::pow(h.bin(i) - expected, 2) / expected;
        ++ndof;
    }
    std::printf("  chi^2 / ndof = %.1f / %u\n", chi2, ndof);
}

void makePlot(const Hist1D& h, GENFUNCTION curve, double lo, double hi, PRectF rect, double keyX, double keyY,
              const char* dataLabel, const char* curveLabel,
              const char* title, const char* xLabel, const char* yLabel, const char* file) {
    PlotView view(rect);
    PlotHist1D pHist(h);
    PlotHist1D::Properties hp;
    hp.pen.setColor("blue");
    hp.brush.setStyle(Qt::SolidPattern);
    hp.brush.setColor(QColor(170, 200, 255));
    pHist.setProperties(hp);
    // draw the analytic density only where it is defined
    PlotFunction1D pCurve(curve, RealArg::Gt(lo) && RealArg::Lt(hi));
    PlotFunction1D::Properties fp;
    fp.pen.setColor("red");
    fp.pen.setWidth(3);
    pCurve.setProperties(fp);
    view.add(&pHist);
    view.add(&pCurve);
    PlotKey key(keyX, keyY);
    key.add(&pHist, dataLabel);
    key.add(&pCurve, curveLabel);
    view.add(&key);
    setTitles(view, title, xLabel, yLabel);
    view.save(file);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    std::mt19937 engine(12345);
    std::uniform_real_distribution<double> flat(0.0, 1.0);
    Variable Y;

    // ---- Transformation 1 --------------------------------------------------
    // y = xmin R^x with R = xmax/xmin, so x = ln(y/xmin)/ln R and
    // rho(y) = dx/dy = 1 / (y ln R) on [xmin, xmax]
    {
        Hist1D h("log-uniform", 90, XMIN, XMAX);
        for (unsigned int i = 0; i < NSAMPLES; ++i) {
            double x = flat(engine);
            h.accumulate(XMIN * std::pow(XMAX / XMIN, x));
        }
        double lnR = std::log(XMAX / XMIN);
        std::printf("Transformation 1: y = xmin (xmax/xmin)^x, xmin = %.0f, xmax = %.0f\n", XMIN, XMAX);
        std::printf("  underflow %zu, overflow %zu\n", h.underflow(), h.overflow());
        chi2Test(h, [&](double y) { return std::log(y / XMIN) / lnR; }, NSAMPLES);

        GENFUNCTION curve = NSAMPLES * h.binWidth() / lnR / Y;
        PRectF rect;
        rect.setXmin(0.0); rect.setXmax(11.0);
        rect.setYmin(0.0); rect.setYmax(50000.0);
        makePlot(h, curve, XMIN, XMAX, rect, 2.6, 46000.0,
                 "y = x_min (x_max/x_min)^x,  x uniform in [0,1]",
                 "rho(y) = 1 / (y ln(x_max/x_min))",
                 "Transformation 1: x_min = 1, x_max = 10",
                 "y", "entries per bin (bin width 0.1)", "transform_powerlaw.png");
    }

    // ---- Transformation 2 --------------------------------------------------
    // s = y^2 = M^2 + M Gamma tan(theta) with theta = M Gamma x.  For x in
    // [0,1], theta runs over ~318 periods of tan, so t = tan(theta) is
    // Cauchy distributed and s has the Breit-Wigner density
    //   rho(s) = (1/pi) M Gamma / ((s - M^2)^2 + M^2 Gamma^2).
    // When tan(theta) < -M/Gamma, s < 0 and the square root does not exist.
    // For y = sqrt(s):
    //   rho(y) = (2y/pi) M Gamma / ((y^2 - M^2)^2 + M^2 Gamma^2)
    {
        Hist1D h("Breit-Wigner", 100, 50.0, 150.0);
        unsigned int negative = 0;
        for (unsigned int i = 0; i < NSAMPLES; ++i) {
            double x = flat(engine);
            double s = M * M + M * GAMMA * std::tan(M * GAMMA * x);
            if (s < 0) { ++negative; continue; }
            h.accumulate(std::sqrt(s));
        }
        // F(y) = (1/pi) atan((y^2 - M^2)/(M Gamma)), up to a constant
        auto F = [](double y) { return std::atan((y * y - M * M) / (M * GAMMA)) / PI; };
        double expectedNeg = std::atan(GAMMA / M) / PI;   // P(tan(theta) < -M/Gamma)
        double expectedOut = 1.0 - expectedNeg - (F(150.0) - F(50.0));
        std::printf("\nTransformation 2: y = sqrt(M^2 + M Gamma tan(M Gamma x)), M = %.0f, Gamma = %.0f\n", M, GAMMA);
        std::printf("  M Gamma = %.0f, so tan(M Gamma x) goes through %.1f periods for x in [0,1]\n",
                    M * GAMMA, M * GAMMA / PI);
        std::printf("  s < 0 (no real y): %u = %.4f of all, expected atan(Gamma/M)/pi = %.4f\n",
                    negative, double(negative) / NSAMPLES, expectedNeg);
        std::printf("  outside [50,150]: %zu = %.4f of all, expected %.4f\n",
                    h.underflow() + h.overflow(), double(h.underflow() + h.overflow()) / NSAMPLES, expectedOut);
        chi2Test(h, F, NSAMPLES);

        GENFUNCTION curve = NSAMPLES * h.binWidth() * (2.0 / PI) * M * GAMMA * Y
                            / ((Y * Y - M * M) * (Y * Y - M * M) + M * M * GAMMA * GAMMA);
        PRectF rect;
        rect.setXmin(50.0); rect.setXmax(150.0);
        rect.setYmin(0.0);  rect.setYmax(80000.0);
        makePlot(h, curve, 0.0, 1e9, rect, 52.0, 76000.0,
                 "y = sqrt(M^2 + M Gamma tan(M Gamma x)),  x uniform in [0,1]",
                 "rho(y) = (2y/pi) M Gamma / ((y^2-M^2)^2 + M^2 Gamma^2)",
                 "Transformation 2: M = 100, Gamma = 10",
                 "y", "entries per bin (bin width 1)", "transform_breitwigner.png");
    }

    std::printf("\nwrote transform_powerlaw.png and transform_breitwigner.png\n");
    return 0;
}