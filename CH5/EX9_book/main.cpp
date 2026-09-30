#include <QStyle>
#include <QStyleHints>
#include <QGuiApplication>
#include <QApplication>
#include "QatGenericFunctions/AbsFunction.h"
#include "QatGenericFunctions/Variable.h"
#include "QatGenericFunctions/Sqrt.h"
#include "QatGenericFunctions/Cos.h"
#include "QatGenericFunctions/Sin.h"
#include "QatGenericFunctions/FixedConstant.h"
#include "QatGenericFunctions/RootFinder.h"
#include "QatGenericFunctions/SimpleIntegrator.h"
#include "QatGenericFunctions/QuadratureRule.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotText.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include "QatPlotting/RealArg.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace Genfun;

// Units: MeV and fm
const double HBARC = 197.3269804;               // MeV fm
const double MC    = 1370.0;                    // charm quark mass, MeV/c^2
const double MU    = MC / 2.0;                  // reduced mass, MeV/c^2
const double ALPHA = 0.38;
const double A     = 2.43e-3;                   // MeV^-1
const double K     = 4.0 / 3.0 * ALPHA * HBARC; // Coulomb-like strength, MeV fm
const double S     = 1.0 / (HBARC * A * A);     // string tension, MeV / fm

//   (1 / (pi hbar c)) * integral_{r1}^{r2} sqrt(2 mu c^2 (E - Veff(r))) dr
// with Veff = V(r) + (hbar c)^2 L2 / (2 mu c^2 r^2).
// Eq. 5.16 then reads  action(E) = n + 1/2.
class WKBAction : public AbsFunction {
    FUNCTION_OBJECT_DEF(WKBAction)
public:
    explicit WKBAction(double L2) : _L2(L2) {}
    WKBAction(const WKBAction& right) : AbsFunction(right), _L2(right._L2) {}
    virtual double operator() (double E) const;
    virtual double operator() (const Argument& a) const {return operator()(a[0]);}
private:
    double _L2;   // l(l+1), or (l+1/2)^2 with the Langer correction
};
FUNCTION_OBJECT_IMP(WKBAction)

double WKBAction::operator() (double E) const {
    Variable R;
    double C = HBARC * HBARC * _L2 / (2.0 * MU);
    GENFUNCTION Veff = -K / R + S * R + C / (R * R);

    double r1 = 0.0, r2;
    if (C > 0) {
        // Veff has a single minimum; the turning points sit on either side of it
        GENFUNCTION slope = K / (R * R) + S - 2.0 * C / (R * R * R);
        Bisection findMin(slope);
        findMin.lowerBound() = 1e-6;
        findMin.upperBound() = 50.0;
        double rmin = findMin.root(1.0);
        if (E <= Veff(rmin)) return 0.0;

        GENFUNCTION gap = E - Veff;
        Bisection inner(gap), outer(gap);
        inner.lowerBound() = 1e-8; inner.upperBound() = rmin;
        outer.lowerBound() = rmin; outer.upperBound() = 50.0;
        r1 = inner.root(rmin / 2);
        r2 = outer.root(2 * rmin);
    } else {
        // l = 0 without the Langer term: the inner turning point is r = 0
        GENFUNCTION gap = E - Veff;
        Bisection outer(gap);
        outer.lowerBound() = 1e-12; outer.upperBound() = 50.0;
        r2 = outer.root(1.0);
    }

    // r = r1 + (r2 - r1)(1 - cos t)/2 removes the square-root endpoints
    Variable T;
    Cos cos;
    Sin sin;
    Sqrt sqrt;
    GENFUNCTION r = r1 + (r2 - r1) * (1.0 - cos(T)) / 2.0;
    GENFUNCTION drdt = (r2 - r1) * sin(T) / 2.0;
    GENFUNCTION integrand = sqrt(2.0 * MU * (E - Veff(r))) * drdt;

    MidpointRule rule;
    SimpleIntegrator integrator(0.0, M_PI, rule, 2000);
    return integrator(integrand) / (M_PI * HBARC);
}

// Energy E_n with action(E) = n + 1/2   (Eq. 5.16)
double wkbEnergy(unsigned int n, double L2) {
    WKBAction action(L2);
    GENFUNCTION f = action - (n + 0.5);
    Bisection find(f, 1e-9, 200);
    find.lowerBound() = -2000.0;
    find.upperBound() = 4000.0;
    return find.root(0.0);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    std::printf("V(r) = -%.4f MeV fm / r + %.3f MeV/fm * r,  mu = %.0f MeV\n\n", K, S, MU);

    const char* sName[3] = {"1S", "2S", "3S"};
    const char* pName[3] = {"1P", "2P", "3P"};
    std::vector<double> ES, EP;

    std::printf("%5s %5s %16s %16s %14s\n", "state", "l", "E Langer (MeV)", "E l(l+1) (MeV)", "mass (MeV)");
    for (unsigned int n = 0; n < 3; ++n) {
        ES.push_back(wkbEnergy(n, 0.25));
        std::printf("%5s %5d %16.2f %16.2f %14.1f\n", sName[n], 0, ES[n], wkbEnergy(n, 0.0), 2 * MC + ES[n]);
    }
    for (unsigned int n = 0; n < 3; ++n) {
        EP.push_back(wkbEnergy(n, 2.25));
        std::printf("%5s %5d %16.2f %16.2f %14.1f\n", pName[n], 1, EP[n], wkbEnergy(n, 2.0), 2 * MC + EP[n]);
    }

    // Energy level diagram: S states on the left, P states on the right
    PRectF rect;
    rect.setXmin(0.0);
    rect.setXmax(4.0);
    rect.setYmin(2900.0);
    rect.setYmax(4500.0);
    PlotView view(rect);

    std::vector<std::unique_ptr<FixedConstant>> levels;
    std::vector<std::unique_ptr<PlotFunction1D>> lines;
    std::vector<std::unique_ptr<PlotText>> labels;

    auto addLevel = [&](double x0, double x1, double E, const char* name, const char* color) {
        double M = 2 * MC + E;
        levels.push_back(std::make_unique<FixedConstant>(M));
        lines.push_back(std::make_unique<PlotFunction1D>(*levels.back(), RealArg::Gt(x0) && RealArg::Lt(x1)));
        PlotFunction1D::Properties prop;
        prop.pen.setColor(color);
        prop.pen.setWidth(3);
        lines.back()->setProperties(prop);
        view.add(lines.back().get());
        labels.push_back(std::make_unique<PlotText>(x1 + 0.05, M + 55.0,
                         QString("%1  %2 MeV").arg(name).arg(M, 0, 'f', 1)));
        view.add(labels.back().get());
    };
    for (int n = 0; n < 3; ++n) addLevel(0.3, 1.2, ES[n], sName[n], "blue");
    for (int n = 0; n < 3; ++n) addLevel(1.9, 2.8, EP[n], pName[n], "red");

    PlotKey key(0.1, 4430.0);
    key.add(lines[0].get(), "S states (l = 0)");
    key.add(lines[3].get(), "P states (l = 1)");
    view.add(&key);

    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << "Charmonium levels from WKB (Langer-corrected Eq. 5.16)"
                << PlotStream::EndP();

    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "S waves (left)          P waves (right)"
                 << PlotStream::EndP();

    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << "mass  2 m_c c^2 + E  (MeV)"
                 << PlotStream::EndP();

    view.save("charmonium_levels.png");
    std::printf("\nwrote charmonium_levels.png\n");
    return 0;
}