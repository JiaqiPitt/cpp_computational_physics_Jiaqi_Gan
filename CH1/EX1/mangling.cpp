#include <cmath>

volatile double input = 1.0;
volatile double result = 0.0;

double calculate(double x)
{
    return std::exp(x);
}

int main()
{
    result = calculate(input);
    return 0;
}
