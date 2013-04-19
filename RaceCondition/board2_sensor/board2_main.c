#include "OS.h"
#include "shell.h"
#include "PingMeasure.h"



void dummyThread(void){
	while(1){}
	
}


int main(void) {
  OS_Init();
  SH_Init();
	PingMeasurePD56_Init(&PingHandler);
	PingMeasurePC57_Init(&PingHandler);
	PingMeasurePB01_Init(&PingHandler);
	OS_Add_Periodic_Thread(&PingTriggerAll, 200, 1);
	OS_AddThread(&SH_Shell, 128, 6);
	OS_AddThread(&dummyThread, 128, 0);
  OS_Launch(TIME_2MS);
  return 0;
}
