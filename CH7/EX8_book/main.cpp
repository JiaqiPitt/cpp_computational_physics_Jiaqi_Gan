#include <QStyle>
#include <QStyleHints>
#include <QGuiApplication>
#include <QApplication>
#include "QatGenericFunctions/Variable.h"
#include "QatGenericFunctions/Exp.h"
#include "QatGenericFunctions/Sqrt.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotProfile.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include <cmath>
#include <cstdio>
#include <numbers>
#include <random>
#include <vector>

using namespace Genfun;

// Atomic units: lengths in Bohr radii, energies in hartree.
// phi1 is centred at +z d/2, phi2 at -z d/2, psi = phi1 +- phi2.
const unsigned int NPOINTS = 200000;   // Monte Carlo points per value of d
const double       PI      = std::numbers::pi;

struct Result {
    double norm[2], normErr[2];   // N_s, N_a
    double energy[2], energyErr[2];   // total energy including 1/d
};

// Importance sampling: draw points from p(r) = (phi1^2 + phi2^2)/2, a 50/50
// mixture of two hydrogen 1s densities.  For each 1s density the radius has
// density 4 r^2 exp(-2r), a gamma distribution with shape 3 and scale 1/2,
// and the direction is uniform.
Result monteCarlo(double d) {
    std::mt19937 engine(12345);   // same random numbers for every d
    std::gamma_distribution<double> radius(3.0, 0.5);
    std::uniform_real_distribution<double> flat(0.0, 1.0);

    // running sums for s = +1 (symmetric) and s = -1 (antisymmetric)
    double sw[2] = {0, 0}, sww[2] = {0, 0}, sy[2] = {0, 0}, syy[2] = {0, 0}, swy[2] = {0, 0};

    for (unsigned int i = 0; i < NPOINTS; ++i) {
        double r = radius(engine);
        double cosT = 2.0 * flat(engine) - 1.0;
        double sinT = std::sqrt(1.0 - cosT * cosT);
        double ph = 2.0 * PI * flat(engine);
        double centre = flat(engine) < 0.5 ? d / 2.0 : -d / 2.0;
        double x = r * sinT * std::cos(ph), y = r * sinT * std::sin(ph), z = centre + r * cosT;

        double r1 = std::sqrt(x * x + y * y + (z - d / 2) * (z - d / 2));
        double r2 = std::sqrt(x * x + y * y + (z + d / 2) * (z + d / 2));
        double phi1 = std::exp(-r1) / std::sqrt(PI);
        double phi2 = std::exp(-r2) / std::sqrt(PI);
        double p = 0.5 * (phi1 * phi1 + phi2 * phi2);

        for (int k = 0; k < 2; ++k) {
            double s = (k == 0) ? 1.0 : -1.0;
            double psi = phi1 + s * phi2;
            // H phi1 = (-1/2 - 1/r2) phi1 and H phi2 = (-1/2 - 1/r1) phi2,
            // because each phi_i is the exact ground state of its own proton
            double Hpsi = -0.5 * psi - phi1 / r2 - s * phi2 / r1;
            double w  = psi * psi / p;    // estimates <psi|psi> = 2(1 +- S)
            double yv = psi * Hpsi / p;   // estimates <psi|H|psi>
            sw[k] += w;  sww[k] += w * w;
            sy[k] += yv; syy[k] += yv * yv; swy[k] += w * yv;
        }
    }

    Result res;
    for (int k = 0; k < 2; ++k) {
        double n = NPOINTS;
        double mw = sw[k] / n, my = sy[k] / n;
        double vw = sww[k] / n - mw * mw, vy = syy[k] / n - my * my, cwy = swy[k] / n - mw * my;
        // N = 1/sqrt(<psi|psi>)
        res.norm[k] = 1.0 / std::sqrt(mw);
        res.normErr[k] = 0.5 * std::pow(mw, -1.5) * std::sqrt(vw / n);
        // E = <psi|H|psi>/<psi|psi> + 1/d, error of a ratio of two means
        double e = my / mw;
        res.energy[k] = e + 1.0 / d;
        res.energyErr[k] = std::sqrt((vy - 2 * e * cwy + e * e * vw) / n) / mw;
    }
    return res;
}

void setTitles(PlotView& view, const char* title, const char* xLabel, const char* yLabel) {
    PlotStream titleStream(view.titleTextEdit());
    titleStream << PlotStream::Clear() << PlotStream::Center()
                << PlotStream::Family("Arial") << PlotStream::Size(16)
                << title << PlotStream::EndP();
    PlotStream xLabelStream(view.xLabelTextEdit());
    xLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << xLabel << PlotStream::EndP();
    PlotStream yLabelStream(view.yLabelTextEdit());
    yLabelStream << PlotStream::Clear() << PlotStream::Center()
                 << PlotStream::Family("Arial") << PlotStream::Size(14)
                 << yLabel << PlotStream::EndP();
}

PlotProfile::Properties pointStyle(const char* color, bool filled) {
    PlotProfile::Properties prop;
    prop.pen.setColor(color);
    prop.symbolSize = 7;
    if (filled) {
        prop.brush.setStyle(Qt::SolidPattern);
        prop.brush.setColor(color);
    }
    return prop;
}

PlotFunction1D::Properties lineStyle(const char* color, Qt::PenStyle style = Qt::SolidLine) {
    PlotFunction1D::Properties prop;
    prop.pen.setColor(color);
    prop.pen.setWidth(2);
    prop.pen.setStyle(style);
    return prop;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    // Analytic LCAO results, used only as a check on the Monte Carlo
    Variable D;
    Exp exp;
    Sqrt sqrt;
    GENFUNCTION S = exp(-D) * (1.0 + D + D * D / 3.0);          // overlap <phi1|phi2>
    GENFUNCTION J = 1.0 / D - exp(-2.0 * D) * (1.0 + 1.0 / D);   // <phi1|1/r2|phi1>
    GENFUNCTION K = exp(-D) * (1.0 + D);                         // <phi1|1/r1|phi2>
    GENFUNCTION Ns = 1.0 / sqrt(2.0 * (1.0 + S));
    GENFUNCTION Na = 1.0 / sqrt(2.0 * (1.0 - S));
    GENFUNCTION Es = -0.5 + 1.0 / D - (J + K) / (1.0 + S);
    GENFUNCTION Ea = -0.5 + 1.0 / D - (J - K) / (1.0 - S);

    PlotProfile normS, normA, energyS, energyA;
    std::printf("%5s %9s %8s %8s %9s %8s %8s %10s %8s %9s %10s %8s %9s\n", "d",
                "N_s MC", "+-", "exact", "N_a MC", "+-", "exact",
                "E_s MC", "+-", "exact", "E_a MC", "+-", "exact");
    double bestD = 0, bestE = 0;
    for (double d = 0.5; d <= 8.0001; d += 0.25) {
        Result r = monteCarlo(d);
        std::printf("%5.2f %9.5f %8.5f %8.5f %9.5f %8.5f %8.5f %10.5f %8.5f %9.5f %10.5f %8.5f %9.5f\n", d,
                    r.norm[0], r.normErr[0], Ns(d), r.norm[1], r.normErr[1], Na(d),
                    r.energy[0], r.energyErr[0], Es(d), r.energy[1], r.energyErr[1], Ea(d));
        normS.addPoint(QPointF(d, r.norm[0]), r.normErr[0], r.normErr[0]);
        normA.addPoint(QPointF(d, r.norm[1]), r.normErr[1], r.normErr[1]);
        energyS.addPoint(QPointF(d, r.energy[0]), r.energyErr[0], r.energyErr[0]);
        energyA.addPoint(QPointF(d, r.energy[1]), r.energyErr[1], r.energyErr[1]);
        if (r.energy[0] < bestE) { bestE = r.energy[0]; bestD = d; }
    }
    std::printf("\nlowest symmetric energy on the grid: E = %.4f hartree at d = %.2f\n", bestE, bestD);
    std::printf("binding energy relative to H + p (-0.5 hartree): %.4f hartree = %.2f eV\n",
                -0.5 - bestE, (-0.5 - bestE) * 27.2);

    // Plot (a): normalization factors
    {
        PRectF rect;
        rect.setXmin(0.0); rect.setXmax(8.5);
        rect.setYmin(0.0); rect.setYmax(4.0);
        PlotView view(rect);

        normS.setProperties(pointStyle("blue", true));
        normA.setProperties(pointStyle("red", false));
        PlotFunction1D pNs(Ns), pNa(Na);
        pNs.setProperties(lineStyle("blue"));
        pNa.setProperties(lineStyle("red"));
        view.add(&pNs);
        view.add(&pNa);
        view.add(&normS);
        view.add(&normA);

        PlotKey key(4.0, 3.8);
        key.add(&normS, "N_s, Monte Carlo");
        key.add(&pNs, "N_s = 1/sqrt(2(1+S)), analytic");
        key.add(&normA, "N_a, Monte Carlo");
        key.add(&pNa, "N_a = 1/sqrt(2(1-S)), analytic");
        view.add(&key);

        setTitles(view, "H2+ trial wavefunctions: normalization factors",
                  "proton separation d (Bohr radii)", "normalization factor");
        view.save("h2plus_normalization.png");
    }

    // Plot (b): energies
    {
        PRectF rect;
        rect.setXmin(0.0); rect.setXmax(8.5);
        rect.setYmin(-0.6); rect.setYmax(-0.1);
        PlotView view(rect);

        energyS.setProperties(pointStyle("blue", true));
        energyA.setProperties(pointStyle("red", false));
        PlotFunction1D pEs(Es), pEa(Ea);
        pEs.setProperties(lineStyle("blue"));
        pEa.setProperties(lineStyle("red"));
        GENFUNCTION separated = 0.0 * D - 0.5;
        PlotFunction1D pSep(separated);
        pSep.setProperties(lineStyle("black", Qt::DashLine));
        view.add(&pSep);
        view.add(&pEs);
        view.add(&pEa);
        view.add(&energyS);
        view.add(&energyA);

        PlotKey key(3.4, -0.115);
        key.add(&energyS, "symmetric psi_s, Monte Carlo");
        key.add(&pEs, "symmetric, analytic");
        key.add(&energyA, "antisymmetric psi_a, Monte Carlo");
        key.add(&pEa, "antisymmetric, analytic");
        key.add(&pSep, "H atom + proton far apart (-0.5)");
        view.add(&key);

        setTitles(view, "H2+ energy vs. proton separation (including 1/d)",
                  "proton separation d (Bohr radii)", "energy (hartree)");
        view.save("h2plus_energy.png");
    }

    std::printf("wrote h2plus_normalization.png and h2plus_energy.png\n");
    return 0;
}