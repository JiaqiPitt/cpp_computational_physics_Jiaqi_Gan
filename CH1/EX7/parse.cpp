#include "plus.hpp"

#include <exception>
#include <iostream>
#include <string>

bool parse_arguments(
    int argc,
    char* argv[],
    int& first,
    int& second
)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <integer1> <integer2>\n";
        return false;
    }

    try
    {
        first = std::stoi(argv[1]);
        second = std::stoi(argv[2]);
    }
    catch (const std::exception&)
    {
        std::cerr << "Error: both arguments must be integers.\n";
        return false;
    }

    return true;
}
