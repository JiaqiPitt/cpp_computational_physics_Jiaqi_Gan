// Exercise 11.15: the SEIR disease model of Fig. 11.12,
//   dS/dt = b N - lambda S - d S + nu R
//   dE/dt = lambda S - (alpha + d) E
//   dI/dt = alpha E - (gamma + d) I
//   dR/dt = gamma I - (nu + d) R
// with the force of infection lambda = beta I / N.  Rates are per day and
// N = 1, so S, E, I and R are fractions of the population.
#include "QatGenericFunctions/AdaptiveRKStepper.h"
#include "QatGenericFunctions/FixedConstant.h"
#include "QatGenericFunctions/RKIntegrator.h"
#include "QatGenericFunctions/Variable.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include "QatPlotting/PlotKey.h"
#include "QatPlotting/PlotStream.h"
#include <QApplication>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleHints>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace Genfun;

const double BETA = 0.2, GAMMA = 0.1, ALPHA = 0.1, NU = 1e-3;   // per day
const double B = 1e-4, D = 1e-4;                                  // births and deaths, per day
const double I0 = 1e-6;
const double T_PLOT = 6000.0, T_END = 10000.0;                    // days

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

void line(PlotFunction1D& curve, const char* colour, Qt::PenStyle dash = Qt::SolidLine) {
    PlotFunction1D::Properties prop;
    prop.pen.setColor(colour);
    prop.pen.setWidth(2);
    prop.pen.setStyle(dash);
    curve.setProperties(prop);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());

    Variable S(0, 4), E(1, 4), I(2, 4), R(3, 4);
    GENFUNCTION N = S + E + I + R;
    GENFUNCTION lambda = BETA * I / N;
    GENFUNCTION dSdt = B * N - lambda * S - D * S + NU * R;
    GENFUNCTION dEdt = lambda * S - (ALPHA + D) * E;
    GENFUNCTION dIdt = ALPHA * E - (GAMMA + D) * I;
    GENFUNCTION dRdt = GAMMA * I - (NU + D) * R;

    AdaptiveRKStepper stepper;
    stepper.tolerance() = 1e-10;
    RKIntegrator integrator(&stepper);
    integrator.addDiffEquation(&dSdt, "S", 1 - I0);
    integrator.addDiffEquation(&dEdt, "E", 0.0);
    integrator.addDiffEquation(&dIdt, "I", I0);
    integrator.addDiffEquation(&dRdt, "R", 0.0);
    GENFUNCTION s = *integrator.getFunction(S), e = *integrator.getFunction(E);
    GENFUNCTION i = *integrator.getFunction(I), r = *integrator.getFunction(R);

    // What to expect.  Basic reproduction number: a case stays exposed for
    // 1/(alpha + d), of which a fraction alpha/(alpha + d) becomes infectious,
    // and infects beta people a day for 1/(gamma + d) days.  Early on S = N,
    // and E, I grow like exp(k t) with (k + alpha + d)(k + gamma + d) = alpha beta.
    // At the endemic equilibrium S = N/R0, alpha E = (gamma + d) I,
    // (nu + d) R = gamma I, and S + E + I + R = N fixes I.
    double R0 = BETA * ALPHA / ((ALPHA + D) * (GAMMA + D));
    double sum = ALPHA + GAMMA + 2 * D, prod = (ALPHA + D) * (GAMMA + D) - ALPHA * BETA;
    double k = (-sum + std::sqrt(sum * sum - 4 * prod)) / 2;
    double Sx = 1 / R0, Ix = (1 - Sx) / ((GAMMA + D) / ALPHA + 1 + GAMMA / (NU + D));
    double Ex = (GAMMA + D) / ALPHA * Ix, Rx = GAMMA / (NU + D) * Ix;

    std::printf("basic reproduction number R0 = %.4f\n", R0);
    std::printf("early growth rate: %.5f /day expected, %.5f /day from I between days 50 and 150 (doubles in %.1f days)\n\n",
                k, std::log(i(150.0) / i(50.0)) / 100.0, std::log(2.0) / k);

    // highs and lows of I, day by day (the first ten are printed), and how
    // well N stays at 1
    std::printf("highs and lows of I:\n     day           I        S\n");
    double worstN = 0, before = I0, now = i(1.0);
    std::vector<double> highs;
    int shown = 0;
    for (double t = 2; t <= T_END; t += 1) {
        double next = i(t);
        if ((now > before && now > next) || (now < before && now < next)) {
            if (shown++ < 10) std::printf("  %6.0f   %.3e  %.4f   %s\n", t - 1, now, s(t - 1), now > next ? "high" : "low");
            if (now > next) highs.push_back(t - 1);
        }
        worstN = std::max(worstN, std::fabs(s(t) + e(t) + i(t) + r(t) - 1));
        before = now;
        now = next;
    }
    std::printf("  ... %zu highs in %.0f days; the last two are %.0f days apart\n", highs.size(), T_END,
                highs[highs.size() - 1] - highs[highs.size() - 2]);
    std::printf("\nlargest |N - 1| over %.0f days: %.1e\n", T_END, worstN);
    std::printf("day %.0f:     S = %.5f  E = %.4e  I = %.4e  R = %.5f\n", T_END, s(T_END), e(T_END), i(T_END), r(T_END));
    std::printf("equilibrium: S = %.5f  E = %.4e  I = %.4e  R = %.5f\n\n", Sx, Ex, Ix, Rx);

    // I/N on a log scale, with the equilibrium level
    {
        PRectF rect;
        rect.setXmin(0.0);
        rect.setXmax(T_PLOT);
        rect.setYmin(1e-7);
        rect.setYmax(1.0);
        PlotView view(rect, PlotView::LINEAR, PlotView::LOG);
        PlotFunction1D curve = i, level = FixedConstant(Ix);
        line(curve, "red");
        line(level, "gray", Qt::DashLine);
        view.add(&level);
        view.add(&curve);
        PlotKey key(3000.0, 0.5);
        key.add(&curve, "infected I / N");
        char text[80];
        std::snprintf(text, sizeof text, "equilibrium I* = %.2e", Ix);
        key.add(&level, text);
        view.add(&key);
        labels(view, "SEIR model: infected fraction", "t (days)", "");
        view.save("seir_infected.png");
    }
    // all four groups
    {
        PRectF rect;
        rect.setXmin(0.0);
        rect.setXmax(T_PLOT);
        rect.setYmin(0.0);
        rect.setYmax(1.05);
        PlotView view(rect);
        PlotFunction1D curves[4] = {s, e, i, r};   // added in the order S, I, E, R below
        const char* names[4] = {"susceptible S", "exposed E", "infectious I", "recovered R"};
        const char* colours[4] = {"blue", "darkorange", "red", "darkgreen"};
        PlotKey key(3500.0, 1.0);
        for (int g = 0; g < 4; ++g) {
            line(curves[g], colours[g], g == 1 ? Qt::DashLine : Qt::SolidLine);   // E lies almost on I
            key.add(&curves[g], names[g]);
        }
        for (int g : {0, 2, 1, 3}) view.add(&curves[g]);
        view.add(&key);
        labels(view, "SEIR model: the four groups", "t (days)", "fraction of N");
        view.save("seir_groups.png");
    }
    std::printf("wrote seir_infected.png, seir_groups.png\n");
    return 0;
}
