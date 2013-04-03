// InputCapture.c
// Runs on LM3S1968
// Use Timer0A in edge time mode to request interrupts on the rising
// edge of PD4 (CCP0), and count the pulses.
// Daniel Valvano
// June 27, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011
   Example 7.1, Program 7.1

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
 
 #include "InputCapture.h"
 #include "hw_types.h"
 #include "lm3s8962.h"
 #include <stdio.h>
 
 /*********************************
  *********************************
  ** MAY NEED TO CHANGE TIMERS ****
  *********************************
  *********************************/

// external signal connected to PD4 (CCP0) (trigger on rising edge)

// #define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
// #define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))  // IRQ 0 to 31 Set Enable Register
// #define NVIC_PRI4_R             (*((volatile unsigned long *)0xE000E410))  // IRQ 16 to 19 Priority Register
// #define TIMER0_CFG_R            (*((volatile unsigned long *)0x40030000))
// #define TIMER0_TAMR_R           (*((volatile unsigned long *)0x40030004))
// #define TIMER0_CTL_R            (*((volatile unsigned long *)0x4003000C))
// #define TIMER0_IMR_R            (*((volatile unsigned long *)0x40030018))
// #define TIMER0_ICR_R            (*((volatile unsigned long *)0x40030024))
// #define TIMER0_TAILR_R          (*((volatile unsigned long *)0x40030028))
// #define TIMER0_TAR_R            (*((volatile unsigned long *)0x40030048))
// #define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
//                                             // function is controlled by bits
//                                             // 1:0 of GPTMTAMR and GPTMTBMR
// #define TIMER_TAMR_TACMR        0x00000004  // GPTM TimerA Capture Mode
// #define TIMER_TAMR_TAMR_CAP     0x00000003  // Capture mode
// #define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
// #define TIMER_CTL_TAEVENT_POS   0x00000000  // Positive edge
// #define TIMER_IMR_CAEIM         0x00000004  // GPTM CaptureA Event Interrupt
//                                             // Mask
// #define TIMER_ICR_CAECINT       0x00000004  // GPTM CaptureA Event Interrupt
//                                             // Clear
// #define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
//                                             // Register Low
// #define GPIO_PORTC_DATA_R       (*((volatile unsigned long *)0x400063FC))
// #define GPIO_PORTC_DIR_R        (*((volatile unsigned long *)0x40006400))
// #define GPIO_PORTC_DEN_R        (*((volatile unsigned long *)0x4000651C))
// #define GPIO_PORTD_AFSEL_R      (*((volatile unsigned long *)0x40007420))
// #define GPIO_PORTD_DEN_R        (*((volatile unsigned long *)0x4000751C))
// #define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
// #define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
// #define SYSCTL_RCGC1_TIMER0     0x00010000  // timer 0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOD      0x00000008  // port D Clock Gating Control
#define SYSCTL_RCGC2_GPIOC      0x00000004  // port C Clock Gating Control

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

volatile unsigned long Count;      // incremented on interrupt
#pragma O0
void TimerCapture_Init(void){
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
                                   // activate port C and port D
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOC+SYSCTL_RCGC2_GPIOD;
  Count = 0;                       // allow time to finish activating
  GPIO_PORTC_DIR_R |= 0x20;        // make PC5 out (PC5 built-in LED)
  GPIO_PORTC_DEN_R |= 0x20;        // enable digital I/O on PC5
  GPIO_PORTD_DEN_R |= 0x10;        // enable digital I/O on PD4
  GPIO_PORTD_AFSEL_R |= 0x10;      // enable alt funct on PD4
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
                                   // configure for capture mode
  TIMER0_TAMR_R = (TIMER_TAMR_TACMR|TIMER_TAMR_TAMR_CAP);
                                   // configure for rising edge event
  TIMER0_CTL_R &= ~(TIMER_CTL_TAEVENT_POS|0xC);
  TIMER0_TAILR_R = 0x0000FFFF;     // start value
  TIMER0_IMR_R |= TIMER_IMR_CAEIM; // enable capture match interrupt
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// clear timer0A capture match flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, +edge timing, interrupts
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R |= NVIC_EN0_INT19;    // enable interrupt 19 in NVIC
}
void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// acknowledge timer0A capture match
  GPIO_PORTC_DATA_R = GPIO_PORTC_DATA_R^0x20; // toggle PC5
  Count = Count + 1;
  printf("Count = %d\n", Count);
}

//debug code
int input_cature_main(void){
  TimerCapture_Init(); // initialize timer0A in capture mode
  while(1){
    WaitForInterrupt();
  }
}
