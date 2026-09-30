#include "QatGenericFunctions/HermitePolynomial.h"
#include "QatGenericFunctions/GaussQuadratureRule.h"
#include "QatGenericFunctions/GaussIntegrator.h"
#include "QatGenericFunctions/Variable.h"
#include "QatGenericFunctions/Exp.h"
#include <Eigen/Dense>
#include <cmath>
#include <cstdio>
#include <string>

using namespace Genfun;
using Eigen::MatrixXd;

const int SIZE = 5;          // 5x5 submatrices, rows/columns 0..4
const int NPOINTS = 10;      // Gauss-Hermite points used for the printed matrices

static Variable X;

// phi_n(x) = pi^(-1/4) / sqrt(2^n n!) * exp(-x^2/2) * H_n(x)   (Eq. 5.69)
auto phi(unsigned int n) {
    static Exp exp;
    double norm = std::pow(M_PI, -0.25) / std::sqrt(std::pow(2.0, n) * std::tgamma(n + 1.0));
    return norm * exp(-X * X / 2.0) * HermitePolynomial(n);
}

// The operators, each acting on phi_j and returning a new function
auto opX(unsigned int j)  { return X * phi(j); }
auto opX2(unsigned int j) { return X * X * phi(j); }
auto opD(unsigned int j)  { return phi(j).prime(); }
auto opD2(unsigned int j) { return phi(j).prime().prime(); }
auto opH(unsigned int j)  { return -0.5 * opD2(j) + 0.5 * opX2(j); }

// (A)_ij = <phi_i | A phi_j>   (Eq. 5.72)
template <typename Op>
MatrixXd matrix(Op op, unsigned int npoints) {
    GaussHermiteRule rule(npoints);
    GaussIntegrator integrator(rule, GaussIntegrator::INTEGRATE_DX);
    MatrixXd A(SIZE, SIZE);
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            A(i, j) = integrator(phi(i) * op(j));
    return A;
}

void print(const std::string& title, MatrixXd A) {
    std::printf("%s\n", title.c_str());
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            double a = std::fabs(A(i, j)) < 5e-13 ? 0.0 : A(i, j);
            std::printf("%11.6f", a);
        }
        std::printf("\n");
    }
    std::printf("\n");
}

// Largest change in the matrix when going from npoints to a 30-point rule
template <typename Op>
double pointError(Op op, unsigned int npoints) {
    return (matrix(op, npoints) - matrix(op, 30)).cwiseAbs().maxCoeff();
}

int main() {
    std::printf("max |A(n points) - A(30 points)| for each operator\n");
    std::printf("%7s %11s %11s %11s %11s %11s\n", "points", "x", "x^2", "D", "D^2", "H");
    for (unsigned int n = 1; n <= 8; ++n)
        std::printf("%7u %11.2e %11.2e %11.2e %11.2e %11.2e\n", n,
                    pointError(opX, n), pointError(opX2, n), pointError(opD, n),
                    pointError(opD2, n), pointError(opH, n));
    std::printf("\nmatrices below use %d points\n\n", NPOINTS);

    MatrixXd Xm  = matrix(opX, NPOINTS);
    MatrixXd X2m = matrix(opX2, NPOINTS);
    MatrixXd Dm  = matrix(opD, NPOINTS);
    MatrixXd D2m = matrix(opD2, NPOINTS);
    MatrixXd Hm  = matrix(opH, NPOINTS);

    print("(a) x", Xm);
    print("(b) x^2", X2m);
    print("(b) x * x  (product of the 5x5 matrices from (a))", Xm * Xm);
    print("(b) x^2 - x * x", X2m - Xm * Xm);
    print("(c) D", Dm);
    print("(c) D^2", D2m);
    print("(c) D * D  (product of the 5x5 matrices)", Dm * Dm);
    print("(c) D^2 - D * D", D2m - Dm * Dm);
    print("(d) H = -1/2 D^2 + 1/2 x^2", Hm);
    print("(d) H built from truncated products: -1/2 D*D + 1/2 x*x", -0.5 * Dm * Dm + 0.5 * Xm * Xm);
    return 0;
}