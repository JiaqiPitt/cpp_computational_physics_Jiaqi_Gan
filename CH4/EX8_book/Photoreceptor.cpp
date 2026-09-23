#include "Photoreceptor.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

FUNCTION_OBJECT_IMP(Photoreceptor)

Photoreceptor::Photoreceptor(const std::string& csvFile)
  : _spline(Genfun::CUBIC_SPLINE) {
    std::ifstream in(csvFile);
    if (!in) throw std::runtime_error("cannot open " + csvFile);
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string lam, a;
        std::getline(ss, lam, ',');
        std::getline(ss, a, ',');
        _lambda.push_back(std::stod(lam));
        _absorbance.push_back(std::stod(a));
        _spline.addPoint(_lambda.back(), _absorbance.back());
    }
}

Photoreceptor::~Photoreceptor() {}

Photoreceptor::Photoreceptor(const Photoreceptor& right)
  : AbsFunction(right), _lambda(right._lambda), _absorbance(right._absorbance),
    _spline(right._spline) {}

double Photoreceptor::operator() (double lambda) const {
    return std::max(0.0, _spline(lambda));
}