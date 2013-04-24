//PingMeasure.h


#define INCHDIV 148 //constant for conversion to inches: needs to be 148/(#microseconds per clock tick)
#define CMDIV   58 //constant for conversion to inches: needs to be 148/(#microseconds per clock tick)

#define PING_BUFF 16

#define PD56ID 0
#define FWDPING 0
#define PC57ID 1
#define PB01ID 2

//initialize the ping measurement sensore. uses PD5, PD6.
//task is a user method that gets called with the resulting measument. Distance in CM.
void PingMeasurePD56_Init(void(*task)(unsigned long distance, int id));
void PingMeasurePC57_Init(void(*task)(unsigned long distance, int id));
void PingMeasurePB01_Init(void(*task)(unsigned long distance, int id));

//trigger a ping measurement
//returns 0 on success. 
//returns 1 if sensor is not ready.
//User function will get executed when the measurement is done
void PingTriggerPD56(void);
void PingTriggerPC57(void);
void PingTriggerPB01(void);
void PingTriggerAll(void);

// record samples into buffer -- for use as argument to PingMeasurePD56_Init
void PingHandler(unsigned long distance, int id);

// dumps contents of ping buffer to UART
void Ping_Dump(void);

// reset the ping buffer
void Ping_Reset(void);

