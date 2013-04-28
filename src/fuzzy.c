
#include "fuzzy.h"

FuzzyDistance Fuzzy_IRs[4], Fuzzy_Ping;

// convert a crisp input into a fuzzy distance
// input  : distance in centimeters
// input  : pointer to FuzzyDistance variable to write the result to
// outputs: none (writes fuzzy values to struct passed in via pointer)
void Fuzzify(unsigned long dist_cm, FuzzyDistance *fuzzy) {
  fuzzy->close = 64;
  fuzzy->ok = 64;
  fuzzy->far = 64;
}

unsigned char Fuzzy_Subtract(FuzzyValue A, FuzzyValue B) {
  return 0;
}
