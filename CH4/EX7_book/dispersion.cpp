#include "QatGenericFunctions/InterpolatingFunction.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotProfile.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleHints>
#include <iostream>
#include <vector>

using namespace Genfun;

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    std::vector<double> lambda = {200, 250, 300, 350, 400, 450, 500, 550, 600,
                                  650, 700, 750, 800, 850, 900, 950, 1000};
    std::vector<double> n = {1.396, 1.362, 1.349, 1.343, 1.339, 1.337, 1.335, 1.333, 1.332,
                             1.331, 1.331, 1.330, 1.329, 1.329, 1.328, 1.327, 1.327};

    InterpolatingFunction spline(CUBIC_SPLINE);
    InterpolatingFunction akima(AKIMA_SPLINE);
    InterpolatingFunction poly(POLYNOMIAL);
    PlotProfile data;

    for (size_t i = 0; i < lambda.size(); ++i) {
        spline.addPoint(lambda[i], n[i]);
        akima.addPoint(lambda[i], n[i]);
        poly.addPoint(lambda[i], n[i]);
        data.addPoint(lambda[i], n[i]);
    }

    std::cout << "n(589 nm):  spline = " << spline(589.0)
              << "  akima = " << akima(589.0)
              << "  polynomial = " << poly(589.0) << "\n";

    PRectF rect;
    rect.setXmin(180.0);
    rect.setXmax(1020.0);
    rect.setYmin(1.32);
    rect.setYmax(1.41);

    PlotView view(rect);

    PlotFunction1D pSpline(spline);
    PlotFunction1D pAkima(akima);
    PlotFunction1D pPoly(poly);

    {
        PlotFunction1D::Properties prop;
        prop.pen.setColor("blue");
        prop.pen.setWidth(2);
        pSpline.setProperties(prop);
    }
    {
        PlotFunction1D::Properties prop;
        prop.pen.setColor("red");
        prop.pen.setWidth(2);
        prop.pen.setStyle(Qt::DashLine);
        pAkima.setProperties(prop);
    }
    {
        PlotFunction1D::Properties prop;
        prop.pen.setColor("darkGreen");
        prop.pen.setWidth(1);
        prop.pen.setStyle(Qt::DotLine);
        pPoly.setProperties(prop);
    }
    {
        PlotProfile::Properties prop;
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor("black");
        prop.symbolSize = 7;
        data.setProperties(prop);
    }

    view.add(&pPoly);
    view.add(&pAkima);
    view.add(&pSpline);
    view.add(&data);

    PlotKey key(600.0, 1.40);
    key.add(&data, "Hale (1973) data");
    key.add(&pSpline, "cubic spline");
    key.add(&pAkima, "Akima spline");
    key.add(&pPoly, "degree-16 polynomial");
    view.add(&key);

    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << "Index of refraction of water vs. wavelength"
                << PlotStream::EndP();

    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "wavelength (nm)"
                 << PlotStream::EndP();

    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "index of refraction n"
                 << PlotStream::EndP();

    view.save("dispersion_interpolation.png");
    std::cout << "wrote dispersion_interpolation.png\n";

    return 0;
}