#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int parseInput(int argc, char* argv[])
{
    const std::string usage = std::string("usage: ") + argv[0] + " <n>";

    if (argc != 2)
    {
        std::cerr << usage << '\n';
        std::exit(1);
    }

    int degree = 0;
    std::istringstream stream(argv[1]);

    if (!(stream >> degree))
    {
        std::cerr << usage << '\n';
        std::exit(2);
    }

    if (degree < 0)
    {
        std::cerr << "n must be non-negative.\n";
        std::exit(3);
    }

    return degree;
}

std::vector<std::string> getCanvas(int width, int height)
{
    std::vector<std::string> canvas(height, std::string(width, ' '));

    const int xAxisRow = static_cast<int>(std::round((height - 1) / 2.0));
    for (int column = 0; column < width; ++column)
    {
        canvas[xAxisRow][column] = '-';
    }

    const int yAxisColumn = width / 2;
    for (int row = 0; row < height; ++row)
    {
        canvas[row][yAxisColumn] = '|';
    }

    canvas[xAxisRow][yAxisColumn] = '+';
    return canvas;
}

double hermitePolynomial(int degree, double x)
{
    if (degree == 0)
    {
        return 1.0;
    }

    if (degree == 1)
    {
        return 2.0 * x;
    }

    double previous = 1.0;
    double current = 2.0 * x;

    for (int n = 1; n < degree; ++n)
    {
        const double next = 2.0 * x * current - 2.0 * n * previous;
        previous = current;
        current = next;
    }

    return current;
}

void plotHermite(std::vector<std::string>& canvas, int degree)
{
    const int height = static_cast<int>(canvas.size());
    const int width = static_cast<int>(canvas[0].size());
    constexpr double xMinimum = -2.0;
    constexpr double xMaximum = 2.0;

    std::vector<double> values(width);
    double maximumAbsoluteValue = 0.0;

    for (int column = 0; column < width; ++column)
    {
        const double x = xMinimum
            + (xMaximum - xMinimum) * column / (width - 1);
        const double y = hermitePolynomial(degree, x);

        values[column] = y;
        maximumAbsoluteValue = std::max(maximumAbsoluteValue, std::abs(y));
    }

    if (maximumAbsoluteValue == 0.0)
    {
        maximumAbsoluteValue = 1.0;
    }

    for (int column = 0; column < width; ++column)
    {
        int row = static_cast<int>(std::round(
            (maximumAbsoluteValue - values[column])
            / (2.0 * maximumAbsoluteValue) * (height - 1)));

        row = std::clamp(row, 0, height - 1);
        canvas[row][column] = '*';
    }

    std::cout << "\nHermite Polynomial H_" << degree
              << "(x), -2 <= x <= 2\n\n";

    for (const auto& line : canvas)
    {
        std::cout << line << '\n';
    }
}

int main(int argc, char* argv[])
{
    const int degree = parseInput(argc, argv);
    std::vector<std::string> canvas = getCanvas(81, 41);
    plotHermite(canvas, degree);
    return 0;
}
