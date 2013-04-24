#ifndef __CMD_h__
#define __CMD_h__
#include "OS.h"

typedef enum {FWD, HARDLEFT, HARDRIGHT, SOFTLEFT, SOFTRIGHT, STOP, MEDLEFT, MEDRIGHT,
              BACK, BACKHLEFT, BACKHRIGHT, BACKSLEFT, BACKSRIGHT, BACKMLEFT, BACKMRIGHT} Command;

extern Command MoveCmd;
extern OS_SemaphoreType CmdReady;

#endif

