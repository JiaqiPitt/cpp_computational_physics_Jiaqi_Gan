#include "QatGenericFunctions/Sin.h"
#include "QatGenericFunctions/Exp.h"
#include "QatGenericFunctions/Variable.h"
#include "QatGenericFunctions/AssociatedLegendre.h"
#include "QatGenericFunctions/Polynomial.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include <QApplication>
#include <iostream>
#include <cstdlib>
#include <string>

using namespace Genfun;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <1-7>\n";
        return 1;
    }
    int choice = std::atoi(argv[1]);

    QApplication app(argc, argv);

    PRectF rect;
    rect.setXmin(-3.0);
    rect.setXmax(3.0);
    rect.setYmin(-3.0);
    rect.setYmax(3.0);

    PlotView view(rect);
    Variable X;

    std::string name;

    switch (choice) {
        case 1: {
            Sin f;
            PlotFunction1D P(f);
            view.add(&P);
            name = "a_sin_x";
            view.save(name + ".png");
            break;
        }
        case 2: {
            GENFUNCTION f = Sin()(5.0*X);
            PlotFunction1D P(f);
            view.add(&P);
            name = "b_sin_5x";
            view.save(name + ".png");
            break;
        }
        case 3: {
            GENFUNCTION f = Sin()(X*X);
            PlotFunction1D P(f);
            view.add(&P);
            name = "c_sin_x2";
            view.save(name + ".png");
            break;
        }
        case 4: {
            GENFUNCTION f = Exp()(-1.0*X) * Sin();
            PlotFunction1D P(f);
            view.add(&P);
            name = "d_exp_sin";
            view.save(name + ".png");
            break;
        }
        case 5: {
            rect.setXmin(-1.0);
            rect.setXmax(1.0);
            rect.setYmin(-6.0);
            rect.setYmax(6.0);
            view.setRect(rect);
            AssociatedLegendre f(3, 2);
            PlotFunction1D P(f);
            view.add(&P);
            name = "e_legendre32";
            view.save(name + ".png");
            break;
        }
        case 6: {
            Polynomial f({1.0, 0.0, -2.0, 0.0});
            PlotFunction1D P(f);
            view.add(&P);
            name = "f_myfunc";
            view.save(name + ".png");
            break;
        }
        case 7: {
            Polynomial p({1.0, 0.0, -2.0, 0.0});
            GENFUNCTION f = p.prime();
            PlotFunction1D P(f);
            view.add(&P);
            name = "g_myfunc_prime";
            view.save(name + ".png");
            break;
        }
        default:
            std::cerr << "choice must be 1-7\n";
            return 1;
    }

    std::cout << "wrote " << name << ".png\n";
    return 0;
}