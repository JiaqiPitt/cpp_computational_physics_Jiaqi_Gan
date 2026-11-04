// Exercise 11.9: a symplectic integrator for separable Hamiltonians
// (SymplecticIntegrator.h), and three tests of it.
//  1. Harmonic oscillator, the example of Section 11.5.2: the error at t = 10
//     against the exact solution for several step sizes h gives the order.
//  2. Pendulum at large amplitude, H = p^2/2 - cos q: the energy over a long
//     time, next to classical Runge-Kutta with the same fixed step (Qat's
//     RungeKuttaSolver with a SimpleRKStepper).
//  3. Kepler problem in the plane, H = (px^2 + py^2)/2 - 1/r, two degrees of
//     freedom: many orbits with leapfrog and with Runge-Kutta.
#include "SymplecticIntegrator.h"
#include "QatGenericFunctions/ButcherTableau.h"
#include "QatGenericFunctions/Cos.h"
#include "QatGenericFunctions/Sin.h"
#include "QatGenericFunctions/RungeKuttaClassicalSolver.h"
#include "QatGenericFunctions/SimpleRKStepper.h"
#include "QatGenericFunctions/Sqrt.h"
#include "QatGenericFunctions/Variable.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotOrbit.h"
#include "QatPlotting/PlotProfile.h"
#include "QatPlotting/PlotStream.h"
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleHints>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Genfun;
using Classical::PhaseSpace;

const SymplecticIntegrator::Method METHODS[3] = {SymplecticIntegrator::EULER, SymplecticIntegrator::LEAPFROG,
                                                 SymplecticIntegrator::YOSHIDA};
const char* NAMES[3] = {"symplectic Euler", "leapfrog", "Yoshida (order 4)"};
const char* COLOURS[3] = {"darkorange", "blue", "darkgreen"};

void labels(PlotView& view, const std::string& title, const std::string& x, const std::string& y) {
    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                << PlotStream::Size(16) << title << PlotStream::EndP();
    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                 << PlotStream::Size(14) << x << PlotStream::EndP();
    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center() << PlotStream::Family("Arial")
                 << PlotStream::Size(14) << y << PlotStream::EndP();
}

void style(PlotProfile& profile, const char* colour, int size) {
    PlotProfile::Properties prop;
    prop.pen.setColor(colour);
    prop.brush.setStyle(Qt::SolidPattern);
    prop.brush.setColor(colour);
    prop.symbolSize = size;
    profile.setProperties(prop);
}

// 1. Harmonic oscillator, m = k = 1, q(0) = 0, p(0) = 1: q = sin t, p = cos t
void orderTest() {
    PhaseSpace space(1);
    const PhaseSpace::Component &q = space.coordinates(), &p = space.momenta();
    space.start(q[0], 0.0);
    space.start(p[0], 1.0);
    GENFUNCTION H = 0.5 * p[0] * p[0] + 0.5 * q[0] * q[0];
    const double T = 10.0;
    const std::vector<double> steps = {0.2, 0.1, 0.05, 0.025, 0.0125, 0.00625};

    std::printf("1. Harmonic oscillator: error at t = %.0f\n       h", T);
    for (const char* name : NAMES) std::printf("   %18s", name);
    std::printf("\n");
    std::vector<double> error[3];
    PlotProfile points[3];
    for (int m = 0; m < 3; ++m) {
        for (double h : steps) {
            SymplecticIntegrator s(H, space, METHODS[m]);
            for (long i = 0, n = std::lround(T / h); i < n; ++i) s.step(h);
            error[m].push_back(std::hypot(s.q(0) - std::sin(T), s.p(0) - std::cos(T)));
            points[m].addPoint(h, error[m].back());
        }
        style(points[m], COLOURS[m], 8);
    }
    for (size_t k = 0; k < steps.size(); ++k) {
        std::printf("%8.5f", steps[k]);
        for (int m = 0; m < 3; ++m) std::printf("   %18.3e", error[m][k]);
        std::printf("\n");
    }
    std::printf("   order");
    for (int m = 0; m < 3; ++m) {
        size_t k = steps.size() - 1;
        std::printf("   %18.2f", std::log2(error[m][k - 1] / error[m][k]));
    }
    std::printf("   (log2 of the ratio of the last two errors)\n\n");

    PRectF rect;
    rect.setXmin(0.004);
    rect.setXmax(0.3);
    rect.setYmin(1e-11);
    rect.setYmax(100.0);
    PlotView view(rect, PlotView::LOG, PlotView::LOG);
    PlotKey key(0.005, 30.0);
    for (int m = 0; m < 3; ++m) {
        view.add(&points[m]);
        key.add(&points[m], NAMES[m]);
    }
    view.add(&key);
    labels(view, "Harmonic oscillator, error at t = 10", "step h", "error");
    view.save("sep_order.png");
}

// 2. Pendulum, H = p^2/2 - cos q, let go from q = 2 rad (115 deg).  The
// plot shows the largest energy error up to each time: it levels off for a
// symplectic method and keeps growing for Runge-Kutta.
void energyTest(double h, double T) {
    PhaseSpace space(1);
    const PhaseSpace::Component &q = space.coordinates(), &p = space.momenta();
    space.start(q[0], 2.0);
    space.start(p[0], 0.0);
    GENFUNCTION H = 0.5 * p[0] * p[0] - Cos()(q[0]);
    double E0 = -std::cos(2.0);

    // sample at t = 1, 1.26, 1.58, 2, ... T (ten times a decade)
    std::vector<long> at;
    for (int k = 0; std::pow(10.0, k / 10.0) <= T * 1.000001; ++k) {
        long n = std::lround(std::pow(10.0, k / 10.0) / h);
        if (at.empty() || n > at.back()) at.push_back(n);
    }
    // run a method, keeping the largest error so far at the sample times;
    // energyAfter(i) is the energy after step i
    auto run = [&](PlotProfile& points, auto energyAfter) {
        double worst = 0;
        long done = 0;
        for (long n : at) {
            for (; done < n; ++done) worst = std::max(worst, std::fabs(energyAfter(done + 1) - E0) / std::fabs(E0));
            points.addPoint(n * h, worst);
        }
        return worst;
    };
    PlotProfile points[4];
    std::printf("2. Pendulum from q = 2 rad, h = %g: largest |E - E0| / |E0| up to t = %g\n", h, T);
    for (int m = 0; m < 3; ++m) {
        SymplecticIntegrator s(H, space, METHODS[m]);
        double worst = run(points[m], [&](long) { s.step(h); return s.energy(); });
        std::printf("  %-20s %.2e\n", NAMES[m], worst);
        style(points[m], COLOURS[m], 6);
    }
    // classical Runge-Kutta with the same fixed step, from Qat
    SimpleRKStepper stepper(ClassicalRungeKuttaTableau(), h);
    Classical::RungeKuttaSolver solver(H, space, &stepper);
    GENFUNCTION E = solver.energy();
    double worst = run(points[3], [&](long i) { return E(i * h); });
    std::printf("  %-20s %.2e\n", "Runge-Kutta 4", worst);
    style(points[3], "red", 6);
    std::printf("\n");

    PRectF rect;
    rect.setXmin(0.8);
    rect.setXmax(1.25 * T);
    rect.setYmin(1e-8);
    rect.setYmax(100.0);
    PlotView view(rect, PlotView::LOG, PlotView::LOG);
    PlotKey key(1.0, 50.0);
    for (int m = 0; m < 4; ++m) {
        view.add(&points[m]);
        key.add(&points[m], m < 3 ? NAMES[m] : "classical Runge-Kutta (not symplectic)");
    }
    view.add(&key);
    char title[100];
    std::snprintf(title, sizeof title, "Pendulum, h = %g: largest |E - E0| / |E0| up to time t", h);
    labels(view, title, "t", "");
    view.save("sep_energy.png");
}

// 3. Kepler problem: from x = 1, y = 0 with px = 0, py = 1.2.  The exact
// orbit is an ellipse with a focus at the origin; the plot shows it next to
// the last of many turns with leapfrog and with Runge-Kutta.
void keplerTest(double h, int turns) {
    PhaseSpace space(2);
    const PhaseSpace::Component &q = space.coordinates(), &p = space.momenta();
    space.start(q[0], 1.0);
    space.start(q[1], 0.0);
    space.start(p[0], 0.0);
    space.start(p[1], 1.2);
    GENFUNCTION H = 0.5 * (p[0] * p[0] + p[1] * p[1]) - 1.0 / Sqrt()(q[0] * q[0] + q[1] * q[1]);
    double E0 = 0.5 * 1.2 * 1.2 - 1.0, L0 = 1.2;
    double e = std::sqrt(1 + 2 * E0 * L0 * L0), a = -1 / (2 * E0), period = 2 * M_PI * std::pow(a, 1.5);
    long n = std::lround(turns * period / h), last = std::lround((turns - 1) * period / h);

    SymplecticIntegrator s(H, space, SymplecticIntegrator::LEAPFROG);
    SimpleRKStepper stepper(ClassicalRungeKuttaTableau(), h);
    Classical::RungeKuttaSolver solver(H, space, &stepper);
    GENFUNCTION x = solver.equationOf(q[0]), y = solver.equationOf(q[1]);
    GENFUNCTION px = solver.equationOf(p[0]), py = solver.equationOf(p[1]);
    PlotProfile leap, rk;
    for (long i = 1; i <= n; ++i) {
        s.step(h);
        double xr = x(i * h), yr = y(i * h);
        if (i > last) {
            leap.addPoint(s.q(0), s.q(1));
            rk.addPoint(xr, yr);
        }
    }
    double t = n * h;
    double Ls = s.q(0) * s.p(1) - s.q(1) * s.p(0), Lr = x(t) * py(t) - y(t) * px(t);
    double Er = 0.5 * (px(t) * px(t) + py(t) * py(t)) - 1 / std::hypot(x(t), y(t));
    std::printf("3. Kepler orbit, e = %.2f, period %.3f, h = %g (%.0f steps a turn), %d turns\n",
                e, period, h, period / h, turns);
    std::printf("  %-20s E - E0 = %+.2e   L - L0 = %+.2e\n", "leapfrog", s.energy() - E0, Ls - L0);
    std::printf("  %-20s E - E0 = %+.2e   L - L0 = %+.2e\n\n", "Runge-Kutta 4", Er - E0, Lr - L0);
    style(leap, "blue", 6);
    style(rk, "red", 6);

    // the exact ellipse, r = L0^2 / (1 + e cos theta)
    Variable theta;
    GENFUNCTION r = L0 * L0 / (1.0 + e * Cos()(theta));
    PlotOrbit exact(r * Cos()(theta), r * Sin()(theta), 0.0, 2 * M_PI);
    PlotOrbit::Properties prop;
    prop.pen.setColor("black");
    prop.pen.setWidth(2);
    exact.setProperties(prop);

    PRectF rect;
    rect.setXmin(-3.2);
    rect.setXmax(3.2);
    rect.setYmin(-3.0);
    rect.setYmax(4.2);
    PlotView view(rect);
    PlotKey key(-3.0, 4.0);
    view.add(&exact);
    view.add(&rk);
    view.add(&leap);
    key.add(&exact, "exact orbit");
    char name[2][80];
    std::snprintf(name[0], sizeof name[0], "leapfrog, turn %d", turns);
    std::snprintf(name[1], sizeof name[1], "classical Runge-Kutta, turn %d", turns);
    key.add(&leap, name[0]);
    key.add(&rk, name[1]);
    view.add(&key);
    char title[100];
    std::snprintf(title, sizeof title, "Kepler orbit, h = %g (%.0f steps a turn)", h, period / h);
    labels(view, title, "x", "y");
    view.save("sep_kepler.png");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    // 0. A Hamiltonian that is not separable is refused
    {
        PhaseSpace space(1);
        const PhaseSpace::Component &q = space.coordinates(), &p = space.momenta();
        space.start(q[0], 1.0);
        space.start(p[0], 1.0);
        try {
            SymplecticIntegrator s(0.5 * p[0] * p[0] * (1.0 + q[0] * q[0]), space);
            std::printf("0. H = p^2 (1 + q^2) / 2 was accepted, which it should not be\n\n");
        } catch (const std::invalid_argument& error) {
            std::printf("0. H = p^2 (1 + q^2) / 2 is refused: %s\n\n", error.what());
        }
    }
    orderTest();
    energyTest(argc > 1 ? std::atof(argv[1]) : 0.1, argc > 2 ? std::atof(argv[2]) : 1e4);
    keplerTest(argc > 3 ? std::atof(argv[3]) : 0.4, argc > 4 ? std::atoi(argv[4]) : 100);
    std::printf("wrote sep_order.png, sep_energy.png, sep_kepler.png\n");
    return 0;
}
