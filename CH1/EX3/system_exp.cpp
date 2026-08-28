#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    std::cout << std::scientific << std::setprecision(10);
    std::cout << "x\tstd::exp(x)\n";

    for (int x = -10; x <= 10; x += 2)
    {
        std::cout << x << '\t'
                  << std::exp(static_cast<double>(x))
                  << '\n';
    }

    return 0;
}
