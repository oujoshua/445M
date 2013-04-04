// PingMeasure.c
// Runs on LM3S8962
// Use Timer0A in edge time mode to request interrupts on the rising
// edge of PD5 (CCP0), and measure period between pulses.
// Daniel Valvano
// June 27, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011

 Copyright 2011 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// external signal connected to PD5 (CCP0) (trigger on rising edge)
#include "OS.h"
#include "lm3s8962.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "PingMeasure.h"


long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value

/*
#define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))  // IRQ 0 to 31 Set Enable Register
#define NVIC_PRI4_R             (*((volatile unsigned long *)0xE000E410))  // IRQ 16 to 19 Priority Register
#define TIMER0_CFG_R            (*((volatile unsigned long *)0x40030000))
#define TIMER0_TAMR_R           (*((volatile unsigned long *)0x40030004))
#define TIMER0_CTL_R            (*((volatile unsigned long *)0x4003000C))
#define TIMER0_IMR_R            (*((volatile unsigned long *)0x40030018))
#define TIMER0_ICR_R            (*((volatile unsigned long *)0x40030024))
#define TIMER0_TAILR_R          (*((volatile unsigned long *)0x40030028))
#define TIMER0_TAR_R            (*((volatile unsigned long *)0x40030048))
#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_TAMR_TACMR        0x00000004  // GPTM TimerA Capture Mode
#define TIMER_TAMR_TAMR_CAP     0x00000003  // Capture mode
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_CTL_TAEVENT_POS   0x00000000  // Positive edge
#define TIMER_IMR_CAEIM         0x00000004  // GPTM CaptureA Event Interrupt
                                            // Mask
#define TIMER_ICR_CAECINT       0x00000004  // GPTM CaptureA Event Interrupt
                                            // Clear
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low
#define GPIO_PORTC_DATA_R       (*((volatile unsigned long *)0x400063FC))
#define GPIO_PORTC_DIR_R        (*((volatile unsigned long *)0x40006400))
#define GPIO_PORTC_DEN_R        (*((volatile unsigned long *)0x4000651C))
#define GPIO_PORTD_AFSEL_R      (*((volatile unsigned long *)0x40007420))
#define GPIO_PORTD_DEN_R        (*((volatile unsigned long *)0x4000751C))
#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC1_TIMER0     0x00010000  // timer 0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOD      0x00000008  // port D Clock Gating Control
#define SYSCTL_RCGC2_GPIOC      0x00000004  // port C Clock Gating Control
*/

#define PD6 (*((volatile unsigned long *)0x40007100))
#define PD5 (*((volatile unsigned long *)0x40007040))

#define INCHDIV 148 //constant for conversion to inches
#define CMDIV 58 //constant for conversion to centimeters


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

void (*userTask)(unsigned long distance);

unsigned long Period;              // (1/clock) units
unsigned long First;               // PD5 first edge
unsigned long distInches;           //distance measured in inces
unsigned long distCm;
unsigned char Done;                // set each rising



void PingMeasurePD56_Init(void(*task)(unsigned long distance)){
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
                                   // activate port D
//  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOC+SYSCTL_RCGC2_GPIOD;
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD;

                                   // allow time to finish activating
  First = 0;                       // first will be wrong
  Done = 0;                        // set on subsequent
	userTask = task;
  GPIO_PORTD_DEN_R |= 0x60;        // enable digital I/O on PD5,6
	GPIO_PORTD_DIR_R |= 0x40;        //set PD6 to output
	GPIO_PORTD_DIR_R &= ~0x20;       //set PD5 to input
	PD6 = 0; 
 
	//set up edge interrupts for PD5
	GPIO_PORTD_IS_R &= ~020; //PD5 is edge sensitive
	GPIO_PORTD_IBE_R |= 0x20; //interrupt on both edges
	GPIO_PORTD_ICR_R |= 0x20; //clear interrupt flag
	GPIO_PORTD_IM_R |= 0x20; //enable interrupts
	
	//port D: interrupt 3
  NVIC_PRI0_R = (NVIC_PRI4_R&0x00FFFFFF)|0x00000000; // top 3 bits
  NVIC_EN0_R |= NVIC_EN0_INT3;    // enable interrupt 19 in NVIC	
  EnableInterrupts();
}

void GPIOPortD_Handler(){
	GPIO_PORTD_ICR_R |= 0x20;
	if(PD5 == 0x20){ //if rising edge
		First = OS_Time();
	}
	else{ //falling edge
		Period = OS_TimeDifference(OS_Time(), First);
		//distInches = Period/INCHDIV;
		distCm = Period/CMDIV;
		Done = 0xFF;
		(*userTask)(distCm);
	}
}



//trigger a ping measurement
//returns 0 on success. 
//returns 1 if sensor is not ready.
int PingTriggerPD56(void){int i; unsigned long iBit;
  if(!Done){return 1;}
	iBit = StartCritical();
	GPIO_PORTD_IM_R &= ~0x20; // disable edge triggered interrupts
	Done = 0;

	//send 10us pulse
	PD6 = 0x40;
	for(i = 0; i < 140; i ++){} //wait approx.10 us
	//enable interrupts
  PD6 = 0;
	EndCritical(iBit);
		
  GPIO_PORTD_IM_R |= 0x20;
	//wait for ping completion
		
//	TIMER0_IMR_R &= ~TIMER_IMR_CAEIM; // disable capture match interrupt
	//distance in mm = #ticks (period) / Cinv*2. divide by 2 because sound has to travel to object and back
	 return 0;
}

void testDummyThread(void){
	for(;;){}
	}
	
void testThread(void){ //test thread that waits for measurements
  while(1){
		while(!Done){	}
		Done = 0;
		distInches = Period/CMDIV;

	}
}
void testInterrupt(void){
	PingTriggerPD56();
}
	
// //ping tester
// int main(void){//unsigned long distance; int i; int delay;
// //  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL |
// //                  SYSCTL_XTAL_8MHZ | SYSCTL_OSC_MAIN);
// // 	delay = 0;
// // 	PingMeasure_Init();
// // 	for(;;){
// // 		distance = PingMeasure();
// // 		
// // 		
// //     
// // 		i = distance;
// // 		i = i/10;
// // 	}
// // 	
// 	OS_Init();
// 	PingMeasurePD56_Init(&tse);
// 	OS_AddThread(&testThread, 128, 0);
// 	OS_AddThread(&testDummyThread, 128, 5);
// 	OS_AddPeriodicThread(&testInterrupt, TIME100US, 1);
// 	
// 	OS_Launch(50*1000); //1 ms timeslice
// }

