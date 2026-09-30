#include "QatGenericFunctions/HermitePolynomial.h"
#include "QatGenericFunctions/GaussQuadratureRule.h"
#include "QatGenericFunctions/GaussIntegrator.h"
#include "QatGenericFunctions/Variable.h"
#include "QatGenericFunctions/Exp.h"
#include <cmath>
#include <cstdio>

using namespace Genfun;

const unsigned int NMAX = 5;

// phi_n(x) = pi^(-1/4) / sqrt(2^n n!) * exp(-x^2/2) * H_n(x)   (Eq. 5.69)
auto phi(unsigned int n) {
    static Variable X;
    static Exp exp;
    double norm = std::pow(M_PI, -0.25) / std::sqrt(std::pow(2.0, n) * std::tgamma(n + 1.0));
    return norm * exp(-X * X / 2.0) * HermitePolynomial(n);
}

// <phi_i|phi_j> computed with an npoints Gauss-Hermite rule
double overlap(unsigned int i, unsigned int j, unsigned int npoints) {
    GaussHermiteRule rule(npoints);
    GaussIntegrator integrator(rule, GaussIntegrator::INTEGRATE_DX);
    return integrator(phi(i) * phi(j));
}

void printMatrix(unsigned int npoints) {
    std::printf("A_ij = <phi_i|phi_j>, Gauss-Hermite with %u points\n", npoints);
    for (unsigned int i = 0; i <= NMAX; ++i) {
        for (unsigned int j = 0; j <= NMAX; ++j) {
            double a = overlap(i, j, npoints);
            if (std::fabs(a) < 5e-15) a = 0.0;
            std::printf("%12.8f", a);
        }
        std::printf("\n");
    }
    std::printf("\n");
}

int main() {
    std::printf(" points   max|A - I|   worst element\n");
    for (unsigned int npoints = 1; npoints <= 10; ++npoints) {
        double worst = 0.0;
        unsigned int wi = 0, wj = 0;
        for (unsigned int i = 0; i <= NMAX; ++i) {
            for (unsigned int j = 0; j <= NMAX; ++j) {
                double err = std::fabs(overlap(i, j, npoints) - (i == j ? 1.0 : 0.0));
                if (err > worst) { worst = err; wi = i; wj = j; }
            }
        }
        std::printf("%7u %12.3e      (%u,%u)\n", npoints, worst, wi, wj);
    }
    std::printf("\n");

    printMatrix(5);
    printMatrix(6);
    return 0;
}