//PingMeasure.h


#define INCHDIV 148 //constant for conversion to inches: needs to be 148/(#microseconds per clock tick)
#define CMDIV   58 //constant for conversion to inches: needs to be 148/(#microseconds per clock tick)

#define PING_BUFF 16



//initialize the ping measurement sensore. uses PD5, PD6.
//task is a user method that gets called with the resulting measument. Distance in CM.
void PingMeasurePD56_Init(void(*task)(unsigned long distance));


//trigger a ping measurement
//returns 0 on success. 
//returns 1 if sensor is not ready.
int PingTriggerPD56(void);

// record samples into buffer -- for use as argument to PingMeasurePD56_Init
void PingHandler(unsigned long distance);

// dumps contents of ping buffer to UART
void Ping_Dump(void);

// reset the ping buffer
void Ping_Reset(void);

