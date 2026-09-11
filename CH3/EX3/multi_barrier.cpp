#include <iostream>
#include <iomanip>
#include <complex>
#include <cmath>
#include <fstream>

void stepBack(std::complex<double> k_left, std::complex<double> k_right, double xb,
              std::complex<double> A_right, std::complex<double> B_right,
              std::complex<double>& A_left, std::complex<double>& B_left) {
    std::complex<double> i(0.0, 1.0);
    std::complex<double> C = A_right*std::exp(i*k_right*xb) + B_right*std::exp(-i*k_right*xb);
    std::complex<double> D = (k_right/k_left)*(A_right*std::exp(i*k_right*xb) - B_right*std::exp(-i*k_right*xb));
    A_left = std::exp(-i*k_left*xb)*(C+D)/2.0;
    B_left = std::exp(i*k_left*xb)*(C-D)/2.0;
}

void computeRT(double E, double V0, double a, double& R, double& T) {
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

    std::complex<double> r = B / A;
    std::complex<double> t = 1.0 / A;

    R = std::norm(r);
    T = (k[4].real() / k[0].real()) * std::norm(t);
}

int main() {
    double E = 1.0;
    double k0 = std::sqrt(E);

    double ka_values[] = {0.5, 1.0, 2.0, 4.0};
    double ratios[] = {0.3, 0.6, 0.9, 1.1, 1.5, 2.0};

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "ka      V0/E    R          T          R+T\n";

    for (double ka : ka_values) {
        double a = ka / k0;
        for (double ratio : ratios) {
            double V0 = ratio * E;
            double R, T;
            computeRT(E, V0, a, R, T);
            std::cout << std::setw(6) << ka << "  " << std::setw(6) << ratio << "  "
                      << R << "  " << T << "  " << (R+T) << "\n";
        }
    }

    std::ofstream fout("resonance_scan.csv");
    fout << "ka,T\n";
    double V0_fine = 0.3 * E;
    for (double ka = 1.0; ka <= 4.0; ka += 0.02) {
        double a = ka / k0;
        double R, T;
        computeRT(E, V0_fine, a, R, T);
        fout << ka << "," << T << "\n";
    }

    return 0;
}
