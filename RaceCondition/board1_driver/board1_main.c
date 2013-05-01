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
#include <string.h>

extern unsigned long ADC_SamplesLost;

short LeftDuty;
short rightDuty;
unsigned long TargetSpdLft;
unsigned long TargetSpdRgt;
volatile char PingReady;
OS_SemaphoreType IR_Ready;

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
unsigned short IR_Samples[3][4] = {0,};
long IR_Dist[4] = {200, };
unsigned long Ping_Dist;
Command Decision;
char ifelse_block;
char DEBUGDATA[60];
char Turned_ON_Flag = 0;
char Done_Flag = 0;
unsigned long Turned_ON_Time;

void pingAction(unsigned long dist, int id){
	Ping_Dist = dist;
	PingReady = 0x1;
}

#pragma O0
void Brain(void)
{
	unsigned long left, right;
	long FIR_Dist;
	left = right = PWM_MED;
  while(1)
	{
		long dright, dleft;
		char set;
		//OS_bWait(&IR_Ready);
		FIR_Dist = OS_EthernetReceiveNonBlocking();	
			Ping_Dist = (FIR_Dist > Ping_Dist)? Ping_Dist:FIR_Dist;
		
		Fuzzify_All(Ping_Dist, IR_Dist[IR_LEFT], IR_Dist[IR_FLEFT], IR_Dist[IR_FRIGHT], IR_Dist[IR_RIGHT]);
		Fuzzy_Compute();
		Defuzzify(&dleft, &dright, &set);
		
		PingReady=0;
		left = MIN(left + dleft, PWM_FAST);
		right = MIN(right + dright, PWM_FAST);
		if(left < right)
			right = PWM_FAST;
		else
			left = PWM_FAST;
		
		if(Turned_ON_Flag)
		{
			if(set == 1)
			{
				left = right = ((PWM_FAST+ (2*MIN(left,right)))/3);
			}
			PWM1_SetADuty(left);
			PWM1_SetBDuty(right);
		}
		else if(Done_Flag == 1)
		{
			PWM1_SetADuty(1);
			PWM1_SetBDuty(1);
		}
		myState.state.fwd = Fuzzy_Output.goStraight;
		myState.state.turn_left = Fuzzy_Output.turnLeft;
		myState.state.turn_right = Fuzzy_Output.turnRight;
		myState.state.PWM_left = left;
		myState.state.PWM_right = right;
		memcpy(myState.state.IRs, IR_Dist, sizeof(unsigned long)*4);
		myState.state.ping = Ping_Dist;
		myState.state.time = OS_Time();
		myState.state.lost = ADC_SamplesLost;
		memcpy(myState.byteArr, &myState.state, sizeof(Fuzzy_State));
  }
}
long Median_Of_3(long a, long b, long c){
  long middle;
  if ((a <= b) && (a <= c)){
    middle = (b <= c) ? b : c;
  }
  else if ((b <= a) && (b <= c)){
    middle = (a <= c) ? a : c;
   }
  else{
    middle = (a <= b) ? a : b;
  }
  return middle;
}

// ADC0 is the (angle) right IR
// ADC1 is the (angle) left IR
void IR_Listener(void) {
  char count = 0;
  while(1) {
     ADC_Mailbox_Receive(&IR_Samples[count][0]);
//    if(count == 2){
      IR_Dist[IR_LEFT] = IR_Distance(Median_Of_3(IR_Samples[0][IR_LEFT], IR_Samples[1][IR_LEFT], IR_Samples[2][IR_LEFT]));
      IR_Dist[IR_RIGHT] = IR_Distance(Median_Of_3(IR_Samples[0][IR_RIGHT], IR_Samples[1][IR_RIGHT], IR_Samples[2][IR_RIGHT]));
       IR_Dist[IR_FLEFT] = IR_Distance(Median_Of_3(IR_Samples[0][IR_FLEFT], IR_Samples[1][IR_FLEFT], IR_Samples[2][IR_FLEFT]));
      IR_Dist[IR_FRIGHT] = IR_Distance(Median_Of_3(IR_Samples[0][IR_FRIGHT], IR_Samples[1][IR_FRIGHT], IR_Samples[2][IR_FRIGHT]));
 //    }
    count = (count + 1) % 3;
        //OS_bSignal(&IR_Ready);
  }
}


void State_Sender(void){
  while(1){
    OS_EthernetSendState();
		OS_Sleep(50);
  }
}

void Turner_Offer(void){
  while(1){
    if(OS_MsTime() >= (Turned_ON_Time + 180*1000)){
      Turned_ON_Flag = 0;
      Done_Flag = 1;
    }
    else{
      OS_Sleep(500);
    }
  }
}
#pragma O0
void GOGOGO(void){
  int speed = PWM_SLOW;
  Turned_ON_Time = OS_MsTime();
  OS_AddThread(&Turner_Offer, 128, 7);
  while(speed < PWM_FAST){
    PWM1_SetADuty(speed);
    PWM1_SetBDuty(speed);
    speed = speed + 2000;
    OS_Sleep(25);
  }
  Turned_ON_Flag = 1;
  Done_Flag = 0;
  OS_Kill();
}

int main (void) {
  OS_Init();
	//eFile_Init();
	//SH_Init();
	OS_EthernetInit();
  ADC_Init(833);
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
