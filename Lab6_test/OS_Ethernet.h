//OS_Ethernet.h

void OS_EthernetInit(void);


void OS_EthernetListener(void);




void OS_EthernetSender(void);


void EthernetTest(void);

// ******** OS_EthernetMailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_EthernetMailBox_Init(void);

// ******** OS_EthernetMailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_EthernetMailBox_Send(unsigned char* buffer, unsigned long size);

// ******** OS_EthernetMailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
void OS_EthernetMailBox_Recv(void);
