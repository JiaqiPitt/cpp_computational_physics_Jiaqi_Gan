#include <QStyle>
#include <QStyleHints>
#include <QGuiApplication>
#include <QApplication>
#include "QatDataAnalysis/Hist1D.h"
#include "QatGenericFunctions/LegendrePolynomial.h"
#include "QatGenericFunctions/GaussQuadratureRule.h"
#include "QatGenericFunctions/GaussIntegrator.h"
#include "QatGenericFunctions/Variable.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotHist1D.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include <cmath>
#include <cstdio>
#include <random>

using namespace Genfun;

const unsigned int NSAMPLES = 100000;   // accepted events
const unsigned int NBINS    = 100;

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    // rho(cos theta) = (sqrt(3/8) P1 - sqrt(7/6) P3 + sqrt(11/24) P5 - sqrt(15/6) P7)^2
    LegendrePolynomial P1(1), P3(3), P5(5), P7(7);
    GENFUNCTION amp = std::sqrt(3.0 / 8.0) * P1 - std::sqrt(7.0 / 6.0) * P3
                    + std::sqrt(11.0 / 24.0) * P5 - std::sqrt(15.0 / 6.0) * P7;
    GENFUNCTION rho = amp * amp;

    // rho is a polynomial of degree 14, so an 8-point Gauss-Legendre rule
    // integrates it exactly (exact up to degree 2*8-1 = 15)
    GaussLegendreRule legendre(8);
    GaussIntegrator integral(legendre);
    double rhoMax = 0.0;
    for (int i = -100000; i <= 100000; ++i) rhoMax = std::max(rhoMax, rho(i / 100000.0));
    double bound = 1.01 * rhoMax;   // small safety margin for the rejection box
    std::printf("integral of rho over [-1,1] = %.10f\n", integral(rho));
    std::printf("max of rho = %.6f (at the ends: rho(1) = %.6f), box height = %.6f\n", rhoMax, rho(1.0), bound);

    // Rejection method: x uniform in [-1,1], y uniform in [0, bound], keep x if y < rho(x)
    std::mt19937 engine(12345);
    std::uniform_real_distribution<double> flatX(-1.0, 1.0), flatY(0.0, bound);
    Hist1D hist("cos theta", NBINS, -1.0, 1.0);
    unsigned long tried = 0;
    while (hist.entries() < NSAMPLES) {
        double x = flatX(engine), y = flatY(engine);
        ++tried;
        if (y < rho(x)) hist.accumulate(x);
    }
    std::printf("accepted %u of %lu tries, efficiency %.4f (expected 1/(2*box) = %.4f)\n",
                NSAMPLES, tried, double(NSAMPLES) / tried, 1.0 / (2.0 * bound));

    // chi^2 against the exact content of each bin
    double chi2 = 0.0;
    unsigned int ndof = 0;
    for (unsigned int i = 0; i < NBINS; ++i) {
        // map the bin [a,b] onto [-1,1]: x = (b-a)/2 t + (a+b)/2
        double a = hist.binLowerEdge(i), b = hist.binUpperEdge(i);
        Variable T;
        GENFUNCTION rhoBin = rho((b - a) / 2.0 * T + (a + b) / 2.0);
        double expected = NSAMPLES * (b - a) / 2.0 * integral(rhoBin);
        if (expected < 5) continue;
        chi2 += std::pow(hist.bin(i) - expected, 2) / expected;
        ++ndof;
    }
    std::printf("chi^2 / ndof = %.1f / %u\n", chi2, ndof);

    PRectF rect;
    rect.setXmin(-1.0);
    rect.setXmax(1.0);
    rect.setYmin(0.0);
    rect.setYmax(NSAMPLES * hist.binWidth() * rhoMax * 1.35);
    PlotView view(rect);

    PlotHist1D pHist(hist);
    {
        PlotHist1D::Properties prop;
        prop.pen.setColor("blue");
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor(QColor(170, 200, 255));
        pHist.setProperties(prop);
    }
    GENFUNCTION curve = NSAMPLES * hist.binWidth() * rho;
    PlotFunction1D pRho(curve);
    {
        PlotFunction1D::Properties prop;
        prop.pen.setColor("red");
        prop.pen.setWidth(3);
        pRho.setProperties(prop);
    }
    view.add(&pHist);
    view.add(&pRho);

    PlotKey key(-0.95, NSAMPLES * hist.binWidth() * rhoMax * 1.3);
    key.add(&pHist, "generated (rejection method)");
    key.add(&pRho, "rho(cos theta)");
    view.add(&key);

    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << "Angular distribution of the visible decay product"
                << PlotStream::EndP();
    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "cos theta"
                 << PlotStream::EndP();
    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "events per bin (bin width 0.02)"
                 << PlotStream::EndP();

    view.save("angular_distribution.png");
    std::printf("wrote angular_distribution.png\n");
    return 0;
}