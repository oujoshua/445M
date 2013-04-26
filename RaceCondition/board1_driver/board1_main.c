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

void pingAction(unsigned long dist, int id)
{  char DEBUGDATA[60];
  if(dist < 90)
	{
    // turn whichever direction is more open
    if(IR_Dist[0] > IR_Dist[1]){
      TurnRight(HARD);
		  sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; HardRight", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
		}
    else{
      TurnLeft(HARD);
		  sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; HardLeft", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
		}
  }
	else if(IR_Dist[IR_FRIGHT] > 10 && IR_Dist[IR_FRIGHT] < 20)
	{
    TurnLeft(SOFT);
		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftLeft", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }
  else if(IR_Dist[IR_FLEFT] > 10 && IR_Dist[IR_FLEFT] < 20)
	{
    TurnRight(SOFT);
		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftRight", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }
	else if(IR_Dist[IR_RIGHT] > 10 && IR_Dist[IR_RIGHT] < 20)
	{
    TurnLeft(SOFT);
		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftLeft", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }
  else if(IR_Dist[IR_LEFT] > 10 && IR_Dist[IR_LEFT] < 20)
	{
    TurnRight(SOFT);
		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftRight", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }
  else
	{
    GoStraight();
  }
	
	OS_EthernetMailBox_Send(DEBUGDATA,60);
}

// ADC0 is the (angle) right IR
// ADC1 is the (angle) left IR
void IR_Listener(void) {
  while(1) {
    ADC_Mailbox_Receive(IR_Samples);
    IR_Dist[0] = IR_Distance(IR_Samples[0]);
    IR_Dist[1] = IR_Distance(IR_Samples[1]);
  }
}

int main (void) {
  OS_Init();
	//eFile_Init();
	//SH_Init();
	OS_EthernetInit();
  ADC_Init(10000);
  PingMeasurePD56_Init(&pingAction);
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM1_Init(50000,100);
	OS_AddThread(&moveThread, 128, 3);
  OS_AddThread(&IR_Listener, 128, 3);
	OS_AddThread(&OS_EthernetSender, 128, 4);
	//OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
  OS_Add_Periodic_Thread(&PingTriggerPD56, 80, 3);
  OS_Launch(TIME_2MS);
	
	return 0;
}
