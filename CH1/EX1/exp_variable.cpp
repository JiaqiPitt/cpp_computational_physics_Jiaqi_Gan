#include <cmath>

volatile double input = 1.0;
volatile double result = 0.0;

int main()
{
    result = std::exp(input);
    return 0;
}
