
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
char Enabled[MAX_USRS] = {ON, };


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
  return output;
  
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


 
// #define DIRECT 0
// #define REVERSE 1
// int controllerDirection = DIRECT;
//  
// void Compute()
// {
//    if(!inAuto) return;
//    unsigned long now = millis();
//    int timeChange = (now - lastTime);
//    if(timeChange>=SampleTime)
//    {
//       /*Compute all the working error variables*/
//       double error = Setpoint - Input;
//       ITerm+= (ki * error);
//       if(ITerm > outMax) ITerm= outMax;
//       else if(ITerm < outMin) ITerm= outMin;
//       double dInput = (Input - lastInput);
//  
//       /*Compute PID Output*/
//       Output = kp * error + ITerm- kd * dInput;
//       if(Output > outMax) Output = outMax;
//       else if(Output < outMin) Output = outMin;
//  
//       /*Remember some variables for next time*/
//       lastInput = Input;
//       lastTime = now;
//    }
// }
//  
// void SetTunings(double Kp, double Ki, double Kd)
// {
//    if (Kp<0 || Ki<0|| Kd<0) return;
//  
//   double SampleTimeInSec = ((double)SampleTime)/1000;
//    kp = Kp;
//    ki = Ki * SampleTimeInSec;
//    kd = Kd / SampleTimeInSec;
//  
//   if(controllerDirection ==REVERSE)
//    {
//       kp = (0 - kp);
//       ki = (0 - ki);
//       kd = (0 - kd);
//    }
// }
//  
// void SetSampleTime(int NewSampleTime)
// {
//    if (NewSampleTime > 0)
//    {
//       double ratio  = (double)NewSampleTime
//                       / (double)SampleTime;
//       ki *= ratio;
//       kd /= ratio;
//       SampleTime = (unsigned long)NewSampleTime;
//    }
// }
//  
// void SetOutputLimits(double Min, double Max)
// {
//    if(Min > Max) return;
//    outMin = Min;
//    outMax = Max;
//  
//    if(Output > outMax) Output = outMax;
//    else if(Output < outMin) Output = outMin;
//  
//    if(ITerm > outMax) ITerm= outMax;
//    else if(ITerm < outMin) ITerm= outMin;
// }
//  
// void SetMode(int Mode)
// {
//     bool newAuto = (Mode == AUTOMATIC);
//     if(newAuto == !inAuto)
//     {  /*we just went from manual to auto*/
//         Initialize();
//     }
//     inAuto = newAuto;
// }
//  
// void Initialize()
// {
//    lastInput = Input;
//    ITerm = Output;
//    if(ITerm > outMax) ITerm= outMax;
//    else if(ITerm < outMin) ITerm= outMin;
// }
//  
// void SetControllerDirection(int Direction)
// {
//    controllerDirection = Direction;
// }
