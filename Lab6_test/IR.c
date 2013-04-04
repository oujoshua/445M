#include "IR.h"

#define PTS 16

// y = y1 + (x-x1)*slope

// long outputY[7] = {10, 15, 20, 30, 40, 50, 55};//output is in centimeters
// long inputX[7] = {800, 575, 450, 315, 240, 205, 196};

// long slope[7] = {-2223, -4000, -7408, -13334, -28572, -55556, -16130};// divide by 100000

long outputY[PTS] = {10,11,12,13,14,15,17,20,25,30,35,40,45,50,55,60};
long inputX[PTS] = {790,735,686,650,606,570,500,422,365,300,265,237,210,183,170,150};
long slope[PTS] = {-1819, -2041, -2778, -2273, -2278, -2858, -3847, -8772, -7693, -14286, -17858, -18519, -18519, -38462, -25000};

// 2nd order polynnomial coeffs
long cA = 2;
long cB = -2337;
long cC = 888820;


long IRDistance(long x){
  long retVal;
  long i = 0;
//   return ((cA * x * x) + cB * x + cC)/10000;
  while(x < inputX[i] && i < PTS){
    i++;
  }
  retVal = outputY[i] + ((x - inputX[i]) * slope[i]) / 100000;
  return retVal;
}
