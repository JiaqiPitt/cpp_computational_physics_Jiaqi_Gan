#include <iostream>
#include <iomanip>
#include <complex>
#include <Eigen/Dense>

void diagonalize(const std::complex<double> M[3][3],
                  std::complex<double> eigenvalues[3],
                  std::complex<double> eigenvectors[3][3]) {
    Eigen::Matrix3cd A;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            A(r, c) = M[r][c];
        }
    }

    Eigen::ComplexEigenSolver<Eigen::Matrix3cd> solver(A);

    for (int k = 0; k < 3; ++k) {
        eigenvalues[k] = solver.eigenvalues()(k);
        for (int r = 0; r < 3; ++r) {
            eigenvectors[r][k] = solver.eigenvectors()(r, k);
        }
    }
}

int main() {
    std::complex<double> M[3][3] = {
        { {0.333333, 0.0}, {-0.244017, 0.0}, {0.910684, 0.0} },
        { {0.910684, 0.0}, {0.333333, 0.0}, {-0.244017, 0.0} },
        { {-0.244017, 0.0}, {0.910684, 0.0}, {0.333333, 0.0} }
    };

    std::complex<double> eigenvalues[3];
    std::complex<double> eigenvectors[3][3];

    diagonalize(M, eigenvalues, eigenvectors);

    std::cout << std::fixed << std::setprecision(6);

    std::cout << "eigenvalues:\n";
    for (int k = 0; k < 3; ++k) {
        std::cout << "  " << eigenvalues[k] << "\n";
    }

    std::cout << "\neigenvectors (columns):\n";
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            std::cout << eigenvectors[r][c] << " ";
        }
        std::cout << "\n";
    }

    return 0;
}