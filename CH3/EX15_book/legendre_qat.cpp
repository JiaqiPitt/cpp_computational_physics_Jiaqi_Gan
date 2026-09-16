#include "QatGenericFunctions/LegendrePolynomial.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotProfile.h"
#include <QApplication>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Genfun;

double newtonRaphson(double x, GENFUNCTION P) {
    double x1 = x;
    while (true) {
        double deltaX = -P(x) / P.prime()(x);
        x += deltaX;
        if (float(x1) == float(x)) break;
        x1 = x;
    }
    return x;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <l>\n";
        return 1;
    }
    int l = std::atoi(argv[1]);
    if (l < 1) {
        std::cerr << "l must be >= 1\n";
        return 1;
    }

    QApplication app(argc, argv);

    const double PI = std::acos(-1.0);

    PRectF rect;
    rect.setXmin(-1.0);
    rect.setXmax(1.0);
    rect.setYmin(-1.2);
    rect.setYmax(1.2);

    PlotView view(rect);

    LegendrePolynomial P(l);
    PlotFunction1D plotP(P);
    view.add(&plotP);

    PlotProfile zeros;

    std::vector<double> roots;
    for (int m = 1; m <= l; ++m) {
        double guess = (1.0 - 1.0/(8.0*l*l) + 1.0/(8.0*l*l*l))
                      * std::cos((4.0*m - 1.0)/(4.0*l + 2.0) * PI);
        double root = newtonRaphson(guess, P);
        roots.push_back(root);
        zeros.addPoint(root, 0.0);
    }

    {
        PlotProfile::Properties prop;
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor("darkBlue");
        prop.symbolSize = 8;
        zeros.setProperties(prop);
    }
    view.add(&zeros);

    std::cout << "zeros of P_" << l << "(x):\n";
    for (double r : roots) {
        std::cout << "  " << r << "\n";
    }

    view.save("legendre_l" + std::to_string(l) + ".png");

    return 0;
}