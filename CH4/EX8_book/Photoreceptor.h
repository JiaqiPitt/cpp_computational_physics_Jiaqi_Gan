#ifndef PHOTORECEPTOR_H
#define PHOTORECEPTOR_H
#include "QatGenericFunctions/AbsFunction.h"
#include "QatGenericFunctions/InterpolatingFunction.h"
#include <string>
#include <vector>

class Photoreceptor : public Genfun::AbsFunction {
    FUNCTION_OBJECT_DEF(Photoreceptor)
public:
    explicit Photoreceptor(const std::string& csvFile);
    virtual ~Photoreceptor();
    Photoreceptor(const Photoreceptor& right);

    virtual double operator() (double lambda) const;
    virtual double operator() (const Genfun::Argument& a) const {return operator()(a[0]);}

    const std::vector<double>& lambdas() const {return _lambda;}
    const std::vector<double>& absorbances() const {return _absorbance;}

private:
    std::vector<double> _lambda;
    std::vector<double> _absorbance;
    Genfun::InterpolatingFunction _spline;
};
#endif