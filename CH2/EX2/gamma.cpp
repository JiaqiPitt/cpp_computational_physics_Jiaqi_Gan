#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

int main() {
    std::vector<double> epsilons;
    for (int i = 1; i <= 16; ++i) {
        epsilons.push_back(std::pow(10.0, -i));
    }

    std::cout << std::scientific << std::setprecision(10);
    std::cout << std::left
              << std::setw(6)  << "i"
              << std::setw(20) << "epsilon"
              << std::setw(24) << "beta"
              << std::setw(24) << "gamma_direct"
              << std::setw(24) << "gamma_stable"
              << std::setw(20) << "frac_error"
              << std::setw(24) << "gamma_direct_f"
              << std::setw(20) << "frac_error_f"
              << "\n";

    for (std::size_t i = 0; i < epsilons.size(); ++i) {
        double eps = epsilons[i];
        double beta = 1.0 - eps;
        float beta_f = static_cast<float>(beta);
        float gamma_direct_f = 1.0f / std::sqrt(1.0f - beta_f * beta_f);

        double gamma_direct = 1.0 / std::sqrt(1.0 - beta * beta);
        double gamma_stable = 1.0 / std::sqrt((2.0 - eps) * eps);

        double frac_error = std::fabs(gamma_direct - gamma_stable) / gamma_stable;
        double frac_error_f = std::fabs(static_cast<double>(gamma_direct_f) - gamma_stable) / gamma_stable;

        std::cout << std::left
                  << std::setw(6)  << (i + 1)
                  << std::setw(20) << eps
                  << std::setw(24) << beta
                  << std::setw(24) << gamma_direct
                  << std::setw(24) << gamma_stable
                  << std::setw(20) << frac_error
                  << std::setw(24) << gamma_direct_f
                  << std::setw(20) << frac_error_f
                  << "\n";
    }

    return 0;
}
