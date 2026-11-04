#ifndef SYMPLECTICINTEGRATOR_H
#define SYMPLECTICINTEGRATOR_H
#include "QatGenericFunctions/AbsFunction.h"
#include "QatGenericFunctions/Argument.h"
#include "QatGenericFunctions/PhaseSpace.h"
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

// A symplectic integrator for any separable Hamiltonian H(q, p) = T(p) + V(q).
// As in Section 11.5.2, H is written as a function of the phase-space
// variables, and the equations of motion
//   dq_i/dt = dH/dp_i,   dp_i/dt = -dH/dq_i
// come from the automatic derivative of QatGenericFunctions.
//
// Because H is separable, dH/dp depends on p only and dH/dq on q only.  So q
// can be moved with p held fixed (a "drift", the exact flow of T) and p with
// q held fixed (a "kick", the exact flow of V).  Each of these is an exact
// Hamiltonian flow, hence symplectic, and so is any sequence of them:
//   EULER     drift h, kick h                          order 1 (the book's SYMPLECTIC example)
//   LEAPFROG  kick h/2, drift h, kick h/2              order 2 (Stormer-Verlet)
//   YOSHIDA   leapfrog steps of w1 h, w0 h, w1 h       order 4
class SymplecticIntegrator {
public:
    enum Method { EULER = 1, LEAPFROG = 2, YOSHIDA = 4 };   // the value is the order

    // Starts from the starting values set in the phase space
    SymplecticIntegrator(Genfun::GENFUNCTION H, const Classical::PhaseSpace& space, Method method = LEAPFROG)
        : _H(H.clone()), _n(space.dim()), _x(2 * space.dim()), _method(method) {
        const Classical::PhaseSpace::Component &q = space.coordinates(), &p = space.momenta();
        for (unsigned int i = 0; i < _n; ++i) {
            _dHdq.emplace_back(H.partial(q[i]).clone());
            _dHdp.emplace_back(H.partial(p[i]).clone());
            _x[i] = space.startValue(q[i]);
            _x[_n + i] = space.startValue(p[i]);
        }
        // H must really be separable: d2H/dq_i dp_j = 0 (checked at the start point)
        for (unsigned int i = 0; i < _n; ++i)
            for (unsigned int j = 0; j < _n; ++j) {
                Genfun::GENFUNCTION dHdpj = *_dHdp[j];
                if (dHdpj.partial(q[i])(_x) != 0.0)
                    throw std::invalid_argument("SymplecticIntegrator: the Hamiltonian is not separable");
            }
    }

    void step(double h) {
        if (_method == EULER) {
            drift(h);
            kick(h);
        } else if (_method == LEAPFROG) {
            leapfrog(h);
        } else {
            double w1 = 1 / (2 - std::cbrt(2.0)), w0 = 1 - 2 * w1;
            leapfrog(w1 * h);
            leapfrog(w0 * h);
            leapfrog(w1 * h);
        }
        _t += h;
    }

    double q(unsigned int i) const { return _x[i]; }
    double p(unsigned int i) const { return _x[_n + i]; }
    double time() const { return _t; }
    double energy() const { return (*_H)(_x); }

private:
    // q moves with p fixed, p moves with q fixed; since dH/dp doesn't depend on
    // q (and dH/dq not on p), each can be updated in place
    void drift(double h) {
        for (unsigned int i = 0; i < _n; ++i) _x[i] += h * (*_dHdp[i])(_x);
    }
    void kick(double h) {
        for (unsigned int i = 0; i < _n; ++i) _x[_n + i] -= h * (*_dHdq[i])(_x);
    }
    void leapfrog(double h) {
        kick(h / 2);
        drift(h);
        kick(h / 2);
    }

    std::unique_ptr<const Genfun::AbsFunction> _H;
    std::vector<std::unique_ptr<const Genfun::AbsFunction>> _dHdq, _dHdp;
    unsigned int _n;
    Genfun::Argument _x;     // q_0 .. q_n-1, p_0 .. p_n-1
    Method _method;
    double _t = 0;
};

#endif
