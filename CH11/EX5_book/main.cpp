// Exercise 11.5: the heavy symmetric top.
//
// With u = cos(theta), the equations on p. 299 are
//   du/dt   = sqrt(f(u)),   f(u) = (1 - u^2)(alpha - beta u) - (b - a u)^2
//   dphi/dt = (b - a u) / (1 - u^2)
//   dpsi/dt = I1 a / I3 - u (b - a u) / (1 - u^2)
// The square root only gives |du/dt|.  u swings between two roots of f, and
// at each of them du/dt has to change sign, which the first equation can't
// do.  Differentiating (du/dt)^2 = f(u) gives
//   d^2u/dt^2 = f'(u) / 2,
// which has no sign problem, so u and w = du/dt are integrated instead, with
// f'(u) from the automatic derivative of QatGenericFunctions.  (du/dt)^2 = f(u)
// is then a check on the integration.
#include "QatGenericFunctions/ACos.h"
#include "QatGenericFunctions/AdaptiveRKStepper.h"
#include "QatGenericFunctions/Argument.h"
#include "QatGenericFunctions/RKIntegrator.h"
#include "QatGenericFunctions/Variable.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotOrbit.h"
#include "QatPlotting/PlotStream.h"
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleHints>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace Genfun;

// A toy top, SI units: mass, distance from the tip to the centre of mass, and
// the moments of inertia about the tip (I1 = I2 across the axis, I3 along it)
const double M = 0.1, L = 0.04, G = 9.81;
const double I1 = 1.6e-4, I3 = 6.0e-5;
const double SPIN = 150.0;         // psi' at the start (rad/s), about 24 turns a second
const double T_END = 1.0;          // s
const double DEG = 180.0 / M_PI;

// One run.  The top starts at angle theta0 from the vertical with theta' = 0,
// so u starts at a turning point, and it is given a precession rate phidot0.
struct Run {
    std::string name, colour;
    double theta0, phidot0;
    double a, b, alpha, beta;                  // the constants of pp. 298-299
    std::unique_ptr<RKIntegrator> integrator;
    const Genfun::AbsFunction *u, *w, *phi, *psi;   // the solution, as functions of t
};

// a, b and E from Eqs. 11.11-11.13 at t = 0, then alpha and beta
void setConstants(Run& r) {
    double u0 = std::cos(r.theta0), s2 = 1 - u0 * u0;
    double w3 = r.phidot0 * u0 + SPIN;                  // spin about the symmetry axis
    r.a = I3 * w3 / I1;                                  // Eq. 11.11
    r.b = r.phidot0 * s2 + r.a * u0;                     // Eq. 11.12, using I3 w3 = I1 a
    double E = 0.5 * I1 * r.phidot0 * r.phidot0 * s2 + 0.5 * I3 * w3 * w3 + M * G * L * u0;   // Eq. 11.13
    r.alpha = 2 * E / I1 - I1 / I3 * r.a * r.a;
    r.beta = 2 * M * G * L / I1;
}

void solve(Run& r) {
    Variable U(0, 4), W(1, 4), PHI(2, 4), PSI(3, 4);
    GENFUNCTION f = (1.0 - U * U) * (r.alpha - r.beta * U) - (r.b - r.a * U) * (r.b - r.a * U);
    GENFUNCTION fPrime = f.partial(U);
    GENFUNCTION dWdt = 0.5 * fPrime;
    GENFUNCTION dPhidt = (r.b - r.a * U) / (1.0 - U * U);
    GENFUNCTION dPsidt = I1 * r.a / I3 - U * (r.b - r.a * U) / (1.0 - U * U);

    AdaptiveRKStepper stepper;
    stepper.tolerance() = 1e-10;
    double u0 = std::cos(r.theta0);
    r.integrator = std::make_unique<RKIntegrator>(&stepper);
    r.integrator->addDiffEquation(&W, "U", u0);            // du/dt = w
    r.integrator->addDiffEquation(&dWdt, "W", 0.0);         // starts at a turning point
    r.integrator->addDiffEquation(&dPhidt, "PHI", 0.0);
    r.integrator->addDiffEquation(&dPsidt, "PSI", 0.0);
    r.u = r.integrator->getFunction(U);
    r.w = r.integrator->getFunction(W);
    r.phi = r.integrator->getFunction(PHI);
    r.psi = r.integrator->getFunction(PSI);

    // The other root of f: u0 is one, and u moves off towards the side where
    // f is positive.  Walk that way until f turns negative, then bisect.
    Argument x(4);
    x[0] = u0;
    double dir = fPrime(x) < 0 ? -1.0 : 1.0;
    auto fAt = [&](double u) { x[0] = u; return f(x); };
    double inside = u0 + 1e-6 * dir, outside = inside;
    while (fAt(outside) > 0 && std::fabs(outside) < 1) outside += 1e-3 * dir;
    for (int i = 0; i < 60; ++i) {
        double mid = 0.5 * (inside + outside);
        (fAt(mid) > 0 ? inside : outside) = mid;
    }
    double root = 0.5 * (inside + outside);
    double u1 = std::min(u0, root), u2 = std::max(u0, root);

    // Exact nutation period and mean precession rate, for comparison.  f is a
    // cubic, f = beta (u - u1)(u - u2)(u - u3), with u1 + u2 + u3 = (alpha + a^2)/beta.
    // With u = u1 + (u2 - u1) sin^2(s), du / sqrt(f) = 2 ds / sqrt(beta (u3 - u)),
    // which has no end-point singularity:
    //   period     = 2 int_u1^u2 du / sqrt(f)
    //   precession = (2 / period) int_u1^u2 (b - a u) / (1 - u^2) du / sqrt(f)
    double u3 = (r.alpha + r.a * r.a) / r.beta - u1 - u2;
    double period = 0, phiGain = 0;
    const int S = 2000;
    for (int i = 0; i < S; ++i) {
        double s = (i + 0.5) / S * M_PI / 2, u = u1 + (u2 - u1) * std::sin(s) * std::sin(s);
        double dt = 4 / std::sqrt(r.beta * (u3 - u)) * (M_PI / 2 / S);   // dt over this slice of s, both halves
        period += dt;
        phiGain += (r.b - r.a * u) / (1 - u * u) * dt;
    }

    // Sample the solution: range of u, (du/dt)^2 - f(u), and the times where
    // w changes sign (the turning points, interpolated between samples)
    const int N = 20000;
    double umin = u0, umax = u0, worst = 0, lastT = 0, lastW = 0;
    std::vector<double> turns;
    for (int i = 1; i <= N; ++i) {
        double t = T_END * i / N, ut = (*r.u)(t), wt = (*r.w)(t);
        umin = std::min(umin, ut);
        umax = std::max(umax, ut);
        worst = std::max(worst, std::fabs(wt * wt - fAt(ut)));
        if (i > 1 && wt * lastW < 0) turns.push_back(lastT + (t - lastT) * lastW / (lastW - wt));
        lastT = t;
        lastW = wt;
    }
    // over whole nutation periods: from the first turning point to the last one of the same kind
    size_t k = (turns.size() - 1) / 2 * 2;
    double measuredPeriod = (turns[k] - turns[0]) / double(k / 2);
    double measuredRate = ((*r.phi)(turns[k]) - (*r.phi)(turns[0])) / (turns[k] - turns[0]);

    std::printf("%s\n", r.name.c_str());
    std::printf("  start: theta = %.0f deg, phi' = %+.1f rad/s, psi' = %.0f rad/s\n", r.theta0 * DEG, r.phidot0, SPIN);
    std::printf("  a = %.3f /s, b = %.3f /s, alpha = %.3f /s^2, beta = %.3f /s^2\n", r.a, r.b, r.alpha, r.beta);
    std::printf("  turning points (roots of f): u = %.6f and %.6f, theta = %.2f and %.2f deg\n",
                u1, u2, std::acos(u1) * DEG, std::acos(u2) * DEG);
    std::printf("  u from the integration stays between %.6f and %.6f\n", umin, umax);
    std::printf("  largest |(du/dt)^2 - f(u)| = %.1e /s^2\n", worst);
    std::printf("  nutation period:  %.6f s from the solution, %.6f s from the integral\n", measuredPeriod, period);
    std::printf("  mean precession:  %.5f rad/s from the solution, %.5f rad/s from the integral\n\n",
                measuredRate, phiGain / period);
}

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

// One of u, phi, psi against time for all the runs
void plotVsTime(const std::vector<Run>& runs, const Genfun::AbsFunction* Run::*which, double ymin, double ymax,
                const std::string& title, const std::string& ylabel, const std::string& file) {
    PRectF rect;
    rect.setXmin(0.0);
    rect.setXmax(T_END);
    rect.setYmin(ymin);
    rect.setYmax(ymax);
    PlotView view(rect);
    std::vector<PlotFunction1D> curves;
    curves.reserve(runs.size());
    PlotKey key(0.03 * T_END, ymax - 0.05 * (ymax - ymin));
    for (const Run& r : runs) {
        curves.emplace_back(*(r.*which));
        PlotFunction1D::Properties prop;
        prop.pen.setColor(r.colour.c_str());
        prop.pen.setWidth(2);
        curves.back().setProperties(prop);
    }
    for (size_t k = 0; k < runs.size(); ++k) {
        view.add(&curves[k]);
        key.add(&curves[k], runs[k].name);
    }
    view.add(&key);
    labels(view, title, "t (s)", ylabel);
    view.save(file);
}

// Where the symmetry axis points: azimuth phi and height above the horizontal
void plotAxis(const std::vector<Run>& runs, const std::string& file) {
    double phiMax = 0;
    for (const Run& r : runs) phiMax = std::max(phiMax, (*r.phi)(T_END) * DEG);
    PRectF rect;
    rect.setXmin(0.0);
    rect.setXmax(1.05 * phiMax);
    rect.setYmin(0.0);
    rect.setYmax(75.0);
    PlotView view(rect);
    std::vector<PlotOrbit> paths;
    paths.reserve(runs.size());
    PlotKey key(0.03 * phiMax, 72.0);
    for (const Run& r : runs) {
        GENFUNCTION azimuth = DEG * (*r.phi);
        GENFUNCTION height = 90.0 - DEG * ACos()(*r.u);
        paths.emplace_back(azimuth, height, 0.0, T_END);
        PlotOrbit::Properties prop;
        prop.pen.setColor(r.colour.c_str());
        prop.pen.setWidth(2);
        paths.back().setProperties(prop);
    }
    for (size_t k = 0; k < runs.size(); ++k) {
        view.add(&paths[k]);
        key.add(&paths[k], runs[k].name);
    }
    view.add(&key);
    labels(view, "Path of the symmetry axis", "azimuth phi (degrees)", "height of the axis above horizontal, 90 - theta (degrees)");
    view.save(file);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    std::vector<Run> runs;
    runs.push_back({"let go at rest (cusps)", "red", 30 / DEG, 0.0});
    runs.push_back({"pushed with the precession (waves)", "blue", 50 / DEG, 2.0});
    runs.push_back({"pushed against it (loops)", "darkgreen", 70 / DEG, -2.0});
    for (Run& r : runs) {
        setConstants(r);
        solve(r);
    }

    double psiMax = 0;
    for (const Run& r : runs) psiMax = std::max(psiMax, (*r.psi)(T_END));
    plotVsTime(runs, &Run::u, 0.0, 1.2, "u = cos(theta)", "u", "top_u.png");
    plotVsTime(runs, &Run::phi, -0.5, 5.0, "Precession angle phi", "phi (rad)", "top_phi.png");
    plotVsTime(runs, &Run::psi, 0.0, 1.1 * psiMax, "Spin angle psi", "psi (rad)", "top_psi.png");
    plotAxis(runs, "top_axis.png");
    std::printf("wrote top_u.png, top_phi.png, top_psi.png, top_axis.png\n");
    return 0;
}
