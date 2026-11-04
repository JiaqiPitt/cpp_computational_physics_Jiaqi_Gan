#ifndef TOP_H
#define TOP_H
#include <array>
#include <cmath>

class Top {
public:
    // A toy top, SI units: mass, distance from the tip to the centre of mass,
    // moments of inertia about the tip, and the spin it is given (rad/s)
    static constexpr double M = 0.1, L = 0.04, G = 9.81;
    static constexpr double I1 = 1.6e-4, I3 = 6.0e-5;
    static constexpr double SPIN = 150.0;

    // Let go at angle theta0 from the vertical with theta' = 0, precessing at
    // phidot0.  a, b and alpha come from Eqs. 11.11-11.13 at t = 0.
    void start(double theta0, double phidot0) {
        double u0 = std::cos(theta0), s2 = 1 - u0 * u0;
        double w3 = phidot0 * u0 + SPIN;           // spin about the symmetry axis
        _a = I3 * w3 / I1;
        _b = phidot0 * s2 + _a * u0;
        double E = 0.5 * I1 * phidot0 * phidot0 * s2 + 0.5 * I3 * w3 * w3 + M * G * L * u0;
        _alpha = 2 * E / I1 - I1 / I3 * _a * _a;
        _beta = 2 * M * G * L / I1;
        _s = {u0, 0.0, 0.0, 0.0};
        _t = 0;
    }

    // One fourth-order Runge-Kutta step of length dt
    void step(double dt) {
        State k1 = rate(_s);
        State k2 = rate(add(_s, k1, dt / 2));
        State k3 = rate(add(_s, k2, dt / 2));
        State k4 = rate(add(_s, k3, dt));
        for (int i = 0; i < 4; ++i) _s[i] += dt / 6 * (k1[i] + 2 * k2[i] + 2 * k3[i] + k4[i]);
        _t += dt;
    }

    double theta() const { return std::acos(_s[0]); }
    double phi() const { return _s[2]; }
    double psi() const { return _s[3]; }
    double time() const { return _t; }

private:
    using State = std::array<double, 4>;   // u, w, phi, psi
    State rate(const State& x) const {
        double u = x[0];
        double fPrime = -2 * u * (_alpha - _beta * u) - _beta * (1 - u * u) + 2 * _a * (_b - _a * u);
        double phidot = (_b - _a * u) / (1 - u * u);
        return {x[1], 0.5 * fPrime, phidot, I1 * _a / I3 - u * phidot};
    }
    static State add(const State& x, const State& k, double h) {
        return {x[0] + h * k[0], x[1] + h * k[1], x[2] + h * k[2], x[3] + h * k[3]};
    }
    double _a = 0, _b = 0, _alpha = 0, _beta = 0, _t = 0;
    State _s{};
};

#endif