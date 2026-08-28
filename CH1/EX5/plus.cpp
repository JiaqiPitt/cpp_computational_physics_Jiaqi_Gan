#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <integer1> <integer2>\n";
        return 1;
    }

    try
    {
        const int first = std::stoi(argv[1]);
        const int second = std::stoi(argv[2]);

        std::cout << first << '+'
                  << second << '='
                  << first + second << '\n';
    }
    catch (const std::exception&)
    {
        std::cerr << "Error: both arguments must be integers.\n";
        return 1;
    }

    return 0;
}
