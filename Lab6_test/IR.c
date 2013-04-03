#include "IR.h"

#define PTS 7

// y = y1 + (x-x1)*slope

long outputY[7] = {10, 15, 20, 30, 40, 50, 55};//output is in centimeters
long inputX[7] = {800, 575, 450, 315, 240, 205, 196};

long slope[7] = {-2223, -4000, -7408, -13334, -28572, -55556, -16130};// divide by 100000


long IRDistance(long x){
  long retVal;
  long i = 0;
  while(x < inputX[i] && i < 7){
    i++;
  }
  retVal = outputY[i] + ((x - inputX[i]) * slope[i]) / 100000;
  return retVal;
}
