#include "OS.h"
#include "shell.h"
#include "PingMeasure.h"
#include "src/ADC.h"
#include "IR.h"

#define ADC_PERIOD 10000 // configure for 20 Hz fs, ADC prescale is 5us
#define IR_PRIORITY 1
#define DECISION_PRIORITY 2


void dummyThread(void){
	while(1){}
	
}

void DecisionMaker(void);


int main(void) {
  OS_Init();
  SH_Init();
  ADC_Init(ADC_PERIOD);
	PingMeasurePD56_Init(&PingHandler);
	PingMeasurePC57_Init(&PingHandler);
	PingMeasurePB01_Init(&PingHandler);
	OS_Add_Periodic_Thread(&PingTriggerAll, 200, 1);
  OS_AddThread(&IR_MasterThread, 128, IR_PRIORITY);
  OS_AddThread(&DecisionMaker, 128, DECISION_PRIORITY);
	OS_AddThread(&SH_Shell, 128, 6);
  OS_Launch(TIME_2MS);
  
  while(1) {
    ;
  }
  
}

// make a decision and send it to the driver board based on the sensor data
void DecisionMaker(void) {
  
  while(1) {
    
  }
  
}
