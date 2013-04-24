#include "OS.h"
#include "eDisk.h"
#include "eFile.h"
#include "shell.h"
#include "tachometerTimer1.h"
#include "src/pwm.h"
#include "pid.h"
#include "shell.h"
#include "stdio.h"
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
	}
	
// 	printf("\r\nspeed5 = %d, speed7 = %d\r\n\
// 					\rnewPWM5 = %d, newPWM7 = %d\r\n",
// 					speed5, speed7, newPWM5, newPWM7);
}


void moveThread(void){
  unsigned long start = OS_MsTime();
// 	eFile_RedirectToFile("PIDlog.txt");
  PID_Enable(0);
   PID_Enable(1);
  OS_Sleep(10000);
  PID_Disable(0);
  PID_Disable(1);
// 	eFile_EndRedirectToFile();
	while(1){}
	
}

int main (void) {
  OS_Init();
	eFile_Init();
// 	SH_Init();
	
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM1_Init(50000,100);
	tacho_Init();
	OS_AddThread(&moveThread, 128, 3);
  PID_SetConstants(1, 0, 2, 0);
	PID_SetConstants(1, 0, 2 , 1);
 	PID_SetTarget(1000, 0);
	PID_SetTarget(1000, 1);
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
