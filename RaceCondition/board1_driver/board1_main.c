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
#include "Fuzzy.h"

extern unsigned long ADC_SamplesLost;


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
char Turned_ON_Flag = 0;
unsigned long Turned_ON_Time;

void pingAction(unsigned long dist, int id){
	Ping_Dist = dist;
	PingReady = 0x1;
}

#pragma O0
void Brain(void)
{
	unsigned long left, right;
	left = right = PWM_MED;
  while(1)
	{
		long dright, dleft;
		//OS_bWait(&IR_Ready);
		Fuzzify_All(Ping_Dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT] + 15, IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
		Fuzzy_Compute();
		Defuzzify(&dleft, &dright);
		
		PingReady=0;
		left = MIN(left + dleft, PWM_FAST);
		right = MIN(right + dright, PWM_FAST);
		/*if(left < right)
			right = PWM_FAST;
		else
			left = PWM_FAST;*/
		
		if(Turned_ON_Flag)
		{
			PWM1_SetADuty(left);
			PWM1_SetBDuty(right);
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
		//OS_bSignal(&IR_Ready);
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
    OS_EthernetSendState(DEBUGDATA, Ping_Dist, IR_Dist, OS_MsTime(),ADC_SamplesLost);
		OS_Sleep(50);
  }
}

void Turner_Offer(void){
  while(1){
    if(OS_MsTime() >= (Turned_ON_Time + 180*1000)){
      Turned_ON_Flag = 0;
    }
    else{
      OS_Sleep(1000);
    }
  }
}

void GOGOGO(void){
  Turned_ON_Flag = 1;
  Turned_ON_Time = OS_MsTime();
  OS_AddThread(&Turner_Offer, 128, 7);
	OS_AddThread(&moveThread, 128, 3);
  OS_Kill();
}

int main (void) {
  OS_Init();
	//eFile_Init();
	//SH_Init();
	OS_EthernetInit();
  ADC_Init(2500);
  PingMeasurePD56_Init(&pingAction);
	OS_InitSemaphore(&IR_Ready, 0);
	PingReady = 0;
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM1_Init(50000,100);
  OS_AddButtonTask(&GOGOGO, 0);
  OS_AddThread(&IR_Listener, 128, 0);
  OS_AddThread(&State_Sender, 128, 7);
	OS_AddThread(&Brain, 128, 1);
	//OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
  OS_Add_Periodic_Thread(&PingTriggerPD56, 80, 3);
  OS_Launch(TIME_2MS);
	
	return 0;
}
