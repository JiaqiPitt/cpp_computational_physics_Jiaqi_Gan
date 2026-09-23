#include "QatGenericFunctions/AbsFunction.h"
#include "QatGenericFunctions/InterpolatingFunction.h"
#include "QatGenericFunctions/QuadratureRule.h"
#include "QatGenericFunctions/SimpleIntegrator.h"
#include "QatGenericFunctions/Sin.h"
#include "QatGenericFunctions/Sqrt.h"
#include "QatGenericFunctions/Variable.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotProfile.h"
#include "QatPlotting/PlotStream.h"
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleHints>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace Genfun;

const double PI = std::acos(-1.0);
const double T0 = 2.0 * PI;
const double DT = 1.0e-3;

struct State {
    double theta;
    double omega;
};

State derivative(const State& s) {
    return {s.omega, -std::sin(s.theta)};
}

State rk4Step(const State& s, double h) {
    State k1 = derivative(s);
    State k2 = derivative({s.theta + 0.5 * h * k1.theta, s.omega + 0.5 * h * k1.omega});
    State k3 = derivative({s.theta + 0.5 * h * k2.theta, s.omega + 0.5 * h * k2.omega});
    State k4 = derivative({s.theta + h * k3.theta, s.omega + h * k3.omega});
    return {s.theta + h / 6.0 * (k1.theta + 2 * k2.theta + 2 * k3.theta + k4.theta),
            s.omega + h / 6.0 * (k1.omega + 2 * k2.omega + 2 * k3.omega + k4.omega)};
}

double quarterPeriod(double thetaMax) {
    State s{thetaMax, 0.0};
    double t = 0.0;
    while (true) {
        State next = rk4Step(s, DT);
        if (next.theta <= 0.0) {
            double h0 = 0.0, h1 = DT;
            double f0 = s.theta, f1 = next.theta;
            for (int i = 0; i < 20 && std::fabs(h1 - h0) > 1e-15; ++i) {
                double h2 = h1 - f1 * (h1 - h0) / (f1 - f0);
                h0 = h1; f0 = f1;
                h1 = h2; f1 = rk4Step(s, h1).theta;
            }
            return t + h1;
        }
        s = next;
        t += DT;
    }
}

InterpolatingFunction trajectory(double thetaMax, double tMax) {
    InterpolatingFunction f(CUBIC_SPLINE);
    State s{thetaMax, 0.0};
    double t = 0.0;
    int step = 0;
    f.addPoint(0.0, 1.0);
    while (t < tMax) {
        s = rk4Step(s, DT);
        t += DT;
        if (++step % 20 == 0) f.addPoint(t / T0, s.theta / thetaMax);
    }
    return f;
}

double exactFactor(double thetaMaxDeg) {
    double k = std::sin(thetaMaxDeg * PI / 180.0 / 2.0);
    Variable phi;
    GENFUNCTION integrand = 1.0 / Sqrt()(1.0 - k * k * Sin()(phi) * Sin()(phi));
    SimpsonsRule simpson;
    SimpleIntegrator K(0.0, PI / 2.0, simpson, 2000);
    return 2.0 / PI * K(integrand);
}

class ExactFactor : public AbsFunction {
    FUNCTION_OBJECT_DEF(ExactFactor)
public:
    ExactFactor() {}
    ExactFactor(const ExactFactor& right) : AbsFunction(right) {}
    virtual ~ExactFactor() {}
    virtual double operator() (double deg) const {return exactFactor(deg);}
    virtual double operator() (const Argument& a) const {return operator()(a[0]);}
};
FUNCTION_OBJECT_IMP(ExactFactor)

void labels(PlotView& view, const std::string& title, const std::string& x, const std::string& y) {
    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16) << title << PlotStream::EndP();
    PlotStream xStream(view.xLabelTextEdit());
    xStream << PlotStream::Clear() << PlotStream::Center()
            << PlotStream::Family("Arial") << PlotStream::Size(14) << x << PlotStream::EndP();
    PlotStream yStream(view.yLabelTextEdit());
    yStream << PlotStream::Clear() << PlotStream::Center()
            << PlotStream::Family("Arial") << PlotStream::Size(14) << y << PlotStream::EndP();
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    const std::vector<int> numerators = {1, 2, 4, 6, 8, 10, 11};
    const std::vector<std::string> names = {"pi/12", "pi/6", "pi/3", "pi/2", "2pi/3", "5pi/6", "11pi/12"};

    std::cout << std::fixed << std::setprecision(8);
    std::cout << " theta_max   degrees    f (RK4)       f (exact)     difference\n";
    PlotProfile rkPoints;
    for (size_t i = 0; i < numerators.size(); ++i) {
        double thetaMax = numerators[i] * PI / 12.0;
        double deg = numerators[i] * 15.0;
        double f = 4.0 * quarterPeriod(thetaMax) / T0;
        double fe = exactFactor(deg);
        rkPoints.addPoint(deg, f);
        std::cout << std::setw(9) << names[i] << std::setw(9) << std::setprecision(0) << deg
                  << std::setprecision(8) << std::setw(14) << f << std::setw(14) << fe
                  << std::setw(14) << std::scientific << std::setprecision(2) << f - fe
                  << std::fixed << std::setprecision(8) << "\n";
    }

    {
        PRectF rect;
        rect.setXmin(0.0);
        rect.setXmax(180.0);
        rect.setYmin(0.9);
        rect.setYmax(2.5);
        PlotView view(rect);

        ExactFactor exact;
        PlotFunction1D curve(exact);
        PlotFunction1D::Properties cprop;
        cprop.pen.setColor("blue");
        cprop.pen.setWidth(2);
        curve.setProperties(cprop);

        PlotProfile::Properties pprop;
        pprop.pen.setColor("red");
        pprop.brush.setStyle(Qt::SolidPattern);
        pprop.brush.setColor("red");
        pprop.symbolSize = 9;
        rkPoints.setProperties(pprop);

        view.add(&curve);
        view.add(&rkPoints);

        PlotKey key(8.0, 2.4);
        key.add(&rkPoints, "RK4 integration of the equation of motion");
        key.add(&curve, "exact: (2/pi) K(k),  k = sin(theta_max / 2)");
        view.add(&key);

        labels(view, "Large-amplitude correction to the pendulum period",
               "amplitude theta_max (degrees)", "f(theta_max) = T / T0");
        view.save("period_factor.png");
    }

    {
        PRectF rect;
        rect.setXmin(0.0);
        rect.setXmax(2.5);
        rect.setYmin(-1.1);
        rect.setYmax(2.7);
        PlotView view(rect);

        const char* colors[] = {"blue", "red", "darkGreen", "magenta", "darkCyan", "darkOrange", "black"};
        std::vector<InterpolatingFunction> paths;
        std::vector<PlotFunction1D*> curves;
        for (size_t i = 0; i < numerators.size(); ++i) {
            paths.push_back(trajectory(numerators[i] * PI / 12.0, 2.5 * T0));
        }
        PlotKey key(0.05, 2.6);
        for (size_t i = 0; i < numerators.size(); ++i) {
            PlotFunction1D* c = new PlotFunction1D(paths[i]);
            PlotFunction1D::Properties prop;
            prop.pen.setColor(colors[i]);
            prop.pen.setWidth(2);
            c->setProperties(prop);
            view.add(c);
            key.add(c, "theta_max = " + names[i]);
            curves.push_back(c);
        }
        view.add(&key);
        labels(view, "Pendulum motion for different amplitudes (RK4)",
               "time t / T0", "theta(t) / theta_max");
        view.save("pendulum_motion.png");
        for (auto c : curves) delete c;
    }

    std::cout << "\nwrote period_factor.png and pendulum_motion.png\n";
    return 0;
}