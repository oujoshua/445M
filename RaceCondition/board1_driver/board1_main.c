#include "OS.h"
#include "eDisk.h"
#include "eFile.h"
#include "shell.h"
//#include "tachometerTimer1.h"
#include "src/ADC.h"
#include "IR.h"
#include "src/pwm.h"
#include "pid.h"
#include "shell.h"
#include "stdio.h"
#include "commands.h"
#include "OS_ethernet.h"
#include "PingMeasure.h"
// board controlling the two motors
#define PWMPERIODMAX 24999
#define PWMPERIODMIN 3

// duty cycles
#define PWM_FAST 40000 
#define PWM_MED  30000
#define PWM_SLOW 20000

#define LEFT 0
#define RIGHT 1

// time it takes to rotate 90 degrees
#define TURN_90 300

short LeftDuty;
short rightDuty;
unsigned long TargetSpdLft;
unsigned long TargetSpdRgt;

void stupid_thread(void);

void GoStraight(void) {
  PWM1_SetADuty(PWM_FAST);
  PWM1_SetBDuty(PWM_FAST);
}

void TurnRight(void) {
  PWM1_SetADuty(PWM_FAST);
  PWM1_SetBDuty(PWM_SLOW);
}

void TurnLeft(void) {
  PWM1_SetADuty(PWM_SLOW);
  PWM1_SetBDuty(PWM_FAST);
}

void Stop() {
  PWM1_SetADuty(0);
  PWM1_SetBDuty(0);
}

void PID_Controller(void){unsigned long newPWM5, newPWM7;
	//if(Done5){
  /*
  if(Done5){
	newPWM5 = PID_Compute(speed5, 0);
	PWM1_SetADuty(newPWM5);
	}else{
		newPWM5 = PID_Compute(0, 0);
	PWM1_SetADuty(newPWM5);
	}
	if(Done7){
	newPWM7 = PID_Compute(speed7, 1);
  PWM1_SetBDuty(newPWM7);
	}else{
		newPWM7 = PID_Compute(0, 1);
  PWM1_SetBDuty(newPWM7);
	}*/
	
// 	printf("\r\nspeed5 = %d, speed7 = %d\r\n\
// 					\rnewPWM5 = %d, newPWM7 = %d\r\n",
// 					speed5, speed7, newPWM5, newPWM7);
}


void moveThread(void){
  unsigned long start = OS_MsTime();
// 	eFile_RedirectToFile("PIDlog.txt");
  PID_Enable(0);
  PID_Enable(1);
  PWM1_SetADuty(49790);
  PWM1_SetBDuty(49990);
  while(1){}
  OS_Sleep(5000);
  Stop();
//   // turn left
//   PID_SetTarget(800, LEFT);
//   OS_Sleep(300);
//   // go straight
//   PID_SetTarget(1000, LEFT);
//   OS_Sleep(2000);
  PID_Disable(0);
  PID_Disable(1);
  while(1) {
  }
// 	eFile_EndRedirectToFile();
}

/*
void moveThread(void){unsigned long stoptime;
	PID_Enable(0);
  PID_Enable(1);
	stoptime = OS_MsTime() + (1000)*(180); //stop after 3 minutes(180 seconds)
//	eFile_RedirectToFile("PIDlog.txt");
	
	while(OS_MsTime() < stoptime){
 //wait for new instruction
   OS_bWait(&CmdReady);
 
		//set target motor speeds based on instruction
		//probably with case statement
		switch (MoveCmd){
			case FWD:{
				PID_SetTarget(1200, 0);
				PID_SetTarget(1200, 1);
				break;
			}
			case HARDLEFT:{
				PID_SetTarget(800, 0);
				PID_SetTarget(1200, 1);
				break;
			}
			case HARDRIGHT:{
				PID_SetTarget(2000, 0);
				PID_SetTarget(1000, 1);
				break;
			}
			case SOFTLEFT:{
				PID_SetTarget(1000, 0);
				PID_SetTarget(1200, 1);
				break;
			}
			case SOFTRIGHT:{
				PID_SetTarget(2000, 0);
				PID_SetTarget(1800, 1);
				break;
			}
			case MEDLEFT:{
				PID_SetTarget(1500, 0);
				PID_SetTarget(2000, 1);
				break;
			}
			case MEDRIGHT:{
				PID_SetTarget(2000, 0);
				PID_SetTarget(1500, 1);
				break;
			}
			case STOP:{
				PID_SetTarget(0, 0);
				PID_SetTarget(0, 1);
				break;				
			}
			default:{
				
				break;
			}
			
		}
		
		
	}
	//stop running when time is up
	Stop();
// 	eFile_EndRedirectToFile();
	while(1){}
	
}
*/

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

unsigned short IR_Samples[4] = {0,};
long IR_Dist = 200;

#pragma O0
void pingAction(unsigned long dist, int id){
  if(dist < 70 || (IR_Dist > 10 && IR_Dist < 25)){
    PWM1_SetADuty(9000);
    PWM1_SetBDuty(35000);
  }
  else {
    PWM1_SetADuty(30000);
    PWM1_SetBDuty(30000);
  }
}


void IR_Listener(void) {
  while(1) {
    ADC_Mailbox_Receive(IR_Samples);
    IR_Dist = IR_Distance(IR_Samples[0]);
  }
}

int main (void) {
  OS_Init();
	eFile_Init();
	//SH_Init();
	//OS_EthernetInit();
  ADC_Init(10000);
  OS_InitSemaphore(&CmdReady, 1);
  PingMeasurePD56_Init(&pingAction);
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM1_Init(50000,100);
	//tacho_Init();
	OS_AddThread(&moveThread, 128, 3);
  OS_AddThread(&IR_Listener, 128, 3);
	//OS_AddThread(&OS_EthernetListener, 128, 3);
//   OS_AddThread(&sendCMDS, 128, 3);
  PID_SetConstants(1, 0, 2, 0);
	PID_SetConstants(1, 0, 2 , 1);
 	PID_SetTarget(1000, 0);
	PID_SetTarget(1000, 1);
	OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
  OS_Add_Periodic_Thread(&PingTriggerPD56, 80, 3);
//   OS_Add_Periodic_Thread(&PID_Controller, 25, 0);
  OS_Launch(TIME_2MS);
	
	return 0;
}

void stupid_move_thread(void) {
  
  GoStraight();
  OS_Sleep(4000);
  TurnLeft();
  OS_Sleep(TURN_90);
  GoStraight();
  OS_Sleep(1000);
  TurnRight();
  OS_Sleep(TURN_90);
  GoStraight();
  OS_Sleep(2000);
  Stop();
  
  while(1) {
    ;
  }
}




void stupid_thread(void) {
	while(1) {
		;
	}
}
