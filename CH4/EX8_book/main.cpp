#include "Photoreceptor.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotProfile.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleHints>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    Photoreceptor blue("curve_blue.csv");
    Photoreceptor rod("curve_rod.csv");
    Photoreceptor green("curve_green.csv");
    Photoreceptor red("curve_red.csv");

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "response at the published peak wavelengths:\n";
    std::cout << "  blue  cones at 420 nm: " << blue(420.0)  << "\n";
    std::cout << "  rods        at 498 nm: " << rod(498.0)   << "\n";
    std::cout << "  green cones at 534 nm: " << green(534.0) << "\n";
    std::cout << "  red   cones at 564 nm: " << red(564.0)   << "\n";

    std::cout << "\n lambda    blue     rod   green     red\n";
    for (double lam = 400.0; lam <= 700.0; lam += 25.0) {
        std::cout << std::setw(7) << lam << std::setw(8) << blue(lam) << std::setw(8) << rod(lam)
                  << std::setw(8) << green(lam) << std::setw(8) << red(lam) << "\n";
    }

    PRectF rect;
    rect.setXmin(400.0);
    rect.setXmax(700.0);
    rect.setYmin(0.0);
    rect.setYmax(140.0);
    PlotView view(rect);

    struct Style { const char* color; bool filled; PlotProfile::Properties::SymbolStyle symbol; };
    Photoreceptor* receptors[4] = {&blue, &rod, &green, &red};
    Style styles[4] = {{"blue", false, PlotProfile::Properties::CIRCLE},
                       {"black", true, PlotProfile::Properties::CIRCLE},
                       {"darkGreen", false, PlotProfile::Properties::CIRCLE},
                       {"red", false, PlotProfile::Properties::TRIANGLE_U}};
    const char* labels[4] = {"blue cones (peak 420 nm)", "rods (peak 498 nm)",
                             "green cones (peak 534 nm)", "red cones (peak 564 nm)"};

    PlotFunction1D* curves[4];
    PlotProfile* points[4];
    PlotKey key(405.0, 136.0);

    for (int i = 0; i < 4; ++i) {
        curves[i] = new PlotFunction1D(*receptors[i]);
        PlotFunction1D::Properties cprop;
        cprop.pen.setColor(styles[i].color);
        cprop.pen.setWidth(2);
        curves[i]->setProperties(cprop);

        points[i] = new PlotProfile();
        const auto& lam = receptors[i]->lambdas();
        const auto& a = receptors[i]->absorbances();
        for (size_t j = 0; j < lam.size(); ++j) {
            if (lam[j] >= 399.0 && lam[j] <= 700.0) points[i]->addPoint(lam[j], a[j]);
        }
        PlotProfile::Properties pprop;
        pprop.pen.setColor(styles[i].color);
        pprop.pen.setWidth(2);
        pprop.brush.setStyle(styles[i].filled ? Qt::SolidPattern : Qt::NoBrush);
        pprop.brush.setColor(styles[i].color);
        pprop.symbolStyle = styles[i].symbol;
        pprop.symbolSize = 8;
        points[i]->setProperties(pprop);

        view.add(curves[i]);
        view.add(points[i]);
        key.add(curves[i], labels[i]);
    }
    view.add(&key);

    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << "Photoreceptor response of the human retina (digitized from Fig. 4.6)"
                << PlotStream::EndP();

    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "wavelength (nm)" << PlotStream::EndP();

    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "normalized absorbance" << PlotStream::EndP();

    view.save("photoreceptor_response.png");
    std::cout << "\nwrote photoreceptor_response.png\n";

    for (int i = 0; i < 4; ++i) {
        delete curves[i];
        delete points[i];
    }
    return 0;
}