#include <QStyle>
#include <QStyleHints>
#include <QGuiApplication>
#include <QApplication>
#include "QatDataAnalysis/Hist1D.h"
#include "QatGenericFunctions/Variable.h"
#include "QatGenericFunctions/Exp.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotHist1D.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <random>
#include <vector>

using namespace Genfun;

// Metropolis walk in 3D velocity space (units of sqrt(kT/m)).
// Target density: exp(-|u|^2/2).  The speed |u| then follows
// rho(u) = sqrt(2/pi) u^2 exp(-u^2/2).
const unsigned int NSAMPLES = 100000;   // recorded samples
const unsigned int THIN     = 10;       // record every THIN-th step
const unsigned int BURNIN   = 1000;     // steps thrown away at the start
const double       STEP     = 1.0;      // width of the Gaussian proposal
const unsigned int NBINS    = 100;
const double       UMAX     = 5.0;

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

PlotHist1D::Properties histStyle(const char* line, QColor fill) {
    PlotHist1D::Properties prop;
    prop.pen.setColor(line);
    prop.pen.setWidth(2);
    if (fill.isValid()) {
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor(fill);
    }
    return prop;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    std::mt19937 engine(12345);
    std::normal_distribution<double> gauss(0.0, STEP);
    std::uniform_real_distribution<double> flat(0.0, 1.0);

    std::array<double, 3> u = {0.0, 0.0, 0.0};
    double u2 = 0.0;
    unsigned long accepted = 0, tried = 0;

    auto step = [&]() {
        std::array<double, 3> trial;
        double trial2 = 0.0;
        for (int k = 0; k < 3; ++k) {
            trial[k] = u[k] + gauss(engine);
            trial2 += trial[k] * trial[k];
        }
        ++tried;
        // accept with probability min(1, p(trial)/p(u))
        if (flat(engine) < std::exp(-(trial2 - u2) / 2.0)) {
            u = trial;
            u2 = trial2;
            ++accepted;
        }
    };

    for (unsigned int i = 0; i < BURNIN; ++i) step();
    accepted = tried = 0;

    Hist1D speedHist("speed", NBINS, 0.0, UMAX);
    Hist1D compHist[3] = {Hist1D("ux", NBINS, -5.0, 5.0),
                          Hist1D("uy", NBINS, -5.0, 5.0),
                          Hist1D("uz", NBINS, -5.0, 5.0)};
    std::vector<double> speeds;
    speeds.reserve(NSAMPLES);
    for (unsigned int i = 0; i < NSAMPLES; ++i) {
        for (unsigned int t = 0; t < THIN; ++t) step();
        double s = std::sqrt(u2);
        speeds.push_back(s);
        speedHist.accumulate(s);
        for (int k = 0; k < 3; ++k) compHist[k].accumulate(u[k]);
    }

    // Means, with errors from 100 batches (the samples are correlated)
    const unsigned int NBATCH = 100, BSIZE = NSAMPLES / NBATCH;
    double meanU = 0, meanU2 = 0, varU = 0, varU2 = 0;
    std::vector<double> bU(NBATCH, 0.0), bU2(NBATCH, 0.0);
    for (unsigned int i = 0; i < NSAMPLES; ++i) {
        bU[i / BSIZE]  += speeds[i] / BSIZE;
        bU2[i / BSIZE] += speeds[i] * speeds[i] / BSIZE;
    }
    for (unsigned int b = 0; b < NBATCH; ++b) { meanU += bU[b] / NBATCH; meanU2 += bU2[b] / NBATCH; }
    for (unsigned int b = 0; b < NBATCH; ++b) {
        varU  += std::pow(bU[b] - meanU, 2) / (NBATCH - 1);
        varU2 += std::pow(bU2[b] - meanU2, 2) / (NBATCH - 1);
    }

    // correlation between neighbouring recorded speeds
    double c = 0.0, v = 0.0;
    for (unsigned int i = 0; i < NSAMPLES; ++i) {
        v += std::pow(speeds[i] - meanU, 2);
        if (i > 0) c += (speeds[i] - meanU) * (speeds[i - 1] - meanU);
    }

    std::printf("proposal width %.2f, acceptance rate %.3f\n", STEP, double(accepted) / tried);
    std::printf("recorded %u samples, one every %u steps, after %u burn-in steps\n", NSAMPLES, THIN, BURNIN);
    std::printf("lag-1 correlation of recorded speeds: %.3f\n", c / v);
    std::printf("<u>   = %.4f +- %.4f   exact sqrt(8/pi) = %.4f\n", meanU, std::sqrt(varU / NBATCH), std::sqrt(8.0 / std::numbers::pi));
    std::printf("<u^2> = %.4f +- %.4f   exact 3\n", meanU2, std::sqrt(varU2 / NBATCH));

    double chi2 = 0.0;
    unsigned int ndof = 0;
    auto cdf = [](double s) {
        return std::erf(s / std::sqrt(2.0)) - std::sqrt(2.0 / std::numbers::pi) * s * std::exp(-s * s / 2);
    };
    for (unsigned int i = 0; i < NBINS; ++i) {
        double expected = NSAMPLES * (cdf(speedHist.binUpperEdge(i)) - cdf(speedHist.binLowerEdge(i)));
        if (expected < 5) continue;
        chi2 += std::pow(speedHist.bin(i) - expected, 2) / expected;
        ++ndof;
    }
    std::printf("speed histogram chi^2 / ndof = %.1f / %u\n", chi2, ndof);

    Variable U;
    Exp exp;

    // Plot 1: speed
    {
        double scale = NSAMPLES * speedHist.binWidth();
        GENFUNCTION maxwell = scale * std::sqrt(2.0 / std::numbers::pi) * U * U * exp(-U * U / 2.0);

        PRectF rect;
        rect.setXmin(0.0); rect.setXmax(UMAX);
        rect.setYmin(0.0); rect.setYmax(3600.0);
        PlotView view(rect);

        PlotHist1D pHist(speedHist);
        pHist.setProperties(histStyle("blue", QColor(170, 200, 255)));
        PlotFunction1D pMaxwell(maxwell);
        PlotFunction1D::Properties prop;
        prop.pen.setColor("red");
        prop.pen.setWidth(3);
        pMaxwell.setProperties(prop);
        view.add(&pHist);
        view.add(&pMaxwell);

        PlotKey key(2.2, 3400.0);
        key.add(&pHist, "MCMC (Metropolis in 3D), |u|");
        key.add(&pMaxwell, "theory: sqrt(2/pi) u^2 exp(-u^2/2)");
        view.add(&key);

        setTitles(view, "Maxwell speed distribution from Markov chain Monte Carlo",
                  "u = v / sqrt(k_B T / m)", "samples per bin (bin width 0.05)");
        view.save("maxwell_mcmc_speed.png");
    }

    // Plot 2: the three velocity components
    {
        double scale = NSAMPLES * compHist[0].binWidth();
        GENFUNCTION gaussian = scale / std::sqrt(2.0 * std::numbers::pi) * exp(-U * U / 2.0);

        PRectF rect;
        rect.setXmin(-5.0); rect.setXmax(5.0);
        rect.setYmin(0.0);  rect.setYmax(6400.0);
        PlotView view(rect);

        PlotHist1D px(compHist[0]), py(compHist[1]), pz(compHist[2]);
        px.setProperties(histStyle("blue", QColor()));
        py.setProperties(histStyle("darkGreen", QColor()));
        pz.setProperties(histStyle("magenta", QColor()));
        PlotFunction1D pGauss(gaussian);
        PlotFunction1D::Properties prop;
        prop.pen.setColor("red");
        prop.pen.setWidth(3);
        pGauss.setProperties(prop);
        view.add(&px);
        view.add(&py);
        view.add(&pz);
        view.add(&pGauss);

        PlotKey key(-4.8, 6200.0);
        key.add(&px, "MCMC u_x");
        key.add(&py, "MCMC u_y");
        key.add(&pz, "MCMC u_z");
        key.add(&pGauss, "theory: Gaussian, sigma = 1");
        view.add(&key);

        setTitles(view, "Velocity components from Markov chain Monte Carlo",
                  "velocity component (units of sqrt(k_B T / m))", "samples per bin (bin width 0.1)");
        view.save("maxwell_mcmc_components.png");
    }

    std::printf("wrote maxwell_mcmc_speed.png and maxwell_mcmc_components.png\n");
    return 0;
}