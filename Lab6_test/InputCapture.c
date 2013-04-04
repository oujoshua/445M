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
 #include "os.h"
 #include "hw_types.h"
 #include "lm3s8962.h"
 #include <stdio.h>
 
 #define IC_BUFFLEN 16
 
 // wheel radius = 3.8 cm
 
 /*********************************
  *********************************
  ** MAY NEED TO CHANGE TIMERS ****
  *********************************
  *********************************/

// external signal connected to PD4 (CCP0) (trigger on rising edge)

// #define SYSCTL_RCGC1_TIMER0     0x00010000  // timer 0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOD      0x00000008  // port D Clock Gating Control
#define SYSCTL_RCGC2_GPIOC      0x00000004  // port C Clock Gating Control

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

unsigned long IC_buffer[IC_BUFFLEN] = {0, };
int IC_idx = 0;

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

unsigned long first_capture, last_capture;

void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// acknowledge timer0A capture match
  GPIO_PORTC_DATA_R = GPIO_PORTC_DATA_R^0x20; // toggle PC5
  if(Count == 0) {
    first_capture = OS_MsTime();
  }
  Count = Count + 1;
  if(IC_idx < IC_BUFFLEN) {
    IC_buffer[IC_idx++] = OS_MsTime();
  }
  if(OS_MsTime() - first_capture > 10000) {
//     last_capture = OS_MsTime();
//     IC_Calc();
//     printf("%d events -> %d rotations in %d ms\n", Count, Count/2, OS_MsTime() - first_capture);
    Count = 0;
  }
}

// calculate the average, standard deviation, and maximum deviation
// on a buffer of timestamps from the input capture module and print
// the results to the UART
unsigned long time_diffs[IC_BUFFLEN - 1] = {0, };
void IC_Calc(void) {
  int i, num = 0;
  unsigned long avg, std_dev, max_dev;
  // calculate the time difference between each event
  for(i = 0; i < IC_idx - 1; i++) {
    num++;
    printf("IC_buffer[%d] = %d\n", i, IC_buffer[i]);
    time_diffs[i] = IC_buffer[i + 1] - IC_buffer[i];
  }
  // calculate the average
  avg = 0;
  for(i = 0; i < num; i++) {
    printf("time_diffs[%d] = %d\n", i, time_diffs[i]);
    avg = avg + time_diffs[i];
  }
  avg = avg / num;
  // calculate the standard and max deviaton
  std_dev = 0; max_dev = 0;
  for(i = 0; i < num; i++) {
    long tdiff = time_diffs[i] - avg;
    std_dev += tdiff * tdiff;
    if(ABS(tdiff) > max_dev) {
      max_dev = ABS(tdiff);
    }
  }
  std_dev = sqrt(std_dev / IC_idx);
  printf("average = %d\nstandard_deviation = %d\n maximum deviaton = %d\n", avg, std_dev, max_dev);
}

// reset the input capture event log
void IC_Reset(void) {
  IC_idx = 0;
}

//debug code
int input_cature_main(void){
  TimerCapture_Init(); // initialize timer0A in capture mode
  while(1){
    WaitForInterrupt();
  }
}
