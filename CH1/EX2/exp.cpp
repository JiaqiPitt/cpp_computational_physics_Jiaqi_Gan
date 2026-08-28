#include <cmath>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <number>\n";
        return 1;
    }

    try
    {
        const double x = std::stod(argv[1]);
        std::cout << std::exp(x) << '\n';
    }
    catch (const std::exception&)
    {
        std::cerr << "Error: the argument must be a number.\n";
        return 1;
    }

    return 0;
}

