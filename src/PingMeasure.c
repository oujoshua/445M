// PingMeasure.c
// Runs on LM3S8962
// Use PortD edge trigger and OS_Time on
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
#include <stdio.h>



/*
#define GPIO_PORTD_AFSEL_R      (*((volatile unsigned long *)0x40007420))
#define GPIO_PORTD_DEN_R        (*((volatile unsigned long *)0x4000751C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOD      0x00000008  // port D Clock Gating Control
*/

#define PD6 (*((volatile unsigned long *)0x40007100))
#define PD5 (*((volatile unsigned long *)0x40007080))
#define PC5 (*((volatile unsigned long *)0x40006080))
#define PC7 (*((volatile unsigned long *)0x40006200))
#define PB0 (*((volatile unsigned long *)0x40005004))
#define PB1 (*((volatile unsigned long *)0x40005008))
#define INCHDIV 148 //constant for conversion to inches
#define CMDIV 58 //constant for conversion to centimeters

#define NUMSENSORS 3

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

void (*userTask)(unsigned long distance, int Id); //user task that's executed upon measurement completion

unsigned long Period[NUMSENSORS];            // (1/clock) units
unsigned long First[NUMSENSORS];               // PD5 first edge
unsigned long distInches[NUMSENSORS];          //distance measured in inches
unsigned long distCm[NUMSENSORS];              //distance measured in CM
unsigned char Done[NUMSENSORS];              // set each rising

unsigned long PingBuff[PING_BUFF] = {0,};
int PingBuff_Idx = 0;


#pragma O0
void PingMeasurePB01_Init(void(*task)(unsigned long distance, int id)){
                                   // activate port C
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;

                                   // allow time to finish activating
  First[PB01ID] = 0;                       // first will be wrong
  Done[PB01ID] = 0xFF;                        // set on subsequent
	userTask = task;
  GPIO_PORTB_DEN_R |= 0x03;        // enable digital I/O on PB0,1
	GPIO_PORTB_DIR_R |= 0x02;        //set PB1 to output
	GPIO_PORTB_DIR_R &= ~0x01;       //set PB0 to input
	PB1 = 0; 
 
	//set up edge interrupts for PC5
	GPIO_PORTB_IS_R &= ~0x01; //PC5 is edge sensitive
	GPIO_PORTB_IBE_R &= ~0x01; //interrupt not on both edges
	GPIO_PORTB_IEV_R |= 0x01; //interrupt on rising edge
	GPIO_PORTB_ICR_R |= 0x01; //clear interrupt flag
	GPIO_PORTB_IM_R &= ~0x01; //disable interrupts
	
	//port C: interrupt 2
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFFFF00FF)|0x00000000; // 2nd to bottom set of bits
  NVIC_EN0_R |= NVIC_EN0_INT1;    // enable interrupt 17 in NVIC	
//   EnableInterrupts();
}


void PingTriggerPB01(void)
{
	int i; unsigned long iBit;
  if(!Done[PB01ID]){return /*1*/;}

	GPIO_PORTB_ICR_R |= 0x01; //enable pc5 interrupts
  GPIO_PORTB_IEV_R |= 0x01; //set for rising edge interrupt
  GPIO_PORTB_IM_R |= 0x01;	
	iBit = StartCritical(); //send start signal in critical section
	Done[PB01ID] = 0;

	//send 10us pulse
	PB1 = 0x02;
	for(i = 0; i < 140; i ++){} //wait approx.10 us (140 iterations was found to be enough)
  PB1 = 0;

	EndCritical(iBit);
		
	//reenable edge triggered interrupts

		
	//return 0;
}

void GPIOPortB_Handler(){
		GPIO_PORTB_ICR_R |= 0x01;

	if((GPIO_PORTB_IEV_R&0x01)){ //if rising edge
		First[PB01ID] = OS_Time();
		GPIO_PORTB_IEV_R &= ~0x01;//next time interrupt on falling edge
	}
	else{ //falling edge
		Period[PB01ID] = OS_TimeDifference(First[PB01ID], OS_Time());
		//distInches = Period/INCHDIV;
		distCm[PB01ID] = Period[PB01ID]/CMDIV;
		Done[PB01ID] = 0xFF;
		(*userTask)(distCm[PB01ID], PB01ID);
	}

}


#pragma O0
void PingMeasurePC57_Init(void(*task)(unsigned long distance, int id)){
                                   // activate port C
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOC;

                                   // allow time to finish activating
  First[PC57ID] = 0;                       // first will be wrong
  Done[PC57ID] = 0xFF;                        // set on subsequent
	userTask = task;
  GPIO_PORTC_DEN_R |= 0xA0;        // enable digital I/O on PC5,7
	GPIO_PORTC_DIR_R |= 0x80;        //set PC7 to output
	GPIO_PORTC_DIR_R &= ~0x20;       //set PC5 to input
	PC7 = 0; 
 
	//set up edge interrupts for PC5
	GPIO_PORTC_IS_R &= ~020; //PC5 is edge sensitive
	GPIO_PORTC_IBE_R &= ~0x20; //interrupt not on both edges
	GPIO_PORTC_IEV_R |= 0x20; //interrupt on rising edge
	GPIO_PORTC_ICR_R |= 0x20; //clear interrupt flag
	GPIO_PORTC_IM_R &= ~0x20; //disable interrupts
	
	//port C: interrupt 2
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFF)|0x00000000; // 2nd to top set of bits
  NVIC_EN0_R |= NVIC_EN0_INT2;    // enable interrupt 18 in NVIC	
//   EnableInterrupts();
}

void GPIOPortC_Handler(){
		GPIO_PORTC_ICR_R |= 0x20;

	if((GPIO_PORTC_IEV_R&0x20)){ //if rising edge
		First[PC57ID] = OS_Time();
		GPIO_PORTC_IEV_R &= ~0x20;//next time interrupt on falling edge
	}
	else{ //falling edge
		Period[PC57ID] = OS_TimeDifference(First[PC57ID], OS_Time());
		//distInches = Period/INCHDIV;
		distCm[PC57ID] = Period[PC57ID]/CMDIV;
		Done[PC57ID] = 0xFF;
		(*userTask)(distCm[PC57ID], 1);

		
	}

}

//trigger a ping measurement
//returns 0 on success. 
//returns 1 if sensor is not ready.
#pragma O0
void PingTriggerPC57(void)
{
	int i; unsigned long iBit;
  if(!Done[PC57ID]){return /*1*/;}

	GPIO_PORTC_ICR_R |= 0x20; //enable pc5 interrupts
  GPIO_PORTC_IEV_R |= 0x20; //set for rising edge interrupt
  GPIO_PORTC_IM_R |= 0x20;	
	iBit = StartCritical(); //send start signal in critical section
	Done[PC57ID] = 0;

	//send 10us pulse
	PC7 = 0x80;
	for(i = 0; i < 140; i ++){} //wait approx.10 us (140 iterations was found to be enough)
  PC7 = 0;

	EndCritical(iBit);
		
	//reenable edge triggered interrupts

		
	//return 0;
}



//Setup the ping sensor on pins PD5, PD6
//PD5 (CCP2 on board) is input: goes to echo on ping
//PC6 (Fault on board) is output: goes to trig on ping
#pragma O0
void PingMeasurePD56_Init(void(*task)(unsigned long distance, int id)){

                                   // activate port D
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD;

                                   // allow time to finish activating
  First[PD56ID] = 0;                       // first will be wrong
  Done[PD56ID] = 0xFF;                        // set on subsequent
	userTask = task;
  GPIO_PORTD_DEN_R |= 0x60;        // enable digital I/O on PD5,6
	GPIO_PORTD_DIR_R |= 0x40;        //set PD6 to output
	GPIO_PORTD_DIR_R &= ~0x20;       //set PD5 to input
	PD6 = 0; 
 
	//set up edge interrupts for PD5
	GPIO_PORTD_IS_R &= ~020; //PD5 is edge sensitive
	GPIO_PORTD_IBE_R &= ~0x20; //interrupt rising edige
	GPIO_PORTD_IEV_R |= 0x20;
	GPIO_PORTD_ICR_R |= 0x20; //clear interrupt flag
	GPIO_PORTD_IM_R  |= 0x20; //disable interrupts
	
	//port D: interrupt 3
  NVIC_PRI0_R = (NVIC_PRI0_R&0x00FFFFFF)|0x00000000; // top 3 bits
  NVIC_EN0_R |= NVIC_EN0_INT3;    // enable interrupt 19 in NVIC	
//   EnableInterrupts();
}

void GPIOPortD_Handler(){
	GPIO_PORTD_ICR_R |= 0x20;
	if(GPIO_PORTD_IEV_R&0x20){ //if rising edge
		First[PD56ID] = OS_Time();
		GPIO_PORTD_IEV_R &= ~0x20;
	}
	else{ //falling edge
		Period[PD56ID] = OS_TimeDifference(First[PD56ID], OS_Time());
		//distInches = Period/INCHDIV;
		distCm[PD56ID] = Period[PD56ID]/CMDIV;
		Done[PD56ID] = 0xFF;
		(*userTask)(distCm[PD56ID], 0);
	}
}





//trigger a ping measurement
//returns 0 on success. 
//returns 1 if sensor is not ready.
#pragma O0
void PingTriggerPD56(void)
{
	int i; unsigned long iBit;
  if(!Done[PD56ID]){return /*1*/;}


	iBit = StartCritical(); //send start signal in critical section
	GPIO_PORTD_ICR_R |= 0x20; //enable pc5 interrupts
  GPIO_PORTD_IEV_R |= 0x20; //set for rising edge interrupt
  GPIO_PORTD_IM_R |= 0x20;		Done[PD56ID] = 0;

	//send 10us pulse
	PD6 = 0x40;
	for(i = 0; i < 140; i ++){} //wait approx.10 us (140 iterations was found to be enough)
  PD6 = 0;
	EndCritical(iBit);
		
		
	//return 0;
}


void PingTriggerAll(void){
	int i; unsigned long iBit;
  if(!Done[PD56ID]){return /*1*/;}
	iBit = StartCritical(); //send start signal in critical section
	GPIO_PORTD_ICR_R |= 0x20; //enable pd5 interrupts
  GPIO_PORTD_IEV_R |= 0x20; //set for rising edge interrupt
  GPIO_PORTD_IM_R |= 0x20;
	GPIO_PORTC_ICR_R |= 0x20; //enable pc5 interrupts
  GPIO_PORTC_IEV_R |= 0x20; //set for rising edge interrupt
  GPIO_PORTC_IM_R |= 0x20;	
	GPIO_PORTB_ICR_R |= 0x01; //enable pb0 interrupts
  GPIO_PORTB_IEV_R |= 0x01; //set for rising edge interrupt
  GPIO_PORTB_IM_R |= 0x01;	

	Done[PD56ID] = 0;
  Done[PC57ID] = 0;
  Done[PB01ID] = 0;
	//send 10us pulse
	PD6 = 0x40;
  PC7 = 0x80;
  PB1 = 0x02;
	for(i = 0; i < 140; i ++){} //wait approx.10 us (140 iterations was found to be enough)
  PD6 = 0;
	PC7 = 0;
	PB1 = 0;
	EndCritical(iBit);
		
	//reenable edge triggered interrupts		
	//return 0;
}


// record samples into buffer -- for use as argument to PingMeasurePD56_Init
void PingHandler(unsigned long distance, int id) {
  if(PingBuff_Idx < PING_BUFF) {
    PingBuff[PingBuff_Idx++] = distance;
  }
	printf("#: %d, Dist: %d\n", id, distance);
}

// dumps contents of ping buffer to UART
void Ping_Dump(void) {
  int i;
  printf("Ping Buffer Contents:\n");
  for(i = 0; i < PingBuff_Idx; i++) {
    printf("%d cm\n", PingBuff[i]);
  }
}

// reset the ping buffer
void Ping_Reset(void) {
  PingBuff_Idx = 0;
}


void testDummyThread(void){
	for(;;){}
	}
	
void testThread(void){ //test thread that waits for measurements
  while(1){
		while(!Done[PD56ID]){	}
		Done[PD56ID] = 0;
		distInches[PD56ID] = Period[PD56ID]/CMDIV;

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

