#include "OS.h"
#include "PingMeasure.h"
#include "src/ADC.h"
#include "IR.h"
#include "OS_Ethernet.h"
#include "commands.h"
#include "eFile.h"
#include "eDisk.h"
#include "rit128x96x4.h"
#define ADC_PERIOD 10000 // configure for 20 Hz fs, ADC prescale is 5us
#define IR_PRIORITY 1
#define DECISION_PRIORITY 2
#define NET_PRIORITY 2
unsigned short IR_Samples[3][4] = {0,};
long IR_Dist[4] = {200, };


void dummyThread(void){
	while(1){}
	
}

void DecisionMaker(void);


long Median_Of_3(long a, long b, long c){
  long middle;
  if ((a <= b) && (a <= c)){
    middle = (b <= c) ? b : c;
  }
  else if ((b <= a) && (b <= c)){
    middle = (a <= c) ? a : c;
   }
  else{
    middle = (a <= b) ? a : b;
  }
  return middle;
}

// ADC0 is the (angle) right IR
// ADC1 is the (angle) left IR
void IR_Listener(void) {
  char count = 0;
	long val;
  while(1) {
     ADC_Mailbox_Receive(&IR_Samples[count][0]);
//    if(count == 2){
		val = IR_Distance(Median_Of_3(IR_Samples[0][IR_LEFT], IR_Samples[1][IR_LEFT], IR_Samples[2][IR_LEFT]));
     OS_EthernetSendVal(&val);
 //    }
    count = (count + 1) % 3;
        //OS_bSignal(&IR_Ready);
  }
}


int main(void) {
  OS_Init();
 	OS_EthernetInit();
	  ADC_Init(833);

	OLED_Init(15);
  _OLED_Message(BOTTOM, 4, "this is stupid", 8);
	OS_AddThread(&IR_Listener, 128, 0);
	//eFile_Init();
  //OS_Add_Periodic_Thread(&disk_timerproc, 10, 4);
 	//OS_AddThread(&OS_EthernetListener, 128, NET_PRIORITY);
  OS_Launch(TIME_2MS);
  
  while(1) {
    ;
  }
  
}

unsigned long FWDDist;
OS_SemaphoreType FWDReady;
// make a decision and send it to the driver board based on the sensor data
void DecisionMaker(void) {
	Command cmd;
	
  OS_InitSemaphore(&FWDReady, 1);//initialize semaphore; starts at 1
	OS_bWait(&FWDReady); //clear semaphore; will be waiting for a signal now
	
  while(1) {
    OS_bWait(&FWDReady);
		if(FWDDist < 25){
			cmd = MEDLEFT;
		}
		else{
		  cmd = FWD;
    }
		OS_EthernetMailBox_Send((unsigned char*)(&cmd), sizeof(cmd));

  }
  
}


void PingGetFWD(unsigned long distance, int id){
	//get ping data, pass to foreground thread
	if(id == FWDPING){
		FWDDist = distance;
		OS_bSignal(&FWDReady);
	}
}
