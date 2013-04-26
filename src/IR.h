
#define IR_BUFFLEN 16
#define IR_FLEFT 1
#define IR_FRIGHT 0
#define IR_LEFT 2
#define IR_RIGHT 3
void IR_Init(void);

long IR_Distance(long x);

// default producer
void IR_Producer(unsigned short data);

// default consumer
void IR_Consumer(void);

// dump buffer data to UART
void IR_Dump(void);

// reset IR buffer
void IR_Reset(void);

// thread for monitoring all 4 ADC channels
void IR_MasterThread(void);
