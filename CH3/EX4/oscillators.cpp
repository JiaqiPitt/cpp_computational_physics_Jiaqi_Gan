#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <Eigen/Dense>

int main() {
    Eigen::VectorXd mass(5);
    mass << 1.0, 3.0, 2.0, 1.0, 2.0;

    Eigen::VectorXd springs(4);
    springs << 1.0, 2.0, 2.0, 1.0;

    Eigen::MatrixXd K = Eigen::MatrixXd::Zero(5, 5);
    for (int i = 0; i < 4; ++i) {
        K(i, i)     += springs(i);
        K(i+1, i+1) += springs(i);
        K(i, i+1)   -= springs(i);
        K(i+1, i)   -= springs(i);
    }

    Eigen::VectorXd sqrtm(5), invsqrtm(5);
    for (int i = 0; i < 5; ++i) {
        sqrtm(i) = std::sqrt(mass(i));
        invsqrtm(i) = 1.0 / sqrtm(i);
    }

    Eigen::MatrixXd Kprime = invsqrtm.asDiagonal() * K * invsqrtm.asDiagonal();

    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(Kprime);
    Eigen::VectorXd omega2 = solver.eigenvalues();
    Eigen::MatrixXd V = solver.eigenvectors();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "omega^2 values:\n" << omega2.transpose() << "\n\n";

    Eigen::VectorXd x0(5);
    x0 << -2.0, 3.0, 0.0, -3.0, 2.0;

    Eigen::VectorXd y0 = sqrtm.asDiagonal() * x0;
    Eigen::VectorXd c = V.transpose() * y0;

    double totalMass = mass.sum();

    std::ofstream fout("positions.csv");
    fout << "t,x1,x2,x3,x4,x5,xcm\n";

    for (double t = 0.0; t <= 30.0; t += 0.05) {
        Eigen::VectorXd cosPart(5);
        for (int i = 0; i < 5; ++i) {
            double omega = std::sqrt(std::max(omega2(i), 0.0));
            cosPart(i) = c(i) * std::cos(omega * t);
        }
        Eigen::VectorXd y = V * cosPart;
        Eigen::VectorXd x = invsqrtm.asDiagonal() * y;

        double xcm = mass.dot(x) / totalMass;

        fout << t;
        for (int i = 0; i < 5; ++i) fout << "," << x(i);
        fout << "," << xcm << "\n";
    }

    return 0;
}