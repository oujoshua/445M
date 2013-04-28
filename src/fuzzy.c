
#include "os.h"
#include "fuzzy.h"
#include "ir.h"


// the crisp inputs are distance in centimeters read by the sensors
FuzzyDistance Fuzzy_IRs[4], Fuzzy_Ping;

// the fuzzy output value that will be computed (go left, straight, or right)
FuzzyMovement Fuzzy_Output;

/* Fuzzify
 * convert a crisp input into a fuzzy distance
 * input  : dist_cm distance in centimeters
 * input  : fuzzy pointer to FuzzyDistance variable to write the result to
 * outputs: output (writes fuzzy values to struct passed in via pointer)
 */
void Fuzzify(unsigned long dist_cm, FuzzyDistance *output) {
  output->close = 64;
  output->ok = 64;
  output->far = 64;
}

/* Fuzzy_Compute
 * given a set of fuzzy inputs representing the state of each sensor, compute a fuzzy output
 * that represents what direction the robot should turn, or go straight
 * input   : none (FuzzyDistance values for each sensor, read from global variables)
 * outputs : output (writes fuzzy values to struct passed in via pointer
 */
void Fuzzy_Compute(FuzzyMovement *output) {
  output->turnLeft = 0;
  output->goStraight = 255;
  output->turnRight = 0;
}

/* Defuzzify
 * compute the crisp output as a weighted average of the fuzzy variables
 * inputs : none (uses Fuzzy_Output global variable)
 * output : dLeft the difference to be applied to the left PWM, returned by writing to variable passed by pointer
 * output : dRight the difference to be applied to the right PWM, returned by writing to the variable passed by pointer
 */
void Defuzzify(long *dLeft, long *dRight) {
  *dLeft = 0;
  *dRight = 0;
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


