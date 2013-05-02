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
 #pragma O0
void Fuzzify(unsigned long dist_cm, unsigned char scale, FuzzyDistance *output)
{
	unsigned long raw = dist_cm;
	unsigned long far_min = 57,
								far_slope = 8;
	if(dist_cm < CLOSE_MIN)
		dist_cm = CLOSE_MIN;
	else if(dist_cm > FAR_MAX)
		dist_cm = FAR_MAX;
	
  output->close = 255 - ((dist_cm - CLOSE_MIN) * CLOSE_SLOPE);
  output->ok = (dist_cm > OK_MID)
									? ((OK_SLOPE * dist_cm - OK_SLOPE * OK_MIN))
									: (OK_MID * OK_SLOPE + 255 - (OK_SLOPE * dist_cm));
	if(raw < far_min)
		output->far = 0;
	else
		output->far = ((raw - far_min) * far_slope )/ scale;
}

void Fuzzify_All(unsigned long front, unsigned long left, unsigned long fleft, unsigned long fright, unsigned long right)
{
	Fuzzify(front, 1, &Fuzzy_Ping);
	Fuzzify(left, 1, &Fuzzy_IRs[IR_LEFT]);
	Fuzzify(fleft, 1, &Fuzzy_IRs[IR_FLEFT]);
	Fuzzify(fright, 1, &Fuzzy_IRs[IR_FRIGHT]);
	Fuzzify(right, 1, &Fuzzy_IRs[IR_RIGHT]);
}

/* Fuzzy_Compute
 * given a set of fuzzy inputs representing the state of each sensor, compute a fuzzy output
 * that represents what direction the robot should turn, or go straight
 * input   : none (FuzzyDistance values for each sensor, read from global variables)
 * outputs : output (writes fuzzy values to struct passed in via pointer
 */
#pragma O0
void Fuzzy_Compute(void)
{
	FuzzyMovement *output = &Fuzzy_Output;
  output->turnLeft  = /*OR(*//*OR(*//*OR(*/OR(OR(OR(AND(Fuzzy_Ping.close, Fuzzy_IRs[IR_FRIGHT].close),
											AND(Fuzzy_Ping.close, Fuzzy_IRs[IR_RIGHT].close)),
											AND(Fuzzy_IRs[IR_FRIGHT].close, Fuzzy_IRs[IR_RIGHT].close )),
											//OR(Fuzzy_IRs[IR_LEFT].far,Fuzzy_IRs[IR_FLEFT].far)),
											OR(AND(Fuzzy_Ping.ok,Fuzzy_IRs[IR_LEFT].far),AND(Fuzzy_Ping.ok, Fuzzy_IRs[IR_FLEFT].far>>1))
										//	AND(Fuzzy_Ping.close, Fuzzy_IRs[IR_LEFT].ok)),
											);
	
  output->goStraight = (AND(AND(AND(Fuzzy_IRs[IR_FRIGHT].ok,Fuzzy_IRs[IR_FLEFT].ok),AND(Fuzzy_IRs[IR_RIGHT].ok, Fuzzy_IRs[IR_LEFT].ok)), OR(Fuzzy_Ping.ok, Fuzzy_Ping.far)))>>1;
  
	output->turnRight = /*OR(*//*OR(*//*OR(*/OR(OR(OR(AND(Fuzzy_Ping.close, Fuzzy_IRs[IR_FLEFT].close),
											AND(Fuzzy_Ping.close, Fuzzy_IRs[IR_LEFT].close )),
											AND(Fuzzy_IRs[IR_FLEFT].close, Fuzzy_IRs[IR_LEFT].close)),
											//OR(Fuzzy_IRs[IR_RIGHT].far,Fuzzy_IRs[IR_FRIGHT].far )),
										 OR(AND(Fuzzy_Ping.ok,Fuzzy_IRs[IR_RIGHT].far),AND(Fuzzy_Ping.ok, Fuzzy_IRs[IR_FRIGHT].far>>1))
										//	AND(Fuzzy_Ping.close, Fuzzy_IRs[IR_RIGHT].ok)),
											);
}

/* Defuzzify
 * co	mpute the crisp output as a weighted average of the fuzzy variables
 * inputs : none (uses Fuzzy_Output global variable)
 * output : dLeft the difference to be applied to the left PWM, returned by writing to variable passed by pointer
 * output : dRight the difference to be applied to the right PWM, returned by writing to the variable passed by pointer
 */
void Defuzzify(long *dLeft, long *dRight, char *set)
{
  *dLeft = (200 * (Fuzzy_Output.turnRight - Fuzzy_Output.turnLeft))/(Fuzzy_Output.turnLeft + Fuzzy_Output.turnRight + Fuzzy_Output.goStraight);
  *dRight = (200 * (Fuzzy_Output.turnLeft - Fuzzy_Output.turnRight))/(Fuzzy_Output.turnLeft + Fuzzy_Output.turnRight + Fuzzy_Output.goStraight);
	if(*dLeft < 24 && *dRight < 24 /*|| Fuzzy_Output.goStraight > (MAX(Fuzzy_Output.turnRight,Fuzzy_Output.turnLeft))*/)
		*set = 1;//< 20
	else
		*set = 0;
}

/* Fuzzy_Or
 * compute the fuzzy or, A + B, which is equivalent to a mathematical MAX
 * input A : FuzzyValue argument to or/max
 * input B : FuzzyValue argument to or/max
 * output  : FuzzyVale or/max of A and B
 */
FuzzyValue OR(FuzzyValue A, FuzzyValue B) {
  return (A > B) ? A : B;
}

/* Fuzzy_And
 * compute the fuzzy and, A * B, which is equivalent to a mathematical MIN
 * input A : FuzzyValue argument to and/min
 * input B : FuzzyValue argument to and/min
 * output  : FuzzyValue and/min of A and B
 */
FuzzyValue AND(FuzzyValue A, FuzzyValue B) {
  return (A < B) ? A : B;
}


