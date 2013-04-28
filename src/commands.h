#ifndef __CMD_h__
#define __CMD_h__
#include "OS.h"

// board controlling the two motors
#define PWMPERIODMAX 24999
#define PWMPERIODMIN 3

// duty cycles
#define PWM_FAST	48000
#define PWM_MED		29000
#define PWM_SLOWISH 26000
#define PWM_SLOW	14000
#define HARD			PWM_SLOW
#define SOFT			PWM_MED
#define MED       PWM_SLOWISH

#define LEFT 0
#define RIGHT 1

// time it takes to rotate 90 degrees
#define TURN_90 300

typedef enum {FWD, HARDLEFT, HARDRIGHT, SOFTLEFT, SOFTRIGHT, STOP, MEDLEFT, MEDRIGHT,
              BACK, BACKHLEFT, BACKHRIGHT, BACKSLEFT, BACKSRIGHT, BACKMLEFT, BACKMRIGHT} Command;

void GoStraight(void);
void TurnRight(int speed);
void TurnLeft(int speed);
void Stop(void);
//void dataHandler(EthernetFrame* frame);

#endif

