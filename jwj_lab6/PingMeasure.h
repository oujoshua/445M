//PingMeasure.h


#define INCHDIV 148 //constant for conversion to inches: needs to be 148/(#microseconds per clock tick)
#define CMDIV   58 //constant for conversion to inches: needs to be 148/(#microseconds per clock tick)



//initialize the ping measurement sensore. uses PD4, PD6.
//task is a user method that gets called with the resulting measument. Distance in CM.
void PingMeasurePD46_Init(void(*task)(unsigned long distance));


//trigger a ping measurement
//returns 0 on success. 
//returns 1 if sensor is not ready.
int PingTriggerPD46(void);

