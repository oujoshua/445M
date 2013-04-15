//**************tachometerTimer1.h****************
//Uses edge triggered GPIO interrupts and 32-bit timer1 to measure the period of 2 tachometers
//Uses ports PC5, PC7 as tachometer input ports
//Edge triggered interrupts are priority 0

//values to read for tachometer readings
//Done variables get set when new values are calculated
//Period variables contain most recent time between rising edges of tachometer inputs
extern volatile unsigned long period7, period5; //periods in units of 5 us: time between last 2 rising edges
extern volatile unsigned char Done7, Done5;    //set on each rising edge of PC5,7

//tacho_Init
//initializes inputs and timers to measure periodic tachometer readings
void tacho_Init(void);


