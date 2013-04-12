// FlexibleTimer0APWM.c
// Runs on LM3S811
// Use Timer0A in PWM mode to generate a square wave of a given
// high period and low period.
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

#define TIMER0_CFG_R            (*((volatile unsigned long *)0x40030000))
#define TIMER0_TAMR_R           (*((volatile unsigned long *)0x40030004))
#define TIMER0_CTL_R            (*((volatile unsigned long *)0x4003000C))
#define TIMER0_TAILR_R          (*((volatile unsigned long *)0x40030028))
#define TIMER0_TAMATCHR_R       (*((volatile unsigned long *)0x40030030))
#define TIMER0_TAMATCHR_R       (*((volatile unsigned long *)0x40030030))
#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_TAMR_TAAMS        0x00000008  // GPTM TimerA Alternate Mode
                                            // Select
#define TIMER_TAMR_TAMR_PERIOD  0x00000002  // Periodic Timer mode
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low
#define TIMER_TBILR_TBILRL_M    0x0000FFFF  // GPTM TimerB Interval Load
                                            // Register
#define GPIO_PORTD_AFSEL_R      (*((volatile unsigned long *)0x40007420))
#define GPIO_PORTD_DEN_R        (*((volatile unsigned long *)0x4000751C))
#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC1_TIMER0     0x00010000  // timer 0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOD      0x00000008  // port D Clock Gating Control

void WaitForInterrupt(void);  // low power mode

// high is number of cycles output high (1<=high<=(65535-low))
// low is number of cycles output low (0<=low<=(65535-high))
// duty cycle = high/(high+low) (~1 cycle<=duty<=~(high+low-1) cycles)
// frequency = (clock)/(high+low) (~92 Hz<=frequency<=~3 MHz)
void PWOut_Init2(unsigned short high, unsigned short low){
  volatile unsigned long delay;
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD; // activate port D
  delay = SYSCTL_RCGC2_R;          // allow time to finish activating
  GPIO_PORTD_DEN_R |= 0x10;        // enable digital I/O on PD4
  GPIO_PORTD_AFSEL_R |= 0x10;      // enable alt funct on PD4
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
                                   // configure for alternate (PWM) mode
  TIMER0_TAMR_R = (TIMER_TAMR_TAAMS|TIMER_TAMR_TAMR_PERIOD);
  TIMER0_TAILR_R = high+low-1;     // timer start value
  TIMER0_TAMATCHR_R = low-1;       // duty cycle = high/(high+low) = X
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, PWM, X% duty
}

//debug code
int main(void){
  PWOut_Init2(8000, 2000);         // initialize timer0A in PWM mode (600 Hz, 80% duty) (Figure 6.17 top)
//  PWOut_Init2(5000, 5000);         // initialize timer0A in PWM mode (600 Hz, 50% duty) (Figure 6.17 middle)
//  PWOut_Init2(2000, 8000);         // initialize timer0A in PWM mode (600 Hz, 20% duty) (Figure 6.17 bottom)
//  PWOut_Init2(24000, 6000);        // initialize timer0A in PWM mode (200 Hz, 80% duty) (Checkpoint 6.13)
//  PWOut_Init2(15000, 15000);       // initialize timer0A in PWM mode (200 Hz, 50% duty) (Checkpoint 6.13)
//  PWOut_Init2(6000, 24000);        // initialize timer0A in PWM mode (200 Hz, 20% duty) (Checkpoint 6.13)
//  PWOut_Init2(1, 9999);            // initialize timer0A in PWM mode (600 Hz, ~0% duty) (min duty cycle)
//  PWOut_Init2(9999, 1);            // initialize timer0A in PWM mode (600 Hz, ~100% duty)(max duty cycle)
//  PWOut_Init2(6, 4);               // initialize timer0A in PWM mode (600 kHz, 60% duty)
//  PWOut_Init2(32768, 32768);       // initialize timer0A in PWM mode (~92 Hz, 50% duty) (min frequency)
//  PWOut_Init2(1, 1);               // initialize timer0A in PWM mode (~3 MHz, 50% duty) (max frequency)
  while(1){
    WaitForInterrupt();
  }
}
