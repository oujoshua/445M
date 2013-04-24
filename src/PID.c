
#include "os.h"

#define OUT_MIN 5
#define OUT_MAX 49999
#define MAX_USRS 2
#define ON 1
#define OFF 0

// constants
int Kp[MAX_USRS] = {1, };
int Kd[MAX_USRS] = {1, };
int Ki[MAX_USRS] = {1, };

// for each user, keep track of the last input and the time it was calculated
// for use in the derivative term
unsigned long LastInput[MAX_USRS] = {0, };
unsigned long LastTime[MAX_USRS] = {0, };

// integral term for each controller
int Integral[MAX_USRS] = {10000, };

// keep track of last output, for use when turning PID on/off
unsigned long LastOutput[MAX_USRS] = {0, };

// the value the controller is trying to force each input to
int Target[MAX_USRS] = {0, };

// the state of each controller (on/off)
char Enabled[MAX_USRS] = {OFF, };


#pragma O0
long PID_Compute(unsigned long input, int id) {
  static int first = -1;  // each bit is one if this is the first time a value is being calculated (LastInput is invalid)
  int error, derTerm;
  long output;
  unsigned long now;
  id = id % MAX_USRS;
  now = OS_Time();  // does this have enough resolution??
  error = Target[id] - input;
  if(first & (1 << id)) {
    // first output value, ignore LastInput, integral, and derivative terms and clear bit
    derTerm = 0;
    Integral[id] = 0;
    first = first & ~(1 << id);
  }
  else {
    long timeDiff = OS_TimeDifference(LastTime[id], now);
    derTerm = (Kd[id] * (input - LastInput[id])) / timeDiff;
    Integral[id] += Ki[id] * (error);
  }
  // bounds check on integral - keep from accumulating too much error
  if(Integral[id] > OUT_MAX) {
    Integral[id] = OUT_MAX;
  }
  else if(Integral[id] < OUT_MIN) {
    Integral[id] = OUT_MIN;
  }
  // comput output
  //output = Kp[id] * error + Ki[id] * Integral[id] - Kd[id] * derTerm;
  //without the D
	output = Kp[id] * error + Integral[id];
	// bounds check output
  output = (output > OUT_MAX) ? OUT_MAX : (output < OUT_MIN) ? OUT_MIN : output;
  
  // remember values for next call
  LastTime[id] = now;
  LastInput[id] = input;
  LastOutput[id] = output;
  if(Enabled[id] == ON) {
    return output;
  }
  else {
    return OUT_MIN;
  }
  
}

// set the Kp, Kd, and Ki terms for the user specified by id
void PID_SetConstants(int p, int d, int i, int id) {
  id = id % MAX_USRS;
  Kp[id] = p;
  Kd[id] = d;
  Ki[id] = i;
}

// set the target [input] for the user specified by id
void PID_SetTarget(int t, int id) {
  id = id % MAX_USRS;
  Target[id] = t;
}

void PID_Disable(int id) {
  id = id % MAX_USRS;
  Enabled[id] = OFF;
}

void PID_Enable(int id) {
  id = id % MAX_USRS;
  Enabled[id] = ON;
}

