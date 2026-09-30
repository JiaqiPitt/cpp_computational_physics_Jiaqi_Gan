#ifndef _MUELLERMATRIX_H_
#define _MUELLERMATRIX_H_
#include "MUELLERCALC/StokesVector.h"
#include <iostream>
class MuellerMatrix {

 public:

  enum Type { Horizontal,Vertical,Diagonal,
	      Antidiagonal,Right,Left,
	      Identity, FastHorizontal, FastVertical,
	      Depolarizer};

  // Construct the identity matrix:
  inline MuellerMatrix();

  // Construct a type from a predefined type
  inline MuellerMatrix(Type type);

  // Construct on sixteen real numbers, row by row:
  inline MuellerMatrix(const double (&m)[4][4]);

  // Linear polarizer with transmission axis at angle theta (radians)
  // from the horizontal:
  inline static MuellerMatrix LinearPolarizer(double theta);

  // Access to individual elements:
  inline double & operator       () (unsigned int i, unsigned int j);
  inline const double & operator () (unsigned int i, unsigned int j) const;

 private:

  double a[4][4];

};

inline std::ostream & operator << (std::ostream & o, const MuellerMatrix & m);
inline MuellerMatrix operator* (const MuellerMatrix & m1, const MuellerMatrix & m2);
inline StokesVector  operator* (const MuellerMatrix & m,  const StokesVector & v);
#include "MUELLERCALC/MuellerMatrix.icc"
#endif