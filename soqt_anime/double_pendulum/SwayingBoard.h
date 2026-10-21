#ifndef SWAYINGBOARD_H
#define SWAYINGBOARD_H
#include <array>
#include <cmath>

// The dog board on its stick, rocking in its own plane about the bottom of
// the stick.  A torsion spring at the bottom holds it upright, so it is an
// inverted pendulum with a restoring spring:
//   I phi'' = -k phi + M g h sin(phi)
// phi is the tilt from the vertical, I the moment of inertia about the pivot,
// M the mass and h the height of the centre of mass above the pivot.
class SwayingBoard {
public:
    struct Parameters {
        double inertia;      // I (kg m^2)
        double massHeight;   // M h (kg m)
        double stiffness;    // k (N m / rad), must exceed M g h
        double g = 9.81;
    };

    SwayingBoard(const Parameters& p, double angle, double rate = 0.0) : _p(p), _s{angle, rate} {}

    // One fourth-order Runge-Kutta step of length dt
    void step(double dt) {
        State k1 = derivative(_s);
        State k2 = derivative({_s[0] + dt / 2 * k1[0], _s[1] + dt / 2 * k1[1]});
        State k3 = derivative({_s[0] + dt / 2 * k2[0], _s[1] + dt / 2 * k2[1]});
        State k4 = derivative({_s[0] + dt * k3[0], _s[1] + dt * k3[1]});
        _s[0] += dt / 6 * (k1[0] + 2 * k2[0] + 2 * k3[0] + k4[0]);
        _s[1] += dt / 6 * (k1[1] + 2 * k2[1] + 2 * k3[1] + k4[1]);
    }

    double angle() const { return _s[0]; }
    double energy() const {
        return 0.5 * _p.inertia * _s[1] * _s[1] + 0.5 * _p.stiffness * _s[0] * _s[0]
             + _p.massHeight * _p.g * std::cos(_s[0]);
    }

    // Spring constant giving small oscillations of the given period
    static double stiffnessForPeriod(double inertia, double massHeight, double period, double g = 9.81) {
        double w = 2 * M_PI / period;
        return inertia * w * w + massHeight * g;
    }

private:
    using State = std::array<double, 2>;   // phi, dphi/dt
    State derivative(const State& s) const {
        return {s[1], (-_p.stiffness * s[0] + _p.massHeight * _p.g * std::sin(s[0])) / _p.inertia};
    }
    Parameters _p;
    State _s;
};

#endif
