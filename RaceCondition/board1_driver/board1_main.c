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

void PID_Controller(void){unsigned long newPWM;
	//if(Done5){

	newPWM = PID_Compute(speed5, 0);
	PWM0_SetADuty(newPWM);

	
}

int main (void) {
  OS_Init();
	
	TargetSpdLft = 2000;
  TargetSpdRgt = 2000;	
  PWM0_Init(50000,100);
	tacho_Init();
	OS_AddThread(&stupid_thread, 128, 3);
  PID_SetConstants(1, 0, 2, 0);
	PID_SetConstants(1, 0, 2, 1);
 	PID_SetTarget(1000, 0);
	PID_SetTarget(1000, 1);
  OS_Add_Periodic_Thread(&PID_Controller, 50, 0);
  OS_Launch(TIME_2MS);
	
	return 0;
}

void stupid_thread(void) {
	while(1) {
		;
	}
}
