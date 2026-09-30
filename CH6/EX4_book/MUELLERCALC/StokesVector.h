#ifndef _STOKESVECTOR_H_
#define _STOKESVECTOR_H_
#include <iostream>
class StokesVector {

 public:

  enum Type {Horizontal,Vertical,Diagonal,Antidiagonal,Right,Left,Unpolarized};

  // Construct a zero vector:
  inline StokesVector();

  // Construct a type from a predefined type (unit intensity)
  inline StokesVector(Type type);

  // Construct on four real numbers:
  inline StokesVector(double s0, double s1, double s2, double s3);

  // Intensity (= S0):
  inline double intensity() const;

  // Degree of polarization, sqrt(S1^2+S2^2+S3^2)/S0:
  inline double degreeOfPolarization() const;

  // Access to individual elements:
  inline double & operator () (unsigned int i);
  inline const double & operator () (unsigned int i) const;

 private:

  double s[4];

};

inline std::ostream & operator << (std::ostream & o, const StokesVector & v);
inline StokesVector operator* (const StokesVector & v, double c);
inline StokesVector operator* (double c, const StokesVector & v);
// Incoherent superposition of two beams:
inline StokesVector operator+ (const StokesVector & v1, const StokesVector & v2);
#include "MUELLERCALC/StokesVector.icc"
#endif