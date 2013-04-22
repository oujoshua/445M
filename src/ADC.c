
#include "os.h"
#include "ADC.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "inc/lm3s8962.h"
#include "debug.h"
#include "rit128x96x4.h"
#include <stdio.h>
#include <string.h>

#ifndef TRUE
  #define TRUE 1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

#define ADC_NVIC_PRIORITY 3
#define PRESCALE 249 // constant prescale value, 5 us ticks
#define TIMER_RATE 5000000

static void _ADC_SetTimer1APeriod(unsigned int fs);
// static int _ADC_EnableNVICInterrupt(int channelNum);
// static int _ADC_DisableNVICInterrupt(int channelNum);
// static int _ADC_SetNIVCPriority(int channelNum, unsigned int priority);
// static void _ADC_ADC0_Init(void);
// static void _ADC_ADC1_Init(void);
// static void _ADC_ADC2_Init(void);
// static void _ADC_ADC3_Init(void);

long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts


static void(*_ADC_tasks[4])(unsigned short) = {NULL};
ADC_MailBox_Type _ADC_Mailbox;

void ADC_Init(unsigned int fs) {
  volatile unsigned long delay;
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC;       // activate ADC
  SYSCTL_RCGC0_R &= ~SYSCTL_RCGC0_ADCSPD_M; // clear ADC sample speed field
  SYSCTL_RCGC0_R += SYSCTL_RCGC0_ADCSPD500K;// configure for 500K ADC max sample rate
  delay = SYSCTL_RCGC0_R;                   // allow time to finish activating
  
  ADC_ACTSS_R &= ~ADC_ACTSS_ASEN1;          // disable sample sequencer 1
  ADC_IM_R &= ~ADC_IM_MASK1;                // disable SS1 interrupts
  
  ADC_EMUX_R &= ~ADC_EMUX_EM1_M;            // clear SS1 trigger select field
  ADC_EMUX_R += ADC_EMUX_EM1_TIMER;         // configure for timer trigger event
  ADC_SSMUX1_R = 0;                         // clear
  ADC_SSMUX1_R += (3 << ADC_SSMUX1_MUX3_S) +
                  (2 << ADC_SSMUX1_MUX2_S) +
                  (1 << ADC_SSMUX1_MUX1_S) +
                  (0 << ADC_SSMUX1_MUX0_S); // sample ADC0 in mux 0, ADC1 in mux 1, ADC2 in m2, ADC3 in m3
  ADC_SSCTL1_R = 0x00006000;                // 4th sample is end of sequence, 4th sample int enable
  
  ADC_IM_R |= ADC_IM_MASK1;                // enable SS1 interrupts
  ADC_ACTSS_R |= ADC_ACTSS_ASEN1;           // enable sample sequencer 1
  
  ADC_TimerInit(fs);
  
  NVIC_EN0_R |= NVIC_EN0_INT15;           // enable NVIC int 15 (SS 1)
  NVIC_PRI3_R = (NVIC_PRI3_R&0x0FFFFFFF)|(ADC_NVIC_PRIORITY << 29); // bits 29-31
  
  ADC_Mailbox_Init();
}

// fs in kHz
void ADC_TimerInit(unsigned int fs) {
  volatile unsigned long delay;
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1;    // activate timer1
  delay = SYSCTL_RCGC1_R;                   // allow time to finish activating
  TIMER1_CTL_R &= ~TIMER_CTL_TAEN;          // disable timer1A during setup
  TIMER1_CTL_R |= TIMER_CTL_TAOTE;          // enable timer1A trigger to ADC
  TIMER1_CFG_R = TIMER_CFG_16_BIT;          // configure for 16-bit timer mode
  TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;   // configure for periodic mode
  TIMER1_TAPR_R = PRESCALE;                 // prescale value for trigger
  _ADC_SetTimer1APeriod(fs);
  TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;        // disable timeout (rollover) interrupt
  TIMER1_CTL_R |= TIMER_CTL_TAEN;           // enable timer1A 16-b, periodic, no interrupts
}

// fs in Hz
// rate is (clock period)*(prescale + 1)(period + 1)
static void _ADC_SetTimer1APeriod(unsigned int period) {
  TIMER1_TAILR_R = period;                  // start value for trigger
}


// should this take a channel number as the argument?
int ADC_Open(int channelNum) {
  return 1;
}  

unsigned short ADC_In(unsigned int channelNum) {
  return 0;
}

int ADC_Collect(unsigned int channelNum, unsigned int fs, void(*task)(unsigned short)) {
//  int i;
  _ADC_SetTimer1APeriod(fs);
  _ADC_tasks[channelNum] = task;
//   for(i = 0; i < numberOfSamples; i++) {
//     buffer[i] = ADC_In(channelNum);
//   }
  return 1;
}

unsigned short ADC_Log[CHANNELS] = {0};
int ADC_LogIndex = 0;

void ADC1_Handler(void) {
  int i;
  unsigned short data[CHANNELS];
  ADC_ISC_R |= ADC_ISC_IN1;             // acknowledge ADC sequence 1 completion
  for(i = 0; i < CHANNELS; i++) {
    data[i] = ADC_SSFIFO1_R & ADC_SSFIFO1_DATA_M;
    ADC_Log[i] = data[i];
//     if(ADC_LogIndex < 16) {
//       ADC_Log[i][ADC_LogIndex] = data;
//     }
    if(_ADC_tasks[i] != NULL) {
      _ADC_tasks[i](data[i]);
    }
    ADC_LogIndex = (ADC_LogIndex < 16) ?  ADC_LogIndex + 1 : ADC_LogIndex;
  }
  ADC_Mailbox_Send(data);
}

void ADC_Mailbox_Init(void) {
  OS_InitSemaphore(&_ADC_Mailbox.hasData, 0); // initialize to empty
}

// returns 0 if successful, 1 if a previous set of samples was overwritten
int ADC_Mailbox_Send(unsigned short samples[CHANNELS]) {
  int existingSample = _ADC_Mailbox.hasData.value > 0;
  memcpy(_ADC_Mailbox.samples, samples, sizeof(unsigned short) * CHANNELS);
  OS_Signal(&_ADC_Mailbox.hasData);
  return existingSample;
}

void ADC_Mailbox_Receive(unsigned short samples[CHANNELS]) {
  OS_Wait(&_ADC_Mailbox.hasData);
  memcpy(samples, _ADC_Mailbox.samples, sizeof(unsigned short) * CHANNELS);
}


void ADC_ResetLog(void) {
  ADC_LogIndex = 0;
}

void ADC_Dump(void) {
  printf("ADC Channels 0-3: %d, %d, %d, %d\n", ADC_Log[0], ADC_Log[1], ADC_Log[2], ADC_Log[3]);
//   int i, j;
//   for(i = 0; i < 4; i++) {
//     printf("ADC%d:\n", i);
//     for(j = 0; j < ADC_LogIndex; j++) {
//       printf("%d\n", ADC_Log[i][j]);
//     }
//   }
}
