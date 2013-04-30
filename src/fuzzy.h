#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define CLOSE_MIN		12
#define CLOSE_MAX		48
#define OK_MIN			12
#define OK_MID			30
#define OK_MAX			48
#define FAR_MIN			12
#define FAR_MAX			48

#define CLOSE_SLOPE	((unsigned short)(255/(CLOSE_MAX - CLOSE_MIN)))
#define OK_SLOPE	((unsigned short)((255/(OK_MAX - OK_MIN)*2)))
#define FAR_SLOPE	((unsigned short)(255/(FAR_MAX - FAR_MIN)))

typedef unsigned short FuzzyValue;

typedef struct FuzzyDistance {
  FuzzyValue close;
  FuzzyValue ok;
  FuzzyValue far;
} FuzzyDistance;

typedef struct FuzzyMovement {
  FuzzyValue turnLeft;
  FuzzyValue goStraight;
  FuzzyValue turnRight;
} FuzzyMovement;

extern FuzzyMovement Fuzzy_Output;
/* convert a crisp input into a fuzzy distance
 * input  : dist_cm distance in centimeters
 * input  : fuzzy pointer to FuzzyDistance variable to write the result to
 * outputs: none (writes fuzzy values to struct passed in via pointer)
 */
void Fuzzify(unsigned long dist_cm, unsigned char scale, FuzzyDistance *fuzzy);

/* Fuzzy_Compute
 * given a set of fuzzy inputs representing the state of each sensor, compute a fuzzy output
 * that represents what direction the robot should turn, or go straight
 * input   : none (FuzzyDistance values for each sensor, read from global variables)
 * outputs : output (writes fuzzy values to struct passed in via pointer
 */
void Fuzzy_Compute(void);

/* Defuzzify
 * compute the crisp output as a weighted average of the fuzzy variables
 * inputs : none (uses Fuzzy_Output global variable)
 * output : dLeft the difference to be applied to the left PWM, returned by writing to variable passed by pointer
 * output : dRight the difference to be applied to the right PWM, returned by writing to the variable passed by pointer
 */
void Defuzzify(long *dLeft, long *dRight);

/* compute the fuzzy or, A + B, which is equivalent to a mathematical MAX
 * input A : FuzzyValue argument to or/max
 * input B : FuzzyValue argument to or/max
 * output  : FuzzyVale or/max of A and B
 */
FuzzyValue OR(FuzzyValue A, FuzzyValue B);

/* compute the fuzzy and, A * B, which is equivalent to a mathematical MIN
 * input A : FuzzyValue argument to and/min
 * input B : FuzzyValue argument to and/min
 * output  : FuzzyValue and/min of A and B
 */
FuzzyValue AND(FuzzyValue A, FuzzyValue B);

void Fuzzify_All(unsigned long front, unsigned long left, unsigned long fleft, unsigned long fright, unsigned long right);
