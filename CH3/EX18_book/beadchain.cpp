#include <iostream>
#include <fstream>
#include <cmath>
#include <Eigen/Dense>

int main() {
    const int N = 100;
    double m = 1.0, k = 1.0;

    Eigen::MatrixXd K = Eigen::MatrixXd::Zero(N, N);
    for (int i = 0; i < N; ++i) {
        K(i, i) = 2.0 * k;
        if (i > 0) {
            K(i, i-1) = -k;
            K(i-1, i) = -k;
        }
    }

    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(K / m);
    Eigen::VectorXd omega2 = solver.eigenvalues();
    Eigen::MatrixXd V = solver.eigenvectors();

    std::cout << "first five omega^2 values:\n";
    for (int j = 0; j < 5; ++j) {
        std::cout << "  " << omega2(j) << "\n";
    }

    std::ofstream fout("eigenmodes.csv");
    fout << "i";
    for (int j = 0; j < 5; ++j) fout << ",mode" << (j+1);
    fout << "\n";

    for (int i = 0; i < N; ++i) {
        fout << (i+1);
        for (int j = 0; j < 5; ++j) {
            fout << "," << V(i, j);
        }
        fout << "\n";
    }

    return 0;
}