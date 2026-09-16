#ifndef TRANSMISSIONKA_H
#define TRANSMISSIONKA_H
#include "QatGenericFunctions/AbsFunction.h"

class TransmissionKa : public Genfun::AbsFunction {
    FUNCTION_OBJECT_DEF(TransmissionKa)
public:
    TransmissionKa(double E, double V0overE);
    virtual ~TransmissionKa();
    TransmissionKa(const TransmissionKa& right);

    virtual double operator() (double ka) const;
    virtual double operator() (const Genfun::Argument& a) const {return operator()(a[0]);}

private:
    double _E;
    double _V0overE;
};
#endif