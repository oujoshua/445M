#ifndef __CMD_h__
#define __CMD_h__
#include "OS.h"

// board controlling the two motors
#define PWMPERIODMAX 24999
#define PWMPERIODMIN 3

// duty cycles
#define PWM_FAST	30000
#define PWM_MED		16000
#define PWM_SLOW	12000
#define HARD			PWM_SLOW
#define SOFT			PWM_MED

#define LEFT 0
#define RIGHT 1

// time it takes to rotate 90 degrees
#define TURN_90 300

typedef enum {FWD, HARDLEFT, HARDRIGHT, SOFTLEFT, SOFTRIGHT, STOP, MEDLEFT, MEDRIGHT,
              BACK, BACKHLEFT, BACKHRIGHT, BACKSLEFT, BACKSRIGHT, BACKMLEFT, BACKMRIGHT} Command;

extern Command MoveCmd;
extern OS_SemaphoreType CmdReady;

void GoStraight(void);

void TurnRight(int speed);

void TurnLeft(int speed);

void Stop(void);

#endif

