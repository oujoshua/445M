// PI.c
// Runs on LM3S811
// Use a setup similar to PeriodMeasure.c to measure the
// tachometer period.  Implement a PI controller to keep this
// period near a desired value.
// Daniel Valvano
// July 5, 2011

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

// DC motor with TIP120 interface connected to PD0 (PWM0)
// Tachometer connected to PD4 (CCP0) with 10 K pull-up
// See <paper schematic>
//
// This program drives PD0 with a particular duty cycle and measures
// the period of the tachometer signal.  This measurement is
// converted to an approximate shaft speed value and compared to the
// desired SETPOINT.  The error is the difference between the
// desired shaft speed and the measured shaft speed.  The
// proportional term is a constant times the error.  The integral
// term is the previous integral term plus a constant times the
// error.  The actuator output is the proportional term plus the
// integral term.  Both the integral term and the output term must
// be clamped within the bounds of valid PWM duty cycles (2 to
// PWMPERIOD - 1).  This is a PI controller.  See Incremental.c
// for an incremental controller.  See Sheet2 of chapter13-5.xls for
// example measurements collected using PICalibrate.c which were
// used to find the constant terms for the PI controller.
//
// NOTE: This program utilizes two different modules of the LM3S811
// which have different clocks.  Variables related to each module
// may have different units.  The Timer0 module is used to measure
// the tachometer period and periodically generate controller
// interrupts.  It runs at a frequency of 6 MHz (the system clock
// frequency).  The PWM0 module is used to generate a variable duty
// cycle square wave to drive the motor and runs at a frequency of 3
// MHz (the system clock frequency divided by two).

#include "PWM.h"

#define NVIC_EN0_INT20          0x00100000  // Interrupt 20 enable
#define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))  // IRQ 0 to 31 Set Enable Register
#define NVIC_PRI4_R             (*((volatile unsigned long *)0xE000E410))  // IRQ 16 to 19 Priority Register
#define NVIC_PRI5_R             (*((volatile unsigned long *)0xE000E414))  // IRQ 20 to 23 Priority Register
#define PWM_0_CMPA_R            (*((volatile unsigned long *)0x40028058))
#define TIMER0_CFG_R            (*((volatile unsigned long *)0x40030000))
#define TIMER0_TAMR_R           (*((volatile unsigned long *)0x40030004))
#define TIMER0_TBMR_R           (*((volatile unsigned long *)0x40030008))
#define TIMER0_CTL_R            (*((volatile unsigned long *)0x4003000C))
#define TIMER0_IMR_R            (*((volatile unsigned long *)0x40030018))
#define TIMER0_ICR_R            (*((volatile unsigned long *)0x40030024))
#define TIMER0_TAILR_R          (*((volatile unsigned long *)0x40030028))
#define TIMER0_TBILR_R          (*((volatile unsigned long *)0x4003002C))
#define TIMER0_TAR_R            (*((volatile unsigned long *)0x40030048))
#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_TAMR_TACMR        0x00000004  // GPTM TimerA Capture Mode
#define TIMER_TAMR_TAMR_CAP     0x00000003  // Capture mode
#define TIMER_TBMR_TBMR_PERIOD  0x00000002  // Periodic Timer mode
#define TIMER_CTL_TBEN          0x00000100  // GPTM Timer B Enable
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_CTL_TAEVENT_POS   0x00000000  // Positive edge
#define TIMER_IMR_TBTOIM        0x00000100  // GPTM Timer B Time-Out Interrupt
                                            // Mask
#define TIMER_IMR_CAEIM         0x00000004  // GPTM CaptureA Event Interrupt
                                            // Mask
#define TIMER_ICR_TBTOCINT      0x00000100  // GPTM Timer B Time-Out Interrupt
                                            // Clear
#define TIMER_ICR_CAECINT       0x00000004  // GPTM CaptureA Event Interrupt
                                            // Clear
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low
#define TIMER_TBILR_TBILRL_M    0x0000FFFF  // GPTM Timer B Interval Load
                                            // Register
#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
#define GPIO_PORTD_AFSEL_R      (*((volatile unsigned long *)0x40007420))
#define GPIO_PORTD_DEN_R        (*((volatile unsigned long *)0x4000751C))
#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC1_TIMER1     0x00020000  // Timer 1 Clock Gating Control
#define SYSCTL_RCGC1_TIMER0     0x00010000  // timer 0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOD      0x00000008  // port D Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control
//#define SETPOINT         836  // (0.001 rps) units (controller setpoint for ~25% PWM)
#define SETPOINT         900  // (0.001 rps) units
//#define SETPOINT         952  // (0.001 rps) units (controller setpoint for ~75% PWM)
#define PWMPERIOD       3000  // (1/(2*clockfreq)) units (for 1,000 Hz PWM)
//#define KP               199  // (1000 cycles/rps) units
#define KP                20  // (1000 cycles/rps) units
//#define KI               150  // (cycles/rps/(0.01sec)) units
#define KI                15  // (cycles/rps/(0.01sec)) units

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
// initialize hardware PWM0 (see PWM.c)
void PWM0_Init(unsigned short period, unsigned short duty);
// set hardware PWM0 duty cycle (see PWMSine.c)
// newDuty is number of PWM clock cycles before output changes (2<=newDuty<=period-1)
// If PWM0_Init() was called, the output goes from 0 to 1 when the
// PWM counter reaches 'newDuty'.  Regardless, PWM0 must have
// already been configured before calling this function.
void PWM0_SetDuty(unsigned short newDuty){
  PWM_0_CMPA_R = newDuty - 1;          // count value when output may change
}
// initialize timer0A in 16-bit capture mode with interrupts enabled
// This function is based on the one in PeriodMeasure.c with minor changes.
unsigned long Period;              // (1/clockfreq) units
unsigned long First;               // Timer0A first edge
unsigned char Done;                // set each rising
void PeriodMeasure_Init(void){
  DisableInterrupts();
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD;// activate port D
                                   // allow time to finish activating
  First = 0;                       // first will be wrong
  Done = 0;                        // set on subsequent
  GPIO_PORTD_DEN_R |= 0x10;        // enable digital I/O on PD4
  GPIO_PORTD_AFSEL_R |= 0x10;      // enable alt funct on PD4
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
                                   // configure for capture mode
  TIMER0_TAMR_R = (TIMER_TAMR_TACMR|TIMER_TAMR_TAMR_CAP);
                                   // configure for rising edge event
  TIMER0_CTL_R &= ~(TIMER_CTL_TAEVENT_POS|0xC);
  TIMER0_TAILR_R = TIMER_TAILR_TAILRL_M;// start value
  TIMER0_IMR_R |= TIMER_IMR_CAEIM; // enable capture match interrupt
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// clear timer0A capture match flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, +edge timing, interrupts
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R |= NVIC_EN0_INT19;    // enable interrupt 19 in NVIC
  EnableInterrupts();
}
// initialize timer0B in 16-bit periodic mode with interrupts enabled
void Timer0BInterrupt_Init(void){
  volatile unsigned long delay;
  DisableInterrupts();
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
  delay = SYSCTL_RCGC1_R;          // allow time to finish activating
  TIMER0_CTL_R &= ~TIMER_CTL_TBEN; // disable timer0B during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
                                   // configure for periodic mode
  TIMER0_TBMR_R = TIMER_TBMR_TBMR_PERIOD;
  TIMER0_TBILR_R = 60000;          // start value for 100 Hz interrupts
  TIMER0_IMR_R |= TIMER_IMR_TBTOIM;// enable timeout (rollover) interrupt
  TIMER0_ICR_R = TIMER_ICR_TBTOCINT;// clear timer0B timeout flag
  TIMER0_CTL_R |= TIMER_CTL_TBEN;  // enable timer0B 16-b, periodic, interrupts
                                   // Timer0B=priority 2
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFFFF00)|0x00000040; // bits 5-7
  NVIC_EN0_R |= NVIC_EN0_INT20;    // enable interrupt 20 in NVIC
  EnableInterrupts();
}
// timer0A interrupt handler occurs on rising edges of PD4 (CCP0)
void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// acknowledge timer0A capture match
  Period = (First - TIMER0_TAR_R)&0xFFFF;// (1/clockfreq) resolution
  First = TIMER0_TAR_R;            // setup for next
  Done = 0xFF;
}
// timer0B interrupt handler occurs every 0.01 sec (100 Hz)
signed short Error = 0;            // (0.001 rps) units
signed short Proportional = 0;     // (1/(2*clockfreq)) units
signed short Integral = 0;         // (1/(2*clockfreq)) units
void Timer0B_Handler(void){
  unsigned long currentSpeed;      // "Period" value in (0.001 rps) units
  signed short newDuty;            // (1/(2*clockfreq)) units
  TIMER0_ICR_R = TIMER_ICR_TBTOCINT;// acknowledge timer0B timeout
  // when the motor is spinning, at least one tachometer rising edge
  // should occur between each controller interrupt
  if(Done == 0){
    currentSpeed = 0;
  }
  // convert 'Period' from (1/clockfreq) units to (0.001 rps) units
  // (1 rev/141 tachPulse)*(1 tachPulse/Period clockCycles)*(6,000,000 clockCycles/1 sec)*1000 =
  // 6,000,000,000/141/Period =
  // 2,000,000,000/47/Period (reduced to fit in 32-bit range)
  else{
    currentSpeed = 2000000000/Period/47;
  }
  Done = 0;
  // e = Desired speed - Measured speed
  Error = SETPOINT - currentSpeed;
  // P = Kp*e
  Proportional = KP*Error;
  // I = I + 150*e (clamp to valid range)
  Integral = Integral + KI*Error;
  // clamp to range 2 to PWMPERIOD - 1
  if(Integral < 2){
    Integral = 2;
  }
  if(Integral > (PWMPERIOD - 1)){
    Integral = PWMPERIOD - 1;
  }
  // U = P + I (clamp to valid range)
  newDuty = Proportional + Integral;
  if(newDuty < 2){
    newDuty = 2;
  }
  if(newDuty > (PWMPERIOD - 1)){
    newDuty = PWMPERIOD - 1;
  }
  // make the change
  PWM0_SetDuty(newDuty);
}
int main(void){
                                       // initialize PWM0
                                       // 6,000,000/2/PWMPERIOD Hz
  PWM0_Init(PWMPERIOD, PWMPERIOD/2);   // 50% duty
  PeriodMeasure_Init();                // initialize timer0A in capture mode
  //Timer0BInterrupt_Init();             // initialize timer0B in periodic mode
  while(1){
    WaitForInterrupt();
  }
}
