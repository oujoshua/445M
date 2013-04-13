
void TimerCapture_Init(void);

// calculate the average, standard deviation, and maximum deviation
// on a buffer of timestamps from the input capture module and print
// the results to the UART
void IC_Calc(void);

// reset the input capture event log
void IC_Reset(void);
