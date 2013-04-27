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
unsigned long Ping_Dist;
Command Decision;
char DEBUGDATA[60];

void pingAction(unsigned long dist, int id)
{
  Ping_Dist = dist;
  if(dist < 40)
	{
    // turn whichever direction is more open
    if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 20 && IR_Dist[IR_RIGHT] > 5 && IR_Dist[IR_RIGHT] < 20 &&
      IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 20 && IR_Dist[IR_LEFT] > 5 && IR_Dist[IR_LEFT] < 20){
        Decision = STOP;
        Stop();
    }
    else if(IR_Dist[IR_RIGHT] > IR_Dist[IR_LEFT]){
      TurnRight(HARD);
      Decision = HARDRIGHT;
//         sprintf(DEBUGDATA, "Hard Right");
    }
    else{
      TurnLeft(HARD);
      Decision = HARDLEFT;
//         sprintf(DEBUGDATA, "Hard Left");
// 		  sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; HardLeft\n", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
// 		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
    }
  }
	else if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 15 && IR_Dist[IR_RIGHT] > 5 && IR_Dist[IR_RIGHT] < 20)
	{
    TurnLeft(HARD);
    Decision = HARDLEFT;
  }
  else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 15 && IR_Dist[IR_LEFT] > 5 && IR_Dist[IR_LEFT] < 20)
	{
    TurnRight(HARD);
    Decision = HARDRIGHT;
  }
  //
	else if(IR_Dist[IR_FRIGHT] > 5 && IR_Dist[IR_FRIGHT] < 20)
	{
    TurnLeft(SOFT);
//     sprintf(DEBUGDATA, "Soft Left");
    Decision = SOFTLEFT;
// 		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftLeft\n", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
// 		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }
  else if(IR_Dist[IR_FLEFT] > 5 && IR_Dist[IR_FLEFT] < 20)
	{
    TurnRight(SOFT);
    Decision = SOFTRIGHT;
//     sprintf(DEBUGDATA, "Soft Right");
// 		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftRight\n", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
// 		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }
/*	else if(IR_Dist[IR_RIGHT] > 5 && IR_Dist[IR_RIGHT] < 20)
	{
    TurnLeft(SOFT);
    Decision = SOFTLEFT;
//     sprintf(DEBUGDATA, "Soft Left");
// 		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftLeft\n", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
// 		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }
  else if(IR_Dist[IR_LEFT] > 5 && IR_Dist[IR_LEFT] < 20)
	{
    TurnRight(SOFT);
    Decision = SOFTRIGHT;
//     sprintf(DEBUGDATA, "Soft Right");
// 		sprintf(DEBUGDATA, "T:%d  PING: %d IR: %d,%d,%d,%d; SoftRight\n", OS_MsTime(), dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT],
// 		         IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
  }*/
  else
	{
    GoStraight();
    Decision = FWD;
//     sprintf(DEBUGDATA, "Straight");
  }
// 	OS_EthernetSendState(DEBUGDATA, dist, IR_Dist, OS_MsTime());
// 	OS_EthernetMailBox_Send(DEBUGDATA,60);
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
  }
}

void State_Sender(void){
  while(1){
    switch(Decision){
      case(FWD):
        sprintf(DEBUGDATA, "FWD");
        break;
      case(HARDLEFT):
        sprintf(DEBUGDATA, "HARDLEFT");
        break;
      case(HARDRIGHT):
        sprintf(DEBUGDATA, "HARDRIGHT");
        break;
      case(SOFTLEFT):
        sprintf(DEBUGDATA, "SOFTLEFT");
        break;
      case(SOFTRIGHT):
        sprintf(DEBUGDATA, "SOFTRIGHT");
        break;
      case(STOP):
        sprintf(DEBUGDATA, "STOP");
        break;
      case(MEDLEFT):
        sprintf(DEBUGDATA, "MEDLEFT");
        break;
      case(MEDRIGHT):
        sprintf(DEBUGDATA, "MEDRIGHT");
        break;
      case(BACK):
        sprintf(DEBUGDATA, "BACK");
        break;
      case(BACKHLEFT):
        sprintf(DEBUGDATA, "BACKHLEFT");
        break;
      case(BACKHRIGHT):
        sprintf(DEBUGDATA, "BACKHRIGHT");
        break;
      case(BACKSLEFT):
        sprintf(DEBUGDATA, "BACKSLEFT");
        break;
      case(BACKSRIGHT):
        sprintf(DEBUGDATA, "BACKSRIGHT");
        break;
      case(BACKMLEFT):
        sprintf(DEBUGDATA, "BACKMLEFT");
        break;
      case(BACKMRIGHT):
        sprintf(DEBUGDATA, "BACKMRIGHT");
        break;
      default:
        sprintf(DEBUGDATA, "WUT");
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
  ADC_Init(10000);
  PingMeasurePD56_Init(&pingAction);
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM1_Init(50000,100);
	OS_AddThread(&moveThread, 128, 3);
  OS_AddThread(&IR_Listener, 128, 3);
  OS_AddThread(&State_Sender, 128, 7);
	//OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
  OS_Add_Periodic_Thread(&PingTriggerPD56, 80, 3);
  OS_Launch(TIME_2MS);
	
	return 0;
}
