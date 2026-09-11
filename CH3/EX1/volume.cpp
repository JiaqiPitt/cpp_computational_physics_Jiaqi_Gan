#include <iostream>
#include <iomanip>
#include <Eigen/Dense>

int main() {
    Eigen::Vector3d a(3.0, 0.0, 0.0);
    Eigen::Vector3d b(0.5, 2.0, 0.0);
    Eigen::Vector3d c(0.3, 0.2, 1.5);

    double triple_product = a.dot(b.cross(c));

    Eigen::Matrix3d M;
    M << a[0], a[1], a[2],
         b[0], b[1], b[2],
         c[0], c[1], c[2];
    double det = M.determinant();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "triple product a.(bxc) = " << triple_product << "\n";
    std::cout << "determinant of M        = " << det << "\n";
    std::cout << "volume = " << std::abs(triple_product) << "\n";

    return 0;
}