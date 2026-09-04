#include <iostream>
#include <iomanip>
#include <complex>
#include <cmath>
#include <fstream>

int main() {
    std::complex<double> i(0.0, 1.0);

    double xs[] = {0.0, 0.5, 1.0, 2.0, -1.5};

    std::cout << std::fixed << std::setprecision(6);

    for (double x : xs) {
        std::complex<double> lhs1 = std::sin(i*x);
        std::complex<double> rhs1 = i*std::sinh(x);

        std::complex<double> lhs2 = std::cos(i*x);
        double rhs2 = std::cosh(x);

        std::cout << "x = " << x << "\n";
        std::cout << "  sin(ix) = " << lhs1 << ", i*sinh(x) = " << rhs1 << "\n";
        std::cout << "  cos(ix) = " << lhs2 << ", cosh(x)   = " << rhs2 << "\n";
    }

    std::ofstream fout("identities_data.csv");
    fout << "x,sinh_x,im_sin_ix,cosh_x,re_cos_ix\n";
    for (double x = -3.0; x <= 3.0; x += 0.05) {
        std::complex<double> s = std::sin(i*x);
        std::complex<double> c = std::cos(i*x);
        fout << x << "," << std::sinh(x) << "," << s.imag() << "," << std::cosh(x) << "," << c.real() << "\n";
    }

    return 0;
}