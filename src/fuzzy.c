
#include "fuzzy.h"


// the crisp inputs are distance in centimeters read by the sensors
FuzzyDistance Fuzzy_IRs[4], Fuzzy_Ping;

// the fuzzy output value that will be computed (go left, straight, or right)
FuzzyMovement Fuzzy_Output;

/* Fuzzify
 * convert a crisp input into a fuzzy distance
 * input  : dist_cm distance in centimeters
 * input  : fuzzy pointer to FuzzyDistance variable to write the result to
 * outputs: none (writes fuzzy values to struct passed in via pointer)
 */
void Fuzzify(unsigned long dist_cm, FuzzyDistance *fuzzy) {
  fuzzy->close = 64;
  fuzzy->ok = 64;
  fuzzy->far = 64;
}


/* Fuzzy_Or
 * compute the fuzzy or, A + B, which is equivalent to a mathematical MAX
 * input A : FuzzyValue argument to or/max
 * input B : FuzzyValue argument to or/max
 * output  : FuzzyVale or/max of A and B
 */
FuzzyValue Fuzzy_Or(FuzzyValue A, FuzzyValue B) {
  return (A > B) ? A : B;
}

/* Fuzzy_And
 * compute the fuzzy and, A * B, which is equivalent to a mathematical MIN
 * input A : FuzzyValue argument to and/min
 * input B : FuzzyValue argument to and/min
 * output  : FuzzyValue and/min of A and B
 */
FuzzyValue Fuzzy_And(FuzzyValue A, FuzzyValue B) {
  return (A < B) ? A : B;
}


