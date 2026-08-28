#include <iomanip>
#include <iostream>

double my_exp(double x)
{
    double sum = 1.0;
    double term = 1.0;

    for (int n = 1; n <= 100; ++n)
    {
        term *= x / n;
        sum += term;
    }

    return sum;
}

int main()
{
    std::cout << std::scientific << std::setprecision(10);
    std::cout << "x\tmy_exp(x)\n";

    for (int x = -10; x <= 10; x += 2)
    {
        std::cout << x << '\t' << my_exp(x) << '\n';
    }

    return 0;
}

