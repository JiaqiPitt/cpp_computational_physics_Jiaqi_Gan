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
#include <cmath>
#include <cstdio>
#include <numbers>
#include <random>

using namespace Genfun;

// Dimensionless speed u = v / sqrt(tau/m).  Then
//   rho(u) = sqrt(2/pi) u^2 exp(-u^2/2),
// and y = u^2/2 (kinetic energy in units of tau) follows a gamma
// distribution with shape 3/2 and scale 1, so u = sqrt(2y).
const unsigned int NSAMPLES = 100000;
const unsigned int NBINS    = 100;
const double       UMAX     = 5.0;

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    std::mt19937 engine(12345);
    std::gamma_distribution<double> gamma(1.5, 1.0);

    Hist1D hist("Maxwell speeds", NBINS, 0.0, UMAX);
    double sumU = 0.0, sumU2 = 0.0, sumU4 = 0.0;
    for (unsigned int i = 0; i < NSAMPLES; ++i) {
        double y = gamma(engine);
        double u = std::sqrt(2.0 * y);
        hist.accumulate(u);
        sumU  += u;
        sumU2 += u * u;
        sumU4 += u * u * u * u;
    }

    double meanU  = sumU / NSAMPLES;
    double meanU2 = sumU2 / NSAMPLES;
    double meanU4 = sumU4 / NSAMPLES;
    // statistical errors of the two sample means
    double errU  = std::sqrt((meanU2 - meanU * meanU) / NSAMPLES);
    double errU2 = std::sqrt((meanU4 - meanU2 * meanU2) / NSAMPLES);
    std::printf("samples: %u, overflow (u > %.1f): %zu\n", NSAMPLES, UMAX, hist.overflow());
    std::printf("<u>   = %.4f +- %.4f   exact sqrt(8/pi) = %.4f\n", meanU, errU, std::sqrt(8.0 / std::numbers::pi));
    std::printf("<u^2> = %.4f +- %.4f   exact 3\n", meanU2, errU2);

    // chi^2 of the histogram against the expected counts per bin
    double chi2 = 0.0;
    unsigned int ndof = 0;
    for (unsigned int i = 0; i < NBINS; ++i) {
        double a = hist.binLowerEdge(i), b = hist.binUpperEdge(i);
        // exact probability in the bin from the cumulative distribution
        auto cdf = [](double u) {
            return std::erf(u / std::sqrt(2.0)) - std::sqrt(2.0 / std::numbers::pi) * u * std::exp(-u * u / 2);
        };
        double expected = NSAMPLES * (cdf(b) - cdf(a));
        if (expected < 5) continue;
        chi2 += std::pow(hist.bin(i) - expected, 2) / expected;
        ++ndof;
    }
    std::printf("chi^2 / ndof = %.1f / %u\n", chi2, ndof);

    Variable U;
    Exp exp;
    double scale = NSAMPLES * hist.binWidth();
    GENFUNCTION maxwell = scale * std::sqrt(2.0 / std::numbers::pi) * U * U * exp(-U * U / 2.0);

    PRectF rect;
    rect.setXmin(0.0);
    rect.setXmax(UMAX);
    rect.setYmin(0.0);
    rect.setYmax(3600.0);
    PlotView view(rect);

    PlotHist1D pHist(hist);
    {
        PlotHist1D::Properties prop;
        prop.pen.setColor("blue");
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor(QColor(170, 200, 255));
        pHist.setProperties(prop);
    }
    PlotFunction1D pMaxwell(maxwell);
    {
        PlotFunction1D::Properties prop;
        prop.pen.setColor("red");
        prop.pen.setWidth(3);
        pMaxwell.setProperties(prop);
    }
    view.add(&pHist);
    view.add(&pMaxwell);

    PlotKey key(2.2, 3400.0);
    key.add(&pHist, "data: u = sqrt(2y), y ~ Gamma(3/2, 1)");
    key.add(&pMaxwell, "theory: sqrt(2/pi) u^2 exp(-u^2/2)");
    view.add(&key);

    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << "Maxwell speed distribution sampled with a gamma distribution"
                << PlotStream::EndP();

    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "u = v / sqrt(k_B T / m)"
                 << PlotStream::EndP();

    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "molecules per bin (bin width 0.05)"
                 << PlotStream::EndP();

    view.save("maxwell_gamma.png");
    std::printf("wrote maxwell_gamma.png\n");
    return 0;
}