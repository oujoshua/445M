// ADC.h

void ADC_Init(unsigned int period);

void ADC_TimerInit(unsigned int period);

// should this take a channel number as the argument?
int ADC_Open(int channelNum); 

unsigned short ADC_In(unsigned int channelNum); 

int ADC_Collect(unsigned int channelNum, unsigned int period, void(*task)(unsigned short));

void ADC_ResetLog(void);

void ADC_Dump(void);

