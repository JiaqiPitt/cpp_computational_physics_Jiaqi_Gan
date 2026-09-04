#include <iostream>
#include <iomanip>
#include <complex>
#include <cmath>
#include <fstream>

std::complex<double> psi(double x, double k1, std::complex<double> k2,
                          std::complex<double> B, std::complex<double> C) {
    std::complex<double> i(0.0, 1.0);
    if (x < 0.0) {
        return std::exp(i*k1*x) + B*std::exp(-i*k1*x);
    }
    return C*std::exp(i*k2*x);
}

int main() {
    double E = 1.0;
    double k1 = std::sqrt(E);

    std::cout << "V/E, R, T, R+T\n";
    std::cout << std::fixed << std::setprecision(6);

    double ratios[] = {0.0, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99, 1.0, 1.5, 2.0, 3.0};
    for (double ratio : ratios) {
        double V = ratio * E;
        std::complex<double> k2 = std::sqrt(std::complex<double>(E - V, 0.0));

        std::complex<double> B = (k1 - k2) / (k1 + k2);
        std::complex<double> C = 2.0*k1 / (k1 + k2);

        double R = std::norm(B);
        double T = (k2.real()/k1) * std::norm(C);

        std::cout << ratio << ", " << R << ", " << T << ", " << R+T << "\n";
    }

    std::ofstream fout1("wavefunction_above.csv");
    fout1 << "x,re_psi,im_psi,psi2\n";
    {
        double V = 0.5*E;
        std::complex<double> k2 = std::sqrt(std::complex<double>(E-V, 0.0));
        std::complex<double> B = (k1-k2)/(k1+k2);
        std::complex<double> C = 2.0*k1/(k1+k2);
        for (double x = -10.0; x <= 10.0; x += 0.05) {
            std::complex<double> p = psi(x, k1, k2, B, C);
            fout1 << x << "," << p.real() << "," << p.imag() << "," << std::norm(p) << "\n";
        }
    }

    std::ofstream fout2("wavefunction_below.csv");
    fout2 << "x,re_psi,im_psi,psi2\n";
    {
        double V = 2.0*E;
        std::complex<double> k2 = std::sqrt(std::complex<double>(E-V, 0.0));
        std::complex<double> B = (k1-k2)/(k1+k2);
        std::complex<double> C = 2.0*k1/(k1+k2);
        for (double x = -10.0; x <= 10.0; x += 0.05) {
            std::complex<double> p = psi(x, k1, k2, B, C);
            fout2 << x << "," << p.real() << "," << p.imag() << "," << std::norm(p) << "\n";
        }
    }

    return 0;
}