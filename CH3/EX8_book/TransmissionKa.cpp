#include "TransmissionKa.h"
#include <complex>
#include <cmath>

FUNCTION_OBJECT_IMP(TransmissionKa)

TransmissionKa::TransmissionKa(double E, double V0overE)
  : _E(E), _V0overE(V0overE) {}

TransmissionKa::~TransmissionKa() {}

TransmissionKa::TransmissionKa(const TransmissionKa& right)
  : AbsFunction(right), _E(right._E), _V0overE(right._V0overE) {}

static void stepBack(std::complex<double> k_left, std::complex<double> k_right, double xb,
              std::complex<double> A_right, std::complex<double> B_right,
              std::complex<double>& A_left, std::complex<double>& B_left) {
    std::complex<double> i(0.0, 1.0);
    std::complex<double> C = A_right*std::exp(i*k_right*xb) + B_right*std::exp(-i*k_right*xb);
    std::complex<double> D = (k_right/k_left)*(A_right*std::exp(i*k_right*xb) - B_right*std::exp(-i*k_right*xb));
    A_left = std::exp(-i*k_left*xb)*(C+D)/2.0;
    B_left = std::exp(i*k_left*xb)*(C-D)/2.0;
}

double TransmissionKa::operator() (double ka) const {
    double E = _E;
    double V0 = _V0overE * E;
    double k0 = std::sqrt(E);
    double a = ka / k0;

    std::complex<double> k[5];
    k[0] = std::sqrt(std::complex<double>(E, 0.0));
    k[1] = std::sqrt(std::complex<double>(E - V0, 0.0));
    k[2] = std::sqrt(std::complex<double>(E - 2.0*V0, 0.0));
    k[3] = k[1];
    k[4] = k[0];

    double xb[4] = {-2.0*a, -a, a, 2.0*a};

    std::complex<double> A = 1.0, B = 0.0;
    for (int j = 3; j >= 0; --j) {
        std::complex<double> Anew, Bnew;
        stepBack(k[j], k[j+1], xb[j], A, B, Anew, Bnew);
        A = Anew;
        B = Bnew;
    }

    std::complex<double> t = 1.0 / A;
    double T = (k[4].real() / k[0].real()) * std::norm(t);
    return T;
}