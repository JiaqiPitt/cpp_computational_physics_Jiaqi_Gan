#include "QatGenericFunctions/AbsFunction.h"
#include "QatGenericFunctions/QuadratureRule.h"
#include "QatGenericFunctions/SimpleIntegrator.h"
#include "QatGenericFunctions/Sqrt.h"
#include "QatGenericFunctions/Tanh.h"
#include "QatPlotWidgets/PlotView.h"
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

class SubstitutedCoth : public AbsFunction {
    FUNCTION_OBJECT_DEF(SubstitutedCoth)
public:
    SubstitutedCoth() {}
    SubstitutedCoth(const SubstitutedCoth& right) : AbsFunction(right) {}
    virtual ~SubstitutedCoth() {}
    virtual double operator() (double t) const {
        if (t == 0.0) return 2.0;
        return 2.0 * t * std::sqrt(1.0 / std::tanh(t * t));
    }
    virtual double operator() (const Argument& a) const {return operator()(a[0]);}
};
FUNCTION_OBJECT_IMP(SubstitutedCoth)

const std::vector<unsigned int> N_VALUES = {2, 4, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64, 80, 96,
                                            128, 160, 192, 256, 320, 384, 512, 640, 768, 1024};

struct Series {
    std::string name;
    std::vector<double> h;
    std::vector<double> err;
};

Series convergence(const std::string& name, GENFUNCTION f, const QuadratureRule& rule, double exact) {
    Series s{name, {}, {}};
    int nonFinite = 0;
    for (unsigned int n : N_VALUES) {
        SimpleIntegrator integrator(0.0, 1.0, rule, n);
        double diff = std::fabs(integrator(f) - exact);
        if (!std::isfinite(diff)) {
            ++nonFinite;
            continue;
        }
        if (diff < 1e-14) continue;
        s.h.push_back(1.0 / n);
        s.err.push_back(diff);
    }
    if (nonFinite > 0) {
        std::cout << "  " << name << ": non-finite result for " << nonFinite << " of "
                  << N_VALUES.size() << " values of n, not plotted\n";
    }
    return s;
}

double fittedOrder(const Series& s) {
    size_t k = s.h.size();
    if (k < 4) return NAN;
    size_t start = k - std::min<size_t>(k, 10);
    double sx = 0, sy = 0, sxx = 0, sxy = 0, m = 0;
    for (size_t i = start; i < k; ++i) {
        double x = std::log(s.h[i]), y = std::log(s.err[i]);
        sx += x; sy += y; sxx += x * x; sxy += x * y; m += 1;
    }
    return (m * sxy - sx * sy) / (m * sxx - sx * sx);
}

void report(const std::vector<Series>& all) {
    for (const auto& s : all) {
        std::cout << "  " << std::left << std::setw(40) << s.name << std::right
                  << " points = " << std::setw(2) << s.h.size()
                  << "   fitted order = " << std::fixed << std::setprecision(2) << fittedOrder(s) << "\n";
    }
}

void plot(const std::vector<Series>& all, const std::string& title, const std::string& file,
          double ymin, double ymax, double keyX, double keyY) {
    PRectF rect;
    rect.setXmin(5e-4);
    rect.setXmax(1.0);
    rect.setYmin(ymin);
    rect.setYmax(ymax);
    PlotView view(rect);
    view.setLogX(true);
    view.setLogY(true);

    const char* colors[] = {"blue", "red", "darkGreen", "black"};
    PlotProfile::Properties::SymbolStyle symbols[] = {
        PlotProfile::Properties::CIRCLE, PlotProfile::Properties::SQUARE,
        PlotProfile::Properties::TRIANGLE_U, PlotProfile::Properties::TRIANGLE_L};

    std::vector<PlotProfile*> profiles;
    PlotKey key(keyX, keyY);
    for (size_t i = 0; i < all.size(); ++i) {
        PlotProfile* p = new PlotProfile();
        for (size_t j = 0; j < all[i].h.size(); ++j) p->addPoint(all[i].h[j], all[i].err[j]);
        PlotProfile::Properties prop;
        prop.pen.setColor(colors[i]);
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor(colors[i]);
        prop.symbolStyle = symbols[i];
        prop.symbolSize = 6;
        p->setProperties(prop);
        view.add(p);
        key.add(p, all[i].name);
        profiles.push_back(p);
    }
    view.add(&key);

    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << title << PlotStream::EndP();
    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "mesh spacing 1/n" << PlotStream::EndP();
    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "|approximate - exact|" << PlotStream::EndP();

    view.save(file);
    std::cout << "  wrote " << file << "\n";
    for (auto p : profiles) delete p;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    MidpointRule rectangle;
    TrapezoidRule trapezoid;
    SimpsonsRule simpson;

    Tanh tanhFn;
    const double exactTanh = std::log(std::cosh(1.0));

    GENFUNCTION sqrtCoth = Sqrt()(1.0 / Tanh());
    SubstitutedCoth substituted;
    const double s = std::sqrt(std::tanh(1.0));
    const double exactCoth = std::atanh(s) + std::atan(s);

    std::cout << std::setprecision(12);
    std::cout << "exact tanh integral       = " << exactTanh << "\n";
    std::cout << "exact sqrt(coth) integral = " << exactCoth << "\n";
    SimpleIntegrator check(0.0, 1.0, simpson, 4096);
    std::cout << "check: Simpson n=4096 on the substituted integrand = " << check(substituted) << "\n\n";

    std::cout << "integral of tanh(x) from 0 to 1:\n";
    std::vector<Series> tanhSeries = {
        convergence("rectangle (midpoint) rule", tanhFn, rectangle, exactTanh),
        convergence("trapezoid rule", tanhFn, trapezoid, exactTanh),
        convergence("Simpson's rule", tanhFn, simpson, exactTanh)};
    report(tanhSeries);
    plot(tanhSeries, "Integral of tanh(x) from 0 to 1", "tanh_convergence.png", 1e-15, 10.0, 1.2e-3, 3.0);

    std::cout << "\nintegral of sqrt(coth(x)) from 0 to 1:\n";
    std::vector<Series> cothSeries = {
        convergence("rectangle rule, original integrand", sqrtCoth, rectangle, exactCoth),
        convergence("rectangle rule, x = t^2", substituted, rectangle, exactCoth),
        convergence("trapezoid rule, x = t^2", substituted, trapezoid, exactCoth),
        convergence("Simpson's rule, x = t^2", substituted, simpson, exactCoth)};
    convergence("trapezoid rule, original integrand", sqrtCoth, trapezoid, exactCoth);
    convergence("Simpson's rule, original integrand", sqrtCoth, simpson, exactCoth);
    report(cothSeries);
    plot(cothSeries, "Integral of sqrt(coth(x)) from 0 to 1", "coth_convergence.png", 1e-18, 10.0, 0.002, 3e-14);

    return 0;
}