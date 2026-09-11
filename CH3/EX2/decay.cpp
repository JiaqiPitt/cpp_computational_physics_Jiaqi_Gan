#include <iostream>
#include <iomanip>
#include <cmath>
#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>

int main() {
    double half_life_b = 45.6;         // minutes
    double half_life_t = 132.0 / 60.0; // 132 seconds -> minutes
    double half_life_p = 195.0;        // minutes

    double lambda_b = std::log(2.0) / half_life_b;
    double lambda_t = std::log(2.0) / half_life_t;
    double lambda_p = std::log(2.0) / half_life_p;

    Eigen::Matrix3d Lambda;
    Lambda << -lambda_b, 0.0,        0.0,
                lambda_b, -lambda_t, 0.0,
                0.0,       lambda_t, -lambda_p;

    std::cout << "Lambda matrix (per minute):\n" << Lambda << "\n\n";

    Eigen::Vector3d x0(1.0, 0.0, 0.0);

    double times[] = {50.0, 150.0, 200.0, 500.0, 1000.0};

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "t(min)     Bi-213      Tl-209      Pb-209\n";
    for (double t : times) {
        Eigen::Matrix3d expLt = (Lambda * t).exp();
        Eigen::Vector3d x = expLt * x0;
        std::cout << std::setw(6) << t << "  "
                  << x(0) << "  " << x(1) << "  " << x(2) << "\n";
    }

    return 0;
}