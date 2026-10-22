#ifndef DOUBLEPENDULUM_H
#define DOUBLEPENDULUM_H
#include <array>
#include <cmath>

// Double pendulum made of two rigid rods swinging in one vertical plane.
// Rod 1 hangs from a fixed pivot, rod 2 hangs from the end of rod 1.
// Each rod is a uniform bar with a bob (here a dog-bone biscuit) fixed
// rigidly to its far end; the bob turns with its rod, so its own moment of
// inertia about its centre counts too.
// Angles are measured from the downward vertical, counterclockwise positive.
//
// With A, B, C, G1, G2 defined below, the Lagrangian
//   L = A w1^2/2 + B w1 w2 cos(t1 - t2) + C w2^2/2 + G1 cos t1 + G2 cos t2
// gives the equations of motion
//   A a1 + B cos(t1 - t2) a2 = -B w2^2 sin(t1 - t2) - G1 sin t1
//   B cos(t1 - t2) a1 + C a2 =  B w1^2 sin(t1 - t2) - G2 sin t2
// which are solved for the angular accelerations a1, a2.
class DoublePendulum {
public:
    struct Parameters {
        double l1          = 0.15;    // length of rod 1 (m), pivot to elbow
        double l2          = 0.15;    // length of rod 2 (m), elbow to tip
        double rodMass1    = 0.019;   // aluminium rod 1 (kg), uniform
        double rodMass2    = 0.019;   // aluminium rod 2 (kg), uniform
        double bobMass1    = 0.025;   // bone biscuit at the end of rod 1 (kg)
        double bobMass2    = 0.025;   // bone biscuit at the end of rod 2 (kg)
        double bobInertia1 = 1.8e-5;  // biscuit 1 about its own centre (kg m^2), ~ m (L^2 + W^2)/12
        double bobInertia2 = 1.8e-5;  // biscuit 2 about its own centre (kg m^2)
        double g           = 9.81;    // m/s^2
    };

    // theta1, theta2 (rad), omega1, omega2 (rad/s)
    using State = std::array<double, 4>;

    DoublePendulum(const Parameters& p, const State& initial) : _p(p), _s(initial) {
        // body 1 = rod 1 + bob 1, rotating about the fixed pivot
        // (J = moment of inertia about the pivot, MA = mass x distance of its centre of mass)
        double J1  = p.rodMass1 * p.l1 * p.l1 / 3.0 + p.bobMass1 * p.l1 * p.l1 + p.bobInertia1;
        double MA1 = p.rodMass1 * p.l1 / 2.0 + p.bobMass1 * p.l1;                // mass x CM distance
        // body 2 = rod 2 + bob 2, rotating about the elbow
        double M2  = p.rodMass2 + p.bobMass2;
        double J2  = p.rodMass2 * p.l2 * p.l2 / 3.0 + p.bobMass2 * p.l2 * p.l2 + p.bobInertia2;
        double MA2 = p.rodMass2 * p.l2 / 2.0 + p.bobMass2 * p.l2;

        _A  = J1 + M2 * p.l1 * p.l1;
        _B  = MA2 * p.l1;
        _C  = J2;
        _G1 = p.g * (MA1 + M2 * p.l1);
        _G2 = p.g * MA2;
    }

    // One fourth-order Runge-Kutta step of length dt
    void step(double dt) {
        State k1 = derivative(_s);
        State k2 = derivative(add(_s, k1, dt / 2));
        State k3 = derivative(add(_s, k2, dt / 2));
        State k4 = derivative(add(_s, k3, dt));
        for (int i = 0; i < 4; ++i)
            _s[i] += dt / 6.0 * (k1[i] + 2 * k2[i] + 2 * k3[i] + k4[i]);
        _t += dt;
    }

    // Total energy (J); constant for the exact motion
    double energy() const {
        double c = std::cos(_s[0] - _s[1]);
        double kinetic = 0.5 * _A * _s[2] * _s[2] + _B * _s[2] * _s[3] * c + 0.5 * _C * _s[3] * _s[3];
        double potential = -_G1 * std::cos(_s[0]) - _G2 * std::cos(_s[1]);
        return kinetic + potential;
    }

    const State& state() const { return _s; }
    const Parameters& parameters() const { return _p; }
    double time() const { return _t; }

    // Put the pendulum in a new state (when it is moved by hand)
    void setState(const State& s) { _s = s; }

    // Elbow and tip positions relative to the pivot (x to the right, y up)
    double elbowX() const { return _p.l1 * std::sin(_s[0]); }
    double elbowY() const { return -_p.l1 * std::cos(_s[0]); }
    double tipX() const { return elbowX() + _p.l2 * std::sin(_s[1]); }
    double tipY() const { return elbowY() - _p.l2 * std::cos(_s[1]); }

    // Time derivative of the state: (omega1, omega2, alpha1, alpha2)
    State derivative(const State& s) const {
        double d = s[0] - s[1];
        double c = std::cos(d), sn = std::sin(d);
        double r1 = -_B * s[3] * s[3] * sn - _G1 * std::sin(s[0]);
        double r2 =  _B * s[2] * s[2] * sn - _G2 * std::sin(s[1]);
        double det = _A * _C - _B * _B * c * c;   // always > 0
        double a1 = (_C * r1 - _B * c * r2) / det;
        double a2 = (_A * r2 - _B * c * r1) / det;
        return {s[2], s[3], a1, a2};
    }

private:
    static State add(const State& s, const State& k, double h) {
        return {s[0] + h * k[0], s[1] + h * k[1], s[2] + h * k[2], s[3] + h * k[3]};
    }

    Parameters _p;
    State _s;
    double _t = 0.0;
    double _A, _B, _C, _G1, _G2;
};

#endif
