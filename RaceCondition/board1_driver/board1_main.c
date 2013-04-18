#include "OS.h"
#include "shell.h"
#include "tachometerTimer1.h"
#include "src/pwm.h"
#include "pid.h"
// board controlling the two motors
#define PWMPERIODMAX 24999
#define PWMPERIODMIN 3
short LeftDuty;
short rightDuty;
unsigned long TargetSpdLft;
unsigned long TargetSpdRgt;

void stupid_thread(void);

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
	
}


void moveThread(void){
	PID_SetTarget(1500, 0);
	PID_SetTarget(1500, 1);
	OS_Sleep(800);
	PID_SetTarget(500, 0);
	OS_Sleep(1000);
	PID_SetTarget(1500, 0);
	while(1){}
	
}

int main (void) {
  OS_Init();
	
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM0_Init(50000,100);
	tacho_Init();
	OS_AddThread(&moveThread, 128, 3);
  PID_SetConstants(2, 0, 2, 0);
	PID_SetConstants(2, 0, 2, 1);
 	PID_SetTarget(1000, 0);
	PID_SetTarget(1000, 1);
  OS_Add_Periodic_Thread(&PID_Controller, 100, 0);
  OS_Launch(TIME_2MS);
	
	return 0;
}




void stupid_thread(void) {
	while(1) {
		;
	}
}
