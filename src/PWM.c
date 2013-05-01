// PWM.c
// Runs on LM3S811
// Use PWM0 to generate a 100 Hz square wave with 50% duty cycle.
// Daniel Valvano
// June 27, 2011
#include "lm3s8962.h"

#define PB_PINS 0x3
#define MAX 250
#define DELTA(A,B) (A>B ? A-B : B-A)
#define CAP(PREV,NEW) (NEW>PREV ? PREV+MAX : PREV-MAX)

// period is number of PWM clock cycles in one period (3<=period)
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
// PWM clock rate = processor clock rate/SYSCTL_RCC_PWMDIV
//                = 6 MHz/2 = 3 MHz (in this example)
#pragma O0
void PWM0_Init(unsigned short period, unsigned short duty){
  volatile unsigned long delay;
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM;   // activate PWM
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // activate port F
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOG;
  delay = SYSCTL_RCGC2_R;          // allow time to finish activating
  GPIO_PORTF_DEN_R |= 0x01;
	GPIO_PORTF_AFSEL_R |= 0x01;      // enable alt funct on PD0
	GPIO_PORTG_DEN_R   |= 0x02;
	GPIO_PORTG_AFSEL_R |= 0x02;
  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; // use PWM divider
  SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M; // clear PWM divider field
  SYSCTL_RCC_R += SYSCTL_RCC_PWMDIV_2;  // configure for /2 divider
  PWM_0_CTL_R = 0;                 // countdown mode
                                   // 1: match compare value counting down
                                   // 0: re-loading
  PWM_0_GENA_R = (PWM_X_GENA_ACTCMPAD_ONE|PWM_X_GENA_ACTLOAD_ZERO);
	PWM_0_GENB_R = (PWM_X_GENB_ACTCMPBD_ONE|PWM_X_GENB_ACTLOAD_ZERO);
  PWM_0_LOAD_R = period - 1;       // cycles needed to count down to 0
  PWM_0_CMPA_R = duty - 1;         // count value when output rises
	PWM_0_CMPB_R = duty - 1;
  PWM_0_CTL_R |= PWM_X_CTL_ENABLE; // start PWM0
  PWM_ENABLE_R |= PWM_ENABLE_PWM0EN|PWM_ENABLE_PWM1EN;    // enable PWM0
	
}

#pragma O0
void PWM1_Init(unsigned short period, unsigned short duty) {
  volatile unsigned long delay;
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM;   // activate PWM
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // activate port B
  delay = SYSCTL_RCGC2_R;          // allow time to finish activating
  // PWM 2 and 3 are on PB0 and PB1
  GPIO_PORTB_DEN_R |= PB_PINS;
  GPIO_PORTB_AFSEL_R |= PB_PINS;
  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; // use PWM divider
  SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M; // clear PWM divider field
  SYSCTL_RCC_R += SYSCTL_RCC_PWMDIV_2;  // configure for /2 divider
  PWM_1_CTL_R = 0;                 // countdown mode
                                   // 1: match compare value counting down
                                   // 0: re-loading
  PWM_1_GENA_R = (PWM_X_GENA_ACTCMPAD_ONE|PWM_X_GENA_ACTLOAD_ZERO);
	PWM_1_GENB_R = (PWM_X_GENB_ACTCMPBD_ONE|PWM_X_GENB_ACTLOAD_ZERO);
  PWM_1_LOAD_R = period - 1;       // cycles needed to count down to 0
  PWM_1_CMPA_R = duty - 1;         // count value when output rises
	PWM_1_CMPB_R = duty - 1;
  PWM_1_CTL_R |= PWM_X_CTL_ENABLE; // start PWM1
  PWM_ENABLE_R |= PWM_ENABLE_PWM2EN|PWM_ENABLE_PWM3EN;    // enable PWM2 and 3
}

void PWM0_SetADuty(unsigned short newDuty){
  PWM_0_CMPA_R = newDuty - 1;          // count value when output may change
}

void PWM0_SetBDuty(unsigned short newDuty){
  PWM_0_CMPB_R = newDuty - 1;
}

void PWM1_SetADuty(unsigned short newDuty) {
	/*static unsigned short prev = 0;
	if(DELTA(prev, newDuty) > MAX)
		newDuty = CAP(prev, newDuty);
	prev = newDuty;*/
  PWM_1_CMPA_R = newDuty - 1;
}

void PWM1_SetBDuty(unsigned short newDuty) {
	/*static unsigned short prev = 0;
	if(DELTA(prev, newDuty) > MAX)
		newDuty = CAP(prev, newDuty);
	prev = newDuty;*/
  PWM_1_CMPB_R = newDuty - 1;
}
