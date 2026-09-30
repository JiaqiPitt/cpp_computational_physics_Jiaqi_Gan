#include "MUELLERCALC/StokesVector.h"
#include "MUELLERCALC/MuellerMatrix.h"
#include <cmath>
#include <cstdio>
#include <iostream>

int main () {

  MuellerMatrix PH(MuellerMatrix::FastHorizontal);
  MuellerMatrix DG(MuellerMatrix::Diagonal);
  MuellerMatrix PV(MuellerMatrix::FastVertical);

  // 1. Same circular polarizer as the Jones example in the book
  MuellerMatrix CP=PH*DG*PV;
  StokesVector v0(StokesVector::Horizontal);
  StokesVector v1=CP*v0;
  std::cout << "1. Circular polarizer PH*DG*PV (same as the Jones example)" << std::endl;
  std::cout << "Filter= " << std::endl << CP << std::endl;
  std::cout << "Horizontal in:  " << v0 << "  out: " << v1
            << "  intensity= " << v1.intensity()
            << "  DOP= " << v1.degreeOfPolarization() << std::endl;

  // 2. Unpolarized light through the same filter (Jones cannot do this)
  StokesVector u(StokesVector::Unpolarized);
  StokesVector u1=CP*u;
  std::cout << "Unpolarized in: " << u << "  out: " << u1
            << "  intensity= " << u1.intensity()
            << "  DOP= " << u1.degreeOfPolarization() << std::endl << std::endl;

  // 3. Partially polarized beam: half horizontal, half unpolarized
  StokesVector p=0.5*StokesVector(StokesVector::Horizontal)+0.5*u;
  std::cout << "2. Partially polarized beam 0.5*H + 0.5*unpolarized = " << p
            << "  DOP= " << p.degreeOfPolarization() << std::endl;
  std::cout << "through H polarizer: intensity= "
            << (MuellerMatrix(MuellerMatrix::Horizontal)*p).intensity() << std::endl;
  std::cout << "through V polarizer: intensity= "
            << (MuellerMatrix(MuellerMatrix::Vertical)*p).intensity() << std::endl << std::endl;

  // 4. Malus's law
  std::cout << "3. Linear polarizer at angle theta" << std::endl;
  std::printf("%8s %14s %12s %16s\n", "theta", "H light out", "cos^2 theta", "unpolarized out");
  for (int deg=0; deg<=90; deg+=15) {
    MuellerMatrix P=MuellerMatrix::LinearPolarizer(deg*M_PI/180);
    std::printf("%8d %14.6f %12.6f %16.6f\n", deg,
                (P*v0).intensity(), std::pow(std::cos(deg*M_PI/180),2), (P*u).intensity());
  }
  std::cout << std::endl;

  // 5. Crossed polarizers, with and without a diagonal one in between
  MuellerMatrix H(MuellerMatrix::Horizontal), V(MuellerMatrix::Vertical);
  std::cout << "4. Unpolarized light through polarizers" << std::endl;
  std::cout << "H then V:        intensity= " << (V*H*u).intensity() << std::endl;
  std::cout << "H then D then V: intensity= " << (V*DG*H*u).intensity() << std::endl << std::endl;

  // 6. Circular polarizers and the depolarizer
  StokesVector R(StokesVector::Right), L(StokesVector::Left);
  MuellerMatrix CR(MuellerMatrix::Right), CL(MuellerMatrix::Left), DP(MuellerMatrix::Depolarizer);
  std::cout << "5. Circular polarizers and depolarizer" << std::endl;
  std::cout << "R light through R polarizer: intensity= " << (CR*R).intensity() << std::endl;
  std::cout << "L light through R polarizer: intensity= " << (CR*L).intensity() << std::endl;
  std::cout << "R light through L polarizer: intensity= " << (CL*R).intensity() << std::endl;
  StokesVector d=DP*v0;
  std::cout << "H light through depolarizer: " << d << "  DOP= " << d.degreeOfPolarization() << std::endl;

  return 0;
}