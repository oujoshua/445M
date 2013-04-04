
#define IR_BUFFLEN 16

void IR_Init(void);

long IRDistance(long x);

// default producer
void IR_Producer(unsigned short data);

// default consumer
void IR_Consumer(void);

// dump buffer data to UART
void IR_Dump(void);

// reset IR buffer
void IR_Reset(void);
