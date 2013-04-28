#include "OS.h"
#include "eDisk.h"
#include "eFile.h"
#include "shell.h"
#include "src/ADC.h"
#include "IR.h"
#include "src/pwm.h"
#include "shell.h"
#include "stdio.h"
#include "commands.h"
#include "OS_ethernet.h"
#include "PingMeasure.h"

short LeftDuty;
short rightDuty;
unsigned long TargetSpdLft;
unsigned long TargetSpdRgt;
volatile char PingReady;
OS_SemaphoreType IR_Ready;
void moveThread(void){
  unsigned long start = OS_MsTime();
// 	eFile_RedirectToFile("PIDlog.txt");
  PWM1_SetADuty(49790);
  PWM1_SetBDuty(49990);
  while(1);
// 	eFile_EndRedirectToFile();
}

/*
void sendCMDS(void){
  MoveCmd = FWD;
  OS_bSignal(&CmdReady);
  OS_Sleep(1000);
  MoveCmd = SOFTLEFT;
  OS_bSignal(&CmdReady);
  OS_Sleep(1000);
  MoveCmd = FWD;
  OS_bSignal(&CmdReady);
  OS_Sleep(1000);
  MoveCmd = STOP; 
  OS_Kill();
}
*/
unsigned short IR_Samples[4] = {0,};
long IR_Dist[4] = {200, };
unsigned long Ping_Dist;
Command Decision;
char ifelse_block;
char DEBUGDATA[60];




void pingAction(unsigned long dist, int id){
	Ping_Dist = dist;
	PingReady = 0x1;
}

void Brain(void)
{
	while(1){
	OS_Wait(&IR_Ready);
	if(PingReady){
		PingReady = 0;
		if(Ping_Dist < 55 || IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 25 && IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 25)
		{
			// turn whichever direction is more open
			if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 20 && IR_Dist[IR_RIGHT] > 5 && IR_Dist[IR_RIGHT] < 20 &&
				IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 20 && IR_Dist[IR_LEFT] > 5 && IR_Dist[IR_LEFT] < 20){
					Decision = STOP;
					ifelse_block = 0;
					Stop();
			}
			else if(IR_Dist[IR_RIGHT] > IR_Dist[IR_LEFT]){
				if(Ping_Dist < 35)
				{
					TurnRight(HARD);
					Decision = HARDRIGHT;
					ifelse_block = 1;
				}
				else
				{
					TurnRight(MED);
					Decision = MEDRIGHT;
					ifelse_block = 2;
				}
			}
			else{
				if(Ping_Dist < 35)
				{
					TurnLeft(HARD);
					Decision = HARDLEFT;
					ifelse_block = 3;
				}
				else
				{
					TurnLeft(MED);
					Decision = MEDLEFT;
					ifelse_block = 4;
				}
			}
		}
		else if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 18 && IR_Dist[IR_RIGHT] > 5 && IR_Dist[IR_RIGHT] < 20)
		{
			TurnLeft(HARD);
			Decision = HARDLEFT;
			ifelse_block = 5;
		}
		else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 18 && IR_Dist[IR_LEFT] > 5 && IR_Dist[IR_LEFT] < 20)
		{
			TurnRight(HARD);
			Decision = HARDRIGHT;
			ifelse_block = 6;
		}
		else if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 23)
		{
			TurnLeft(MED);
			Decision = MEDLEFT;
			ifelse_block = 7;
		}
		else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 23)
		{
			TurnRight(MED);
			Decision = MEDRIGHT;
			ifelse_block = 8;
		}
		else if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 28)
		{
			TurnLeft(SOFT);
			Decision = SOFTLEFT;
			ifelse_block = 9;
		}
		else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 28)
		{
			TurnRight(SOFT);
			Decision = SOFTRIGHT;
			ifelse_block = 10;
		}
// 		else if(IR_Dist[IR_RIGHT] > 50 /*&& IR_Dist[IR_LEFT] < 8*/){
// 			TurnRight(MED);
// 			Decision = MEDRIGHT;
// 			ifelse_block = 11;
// 		}
// 		else if(IR_Dist[IR_LEFT] > 50 /*&& IR_Dist[IR_RIGHT] < 8*/){
// 			TurnLeft(MED);
// 			Decision = MEDLEFT;
// 			ifelse_block = 12;
// 		}
		else
		{
			GoStraight();
			Decision = FWD;
			ifelse_block = 13;
		}
  }
	else{ //if no new ping data, but new IR data
		if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 18 && IR_Dist[IR_RIGHT] > 5 && IR_Dist[IR_RIGHT] < 20)
		{
			TurnLeft(HARD);
			Decision = HARDLEFT;
			ifelse_block = 14;
		}
		else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 18 && IR_Dist[IR_LEFT] > 5 && IR_Dist[IR_LEFT] < 20)
		{
			TurnRight(HARD);
			Decision = HARDRIGHT;
			ifelse_block = 15;
		}
		else if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 23)
		{
			TurnLeft(MED);
			Decision = MEDLEFT;
			ifelse_block = 16;
		}
		else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 23)
		{
			TurnRight(MED);
			Decision = MEDRIGHT;
			ifelse_block = 17;
		}
		else if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 28)
		{
			TurnLeft(SOFT);
			Decision = SOFTLEFT;
			ifelse_block = 18;
		}
		else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 28)
		{
			TurnRight(SOFT);
			Decision = SOFTRIGHT;
			ifelse_block = 19;
		}

// 		else if(IR_Dist[IR_RIGHT] > 50 /*&& IR_Dist[IR_LEFT] < 8*/){
// 			TurnRight(MED);
// 			Decision = MEDRIGHT;
// 			ifelse_block = 20;
// 		}
// 		else if(IR_Dist[IR_LEFT] > 50 /*&& IR_Dist[IR_RIGHT] < 8*/){
// 			TurnLeft(MED);
// 			Decision = MEDLEFT;
// 			ifelse_block = 21;
// 		}
		else
		{
			GoStraight();
			Decision = FWD;
			ifelse_block = 22;
		}
	}
}
}

// ADC0 is the (angle) right IR
// ADC1 is the (angle) left IR
void IR_Listener(void) {
  while(1) {
    ADC_Mailbox_Receive(IR_Samples);
    IR_Dist[IR_LEFT] = IR_Distance(IR_Samples[IR_LEFT]);
    IR_Dist[IR_RIGHT] = IR_Distance(IR_Samples[IR_RIGHT]);
    IR_Dist[IR_FLEFT] = IR_Distance(IR_Samples[IR_FLEFT]);
    IR_Dist[IR_FRIGHT] = IR_Distance(IR_Samples[IR_FRIGHT]);
		OS_Signal(&IR_Ready);
  }
}

void State_Sender(void){
  while(1){
    switch(Decision){
      case(FWD):
        sprintf(DEBUGDATA, "FWD, %d", ifelse_block);
        break;
      case(HARDLEFT):
        sprintf(DEBUGDATA, "HARDLEFT, %d", ifelse_block);
        break;
      case(HARDRIGHT):
        sprintf(DEBUGDATA, "HARDRIGHT, %d", ifelse_block);
        break;
      case(SOFTLEFT):
        sprintf(DEBUGDATA, "SOFTLEFT, %d", ifelse_block);
        break;
      case(SOFTRIGHT):
        sprintf(DEBUGDATA, "SOFTRIGHT, %d", ifelse_block);
        break;
      case(STOP):
        sprintf(DEBUGDATA, "STOP, %d", ifelse_block);
        break;
      case(MEDLEFT):
        sprintf(DEBUGDATA, "MEDLEFT, %d", ifelse_block);
        break;
      case(MEDRIGHT):
        sprintf(DEBUGDATA, "MEDRIGHT, %d", ifelse_block);
        break;
      case(BACK):
        sprintf(DEBUGDATA, "BACK, %d", ifelse_block);
        break;
      case(BACKHLEFT):
        sprintf(DEBUGDATA, "BACKHLEFT, %d", ifelse_block);
        break;
      case(BACKHRIGHT):
        sprintf(DEBUGDATA, "BACKHRIGHT, %d", ifelse_block);
        break;
      case(BACKSLEFT):
        sprintf(DEBUGDATA, "BACKSLEFT, %d", ifelse_block);
        break;
      case(BACKSRIGHT):
        sprintf(DEBUGDATA, "BACKSRIGHT, %d", ifelse_block);
        break;
      case(BACKMLEFT):
        sprintf(DEBUGDATA, "BACKMLEFT, %d", ifelse_block);
        break;
      case(BACKMRIGHT):
        sprintf(DEBUGDATA, "BACKMRIGHT, %d", ifelse_block);
        break;
      default:
        sprintf(DEBUGDATA, "WUT, %d", ifelse_block);
        break;
    }
    OS_EthernetSendState(DEBUGDATA, Ping_Dist, IR_Dist, OS_MsTime());
  }
}

int main (void) {
  OS_Init();
	//eFile_Init();
	//SH_Init();
	OS_EthernetInit();
  ADC_Init(7500);
  PingMeasurePD56_Init(&pingAction);
	OS_InitSemaphore(&IR_Ready, 0);
	PingReady = 0;
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM1_Init(50000,100);
	OS_AddThread(&moveThread, 128, 3);
  OS_AddThread(&IR_Listener, 128, 0);
  OS_AddThread(&State_Sender, 128, 7);
	OS_AddThread(&Brain, 128, 1);
	//OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
  OS_Add_Periodic_Thread(&PingTriggerPD56, 80, 3);
  OS_Launch(TIME_2MS);
	
	return 0;
}
