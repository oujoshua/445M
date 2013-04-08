#include <stdio.h>
#include "hw_types.h"
#include "sysctl.h"
#include "rit128x96x4.h"
#include "OS.h"
#include "UART.h"
#include "shell.h"
#include "ADC.h"
#include "IR.h"
#include "lm3s8962.h"
#include "edisk.h"
#include "efile.h"
#include "PingMeasure.h"
#include "OS_Ethernet.h"
#include <string.h>

#define TIMESLICE TIME_2MS    // thread switch time in system time units
#define RUNLENGTH 10000   // display results and quit when NumSamples==RUNLENGTH

void PID(void);
void DAS(void);
void ButtonPush(void);
void Consumer(void);
void enetTest(void);
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
short PID_stm32(short Error, short *Coeff);

unsigned long NumCreated;   // number of foreground threads created
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
unsigned long PIDWork;      // current number of PID calculations finished
unsigned long NumSamples;   // incremented every sample
unsigned long FilterWork;   // number of digital filter calculations finished
long MaxJitter;             // largest time jitter between interrupts in usec
long MinJitter;             // smallest time jitter between interrupts in usec
#define JITTERSIZE 64
unsigned long const JitterSize=JITTERSIZE;
unsigned long JitterHistogram[JITTERSIZE]={0,};
long x[64],y[64];         // input and output arrays for FFT

extern unsigned char Ethernet_okToSend;

void SD_Init(void)
{
	eFile_Init();
	OS_Kill();
}

int main(void)
{
	/* Initialize 8MHz clock */
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_8MHZ | SYSCTL_OSC_MAIN);
//  OLED_Init(15);
//   ADC_Init(1000);
//   ADC_Open(0);
//   ADC_Open(1);
//   SH_Init();
  OS_Init();
  OS_MailBox_Init();
  
  //********initialize communication channels
  OS_MailBox_Init();
  OS_Fifo_Init(MAX_FIFO_SIZE/2);
  
  NumCreated = 0;
  NumSamples = 0;
  MaxJitter = 0;       // OS_Time in 20ns units
  MinJitter = 10000000;
  
  PingMeasurePD56_Init(&PingHandler);
  IR_Init();
  
  // testing/debugging stuff
  OS_Add_Periodic_Thread(&disk_timerproc,10,0);   // time out routines for disk
  OS_Add_Periodic_Thread(&PingTriggerPD56, 100, 0);
  OS_AddButtonTask(&ButtonPush, 1);
  OS_AddDownTask(&ButtonPush, 1);
  //NumCreated += OS_AddThread(&PID, 128, 1);
  NumCreated += OS_AddThread(&Consumer, 128, 0);
  OS_AddThread(&SH_Shell, 128, 6);
	OS_AddThread(&SD_Init, 128, 0);
//   NumCreated += OS_AddThread(&enetTest, 128, 2);
  OS_Launch(TIMESLICE);
	
	/* Loop indefinitely */
  while(1);
}

void enetTest(void) {
  unsigned char str[8];
//   while(!Ethernet_okToSend) {
//       OS_Sleep(50);
//   }
  while(1) {
    memset(str, 0, 8);
    sprintf(str, "%d", DataLost);
    printf("attempting to send %s\n", str);
    OS_EthernetMailBox_Send(str, 8);
    printf("sent %s\n", str);
    OS_Sleep(2000);
  }
}

//------------------Task 2--------------------------------
// background thread executes with select button
// one foreground task created with button push
// foreground treads run for 2 sec and die
// ***********ButtonWork*************
void ButtonWork(void){
unsigned long i;
unsigned long myId = OS_Id(); 
  char str[20];
  sprintf(str, "NumCreated = %d", NumCreated);
//  OLED_Out(BOTTOM, str); 
  if(NumSamples < RUNLENGTH){   // finite time run
    for(i=0;i<20;i++){  // runs for 2 seconds
      OS_Sleep(50);     // set this to sleep for 0.1 sec
    }
  }
  sprintf(str, "PIDWork    = %d", PIDWork);
//  OLED_Out(BOTTOM, str);
  sprintf(str, "DataLost   = %d", DataLost);
//  OLED_Out(BOTTOM, str);
  sprintf(str, "Jitter(us) = %d",MaxJitter-MinJitter);
//  OLED_Out(BOTTOM, str);
//  OLED_Out(BOTTOM, "");
  OS_Kill();  // done
  OS_Delay(OS_ARBITRARY_DELAY);
} 

//************ButtonPush*************
// Called when Select Button pushed
// Adds another foreground task
// background threads execute once and return
void ButtonPush(void){
  if(OS_AddThread(&ButtonWork,100,1)){
    NumCreated++; 
  }
  OS_Kill();
  OS_Delay(OS_ARBITRARY_DELAY);
}

//------------------Task 3--------------------------------
// hardware timer-triggered ADC sampling at 1 kHz
// Producer runs as part of ADC ISR
// Producer uses fifo to transmit 1000 samples/sec to Consumer
// every 64 samples, Consumer calculates FFT
// every 64 ms, consumer sends data to Display via mailbox
// Display thread updates oLED with measurement

//******** Producer *************** 
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 1 kHz, started by your ADC_Collect
// The timer triggers the ADC, creating the 1 kHz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 10-bit sample 
// sends data to the consumer, runs periodically at 1 kHz
// inputs:  none
// outputs: none
void Producer(unsigned short data){
  if(1 /* NumSamples < RUNLENGTH */){   // finite time run
//     NumSamples++;               // number of samples
    if(OS_Fifo_Put(data) == 0){ // send to consumer
      DataLost++;
    } 
    else {
      NumSamples++;
    }
  }
  else {
    // do a put so other threads don't block
    OS_Fifo_Put(0);
  }
}
void Display(void); 

//******** Consumer *************** 
// foreground thread, accepts data from producer
// calculates FFT, sends DC component to Display
// inputs:  none
// outputs: none
unsigned char netBuffer[46] = {0,}; //padded buffer for ethernet packet, since min. size is 46 bytes

// unsigned char strbuf[8] = {0};
void Consumer(void){
unsigned long data /*,DCcomponent*/; // 10-bit raw ADC sample, 0 to 1023
//unsigned long t;  // time in ms
unsigned long myId = OS_Id();
//   unsigned long avg, std_dev, max_dev;
//   int i;
  ADC_Collect(0, 32, &Producer); // start ADC sampling, channel 0, 1000 Hz
//   NumCreated += OS_AddThread(&Display,128,0); 
  while(1 /* NumSamples < RUNLENGTH */) {
// 		printf("A\n");
		data = OS_Fifo_Get();
//     printf("B\n");
//     sprintf(strbuf, "%d", data);
		((unsigned long*)netBuffer)[0] = data;
//     printf("consumer sending\n");
		OS_EthernetMailBox_Send(netBuffer, 46);
		/*
    for(t = 0; t < 64; t++){   // collect 64 ADC samples
      data = OS_Fifo_Get();    // get from producer
      x[t] = IR_Distance(data);             // real part is 0 to 1023, imaginary part is 0
    }
  
//   // calculate the average
//   avg = 0;
//   for(i = 0; i < 64; i++) {
//     avg = avg + x[i];
//   }
//   avg = avg / 64;
//   // calculate the standard and max deviaton
//   std_dev = 0; max_dev = 0;
//   for(i = 0; i < 64; i++) {
//     long tdiff = x[i] - avg;
//     std_dev += tdiff * tdiff;
//     if(ABS(tdiff) > max_dev) {
//       max_dev = ABS(tdiff);
//     }
//   }
//   
//   std_dev = sqrt(std_dev / 64);
//   printf("average = %d\nstandard_deviation = %d\n maximum deviaton = %d\n", avg, std_dev, max_dev);
//     printf("ADC: %d -> cm: %d\n", x[0], IRDistance(x[0]));
    
    cr4_fft_64_stm32(y,x,64);  // complex FFT of last 64 ADC values
    DCcomponent = y[0]&0xFFFF; // Real part at frequency 0, imaginary part should be zero
    OS_MailBox_Send(DCcomponent);
		*/
    
  }
  printf("Consumer dead\n");
//   OLED_Out(BOTTOM, "CONSUMER DONE");
  OS_Kill();  // done
  OS_Delay(OS_ARBITRARY_DELAY);
}

static unsigned long voltage;
void DisplayThread(void)
{
	char str[20];
	while(NumSamples < RUNLENGTH)
	{
		sprintf(str, "Time left is %d  ", (RUNLENGTH-NumSamples)/1000);
//		_OLED_Message(TOP, 1, str, 15);
		sprintf(str, "v(mV) = %d", voltage);
//		_OLED_Message(TOP, 2, str, 15);
//    OS_Suspend();
     OS_Sleep(500);
	}
	OS_Kill();
	OS_Delay(OS_ARBITRARY_DELAY);
}

//******** Display *************** 
// foreground thread, accepts data from consumer
// displays calculated results on the LCD
// inputs:  none                            
// outputs: none
void Display(void){ 
unsigned long data;
//  char str[20];
//    sprintf(str, "Run length is %d", RUNLENGTH/1000);
//  OLED_Out(TOP, str);   // top half used for Display
//    NumCreated += OS_AddThread(&DisplayThread, 128, 0);
  while(NumSamples < RUNLENGTH) {
//    sprintf(str, "Time left is %d", (RUNLENGTH-NumSamples)/1000);
//     OS_LogEvent(EVENT_OLED_START);
//    OLED_Out(TOP, str);   // top half used for Display
//     OS_LogEvent(EVENT_OLED_FINISH);
    data = OS_MailBox_Recv();
    voltage = 3000*data/1024;               // calibrate your device so voltage is in mV
//		sprintf(str, "v(mV) = %d", voltage);
//		OLED_Out(TOP, str);
// 		OS_Delay(OS_ARBITRARY_DELAY);
  }
// 	OLED_Out(BOTTOM, "DONE");
	OS_Kill();  // done
  OS_Delay(OS_ARBITRARY_DELAY);
}

