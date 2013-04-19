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
	
	printf("\r\nspeed5 = %d, speed7 = %d\r\n\
					\rnewPWM5 = %d, newPWM7 = %d\r\n",
					speed5, speed7, newPWM5, newPWM7);
}


void moveThread(void){
	eFile_RedirectToFile("PIDlog.txt");
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
	OS_Sleep
	eFile_EndRedirectToFile();
	while(1){}
	
}

int main (void) {
  OS_Init();
	eFile_Init();
	//SH_Init();
	
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM0_Init(50000,100);
	tacho_Init();
	OS_AddThread(&moveThread, 128, 3);
  PID_SetConstants(1, 0, 2, 0);
	PID_SetConstants(1, 0, 2 , 1);
 	PID_SetTarget(1500, 0);
	PID_SetTarget(1500, 1);
	OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
  OS_Add_Periodic_Thread(&PID_Controller, 25, 0);
  OS_Launch(TIME_2MS);
	
	return 0;
}




void stupid_thread(void) {
	while(1) {
		;
	}
}
