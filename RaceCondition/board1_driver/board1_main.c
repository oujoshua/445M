#include "OS.h"
#include "eDisk.h"
#include "eFile.h"
#include "shell.h"
#include "tachometerTimer1.h"
#include "src/pwm.h"
#include "pid.h"
#include "shell.h"
#include "stdio.h"
#include "commands.h"
#include "OS_ethernet.h"
// board controlling the two motors
#define PWMPERIODMAX 24999
#define PWMPERIODMIN 3

// duty cycles
#define PWM_FAST 40000 
#define PWM_MED  30000
#define PWM_SLOW 20000

// time it takes to rotate 90 degrees
#define TURN_90 500

short LeftDuty;
short rightDuty;
Command MoveCmd;
OS_SemaphoreType CmdReady;
unsigned long TargetSpdLft;
unsigned long TargetSpdRgt;

void stupid_thread(void);

void GoStraight(void) {
  PWM0_SetADuty(PWM_FAST);
  PWM0_SetBDuty(PWM_FAST);
}

void TurnRight(void) {
  PWM0_SetADuty(PWM_FAST);
  PWM0_SetBDuty(PWM_SLOW);
}

void TurnLeft(void) {
  PWM0_SetADuty(PWM_SLOW);
  PWM0_SetBDuty(PWM_FAST);
}

void Stop() {
  PWM0_SetADuty(0);
  PWM0_SetBDuty(0);
}

void PID_Controller(void){unsigned long newPWM5, newPWM7;
	//if(Done5){
  if(Done5){
	newPWM5 = PID_Compute(speed5, 0);
	PWM0_SetADuty(newPWM5);
	}else{
		newPWM5 = PID_Compute(0, 0);
	PWM0_SetADuty(newPWM5);
	}
	if(Done7){
	newPWM7 = PID_Compute(speed7, 1);
  PWM0_SetBDuty(newPWM7);
	}else{
		newPWM7 = PID_Compute(0, 1);
  PWM0_SetBDuty(newPWM7);
	}
	
	printf("\r\nspeed5 = %d, speed7 = %d\r\n\
					\rnewPWM5 = %d, newPWM7 = %d\r\n",
					speed5, speed7, newPWM5, newPWM7);
}


void moveThread(void){unsigned long stoptime;
	
	stoptime = OS_MsTime() + (1000)*(180); //stop after 3 minutes(180 seconds)
	eFile_RedirectToFile("PIDlog.txt");
	
	/*
	OS_Sleep(2000);
	PWM0_SetADuty(30000);
	PWM0_SetBDuty(30000);
	//PID_SetTarget(500, 1);
	OS_Sleep(500);
	PWM0_SetADuty(20000);
	//PID_SetTarget(1500, 1);
	OS_Sleep(750);
	PWM0_SetADuty(30000);
	//PID_SetTarget(500, 0);
	OS_Sleep(500);
	//PID_SetTarget(1500,0);
	PWM0_SetBDuty(20000);
	OS_Sleep(2000);
	PWM0_SetBDuty(30000);
	//PID_SetTarget(0, 0);
	//PID_SetTarget(0,1);
	OS_Sleep(1000);
	
	*/
	
	while(OS_MsTime() < stoptime){
 //wait for new instruction
   OS_bWait(&CmdReady);
 
		//set target motor speeds based on instruction
		//probably with case statement
		switch (MoveCmd){
			case FWD:{
				PID_SetTarget(2000, 0);
				PID_SetTarget(2000, 1);
				break;
			}
			case HARDLEFT:{
				PID_SetTarget(1000, 0);
				PID_SetTarget(2000, 1);
				break;
			}
			case HARDRIGHT:{
				PID_SetTarget(2000, 0);
				PID_SetTarget(1000, 1);
				break;
			}
			case SOFTLEFT:{
				PID_SetTarget(1800, 0);
				PID_SetTarget(2000, 1);
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
	eFile_EndRedirectToFile();
	while(1){}
	
}
int main (void) {
  OS_Init();
	eFile_Init();
	//SH_Init();
	OS_EthernetInit();
  OS_InitSemaphore(&CmdReady, 1);
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM0_Init(50000,100);
	tacho_Init();
	OS_AddThread(&moveThread, 128, 3);
	OS_AddThread(&OS_EthernetListener, 128, 3);
  PID_SetConstants(1, 0, 2, 0);
	PID_SetConstants(1, 0, 2 , 1);
 	PID_SetTarget(1500, 0);
	PID_SetTarget(1500, 1);
	OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
  OS_Add_Periodic_Thread(&PID_Controller, 25, 0);
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
