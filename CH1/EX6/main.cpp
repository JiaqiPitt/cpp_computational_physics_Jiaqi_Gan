#include "plus.hpp"

int main(int argc, char* argv[])
{
    int first = 0;
    int second = 0;

    if (!parse_arguments(argc, argv, first, second))
    {
        return 1;
    }

    const int result = add(first, second);
    print_result(first, second, result);

    return 0;
}
