#include "os.h"
#include "mac.h"
#include "lm3s8962.h"
#include "eFile.h"
#include "commands.h"
#include "OS_Ethernet.h"
#include "rit128x96x4.h"
#include "ir.h"
#include <stdio.h>
#include <string.h>
#define LOG_FILE "enet.txt"
#define BOARD 1
// void OS_EthernetListener(void);
// void OS_EthernetSender(void);
// void EthernetTest(void);
Command MoveCmd;
OS_SemaphoreType CmdReady;
unsigned char RcvMessage[MAXBUF];
unsigned char SendBuff[MAXBUF];
unsigned long ulUser0, ulUser1;
unsigned long RcvCount, XmtCount;
unsigned char Me[6];
unsigned char All[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
volatile long FIR_Dist;
volatile long FIR_Ready;
OS_EthernetMailbox _OS_EthernetMailbox;
unsigned char Ethernet_okToSend = 0;

Fuzzy_Ethernet_State myState, hisState;
extern unsigned long IC_buffer[];
extern unsigned long PingBuff[];
extern unsigned long IR_Buff[];


#pragma O0
void OS_EthernetInit(void) {
  volatile unsigned long delay;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // activate port F
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD; // activate port D
  delay = SYSCTL_RCGC2_R;
  
  RcvCount = 0;
  XmtCount = 0;
  
  GPIO_PORTD_DIR_R |= 0x01;   // make PD0 out
  GPIO_PORTD_DEN_R |= 0x01;   // enable digital I/O on PD0
  GPIO_PORTF_DIR_R &= ~0x02;  // make PF1 in (PF1 built-in select button)
  GPIO_PORTF_DIR_R |= 0x01;   // make PF0 out (PF0 built-in LED)
  GPIO_PORTF_DEN_R |= 0x03;   // enable digital I/O on PF1, PF0
  GPIO_PORTF_PUR_R |= 0x02;   // PF1 has pullup
  
  // For the Ethernet Eval Kits, the MAC address will be stored in the
  // non-volatile USER0 and USER1 registers.  
  ulUser0 = FLASH_USERREG0_R;
  ulUser1 = FLASH_USERREG1_R;
  Me[0] = ((ulUser0 >>  0) & 0xff);
  Me[1] = ((ulUser0 >>  8) & 0xff);
  Me[2] = ((ulUser0 >> 16) & 0xff);
  Me[3] = ((ulUser1 >>  0) & 0xff);
  Me[4] = ((ulUser1 >>  8) & 0xff);
  Me[5] = ((ulUser1 >> 16) & 0xff);

  MAC_Init(Me);
  
  OS_EthernetMailBox_Init();
  
  OS_AddThread(&OS_EthernetListener, 128, 4);
  OS_AddThread(&OS_EthernetSender, 128, 4);
//   OS_AddThread(&EthernetTest, 128, 4);
  
	printf("Ethernet connected\n");
  Ethernet_okToSend = 1;
//   OS_Kill();
}

char str[32] = {0, };
void OS_EthernetListener(void) {
  unsigned long size;
  eFile_Init();
  while(1) {
    size = MAC_ReceiveNonBlocking(RcvMessage, sizeof(Fuzzy_State) + 14);
    if (size) {
			
			if(BOARD != 1){
      memcpy(hisState.byteArr, RcvMessage + 14, sizeof(Fuzzy_State));
      OLED_Init(15);
      sprintf(str, "fwd:%d            ", hisState.state.fwd);
      _OLED_Message(TOP, 0, str, 15);
      sprintf(str, "lft:%d            ", hisState.state.turn_left);
      _OLED_Message(TOP, 1, str, 15);
      sprintf(str, "rgt:%d             ", hisState.state.turn_right);
      _OLED_Message(TOP, 2, str, 15);
      sprintf(str, "%d,%d     ", hisState.state.PWM_left, hisState.state.PWM_right);
      _OLED_Message(TOP, 3, str, 15);
      
      sprintf(str, "%d,%d,%d,%d,%d          ",hisState.state.IRs[IR_LEFT],
			                              hisState.state.IRs[IR_FLEFT],
			                              hisState.state.ping,
			                              hisState.state.IRs[IR_FRIGHT],
			                              hisState.state.IRs[IR_RIGHT]);
      _OLED_Message(BOTTOM, 0, str, 15);
      sprintf(str, "Time:%d            ", hisState.state.time);
      _OLED_Message(BOTTOM, 1, str, 15);
      sprintf(str, "Lost:%d            ", hisState.state.lost);
      _OLED_Message(BOTTOM, 2, str, 15);
    } 
		
		else{
			//if recieving on board 1, it will be an ADC value
			FIR_Dist = *(long*)(RcvMessage + 14);
			FIR_Ready = 1;
		}
	  }
//     size = MAC_ReceiveNonBlocking(RcvMessage,MAXBUF);
//     if(size){
// 			
// 			
// 			
//       RcvCount++;
//       // if an SD card is available, dump messages to disk; otherwise print to OLED
//       memcpy(hisState.byteArr, RcvMessage + 14, sizeof(Fuzzy_State));
//       if(eFile_WOpen(LOG_FILE) == 0) {
//         for(i = 14; i < size; i++) {
// 					if(RcvMessage[i])
// 						eFile_Write(RcvMessage[i]);
//         }
//         eFile_Write('\n');
//         eFile_WClose();
//       }
//       else {
//         OLED_Init(15);  // repeated calls just return immediately
//         i = (i + 1) % 5;
// //         sprintf(str, "%d %s\n",size,RcvMessage+14);
//         
//         _OLED_Message(TOP, i, RcvMessage+14, 15);
//       }
// 			
// 			//recieve instruction
// 			//send instruction to controller (through FIFO? mailbox?)
// 			MoveCmd = *(Command*)(RcvMessage + 14);
// 			OS_bSignal(&CmdReady);
// 			
			
//     }
  }
}



void OS_EthernetSender(void) {
  while(1) {
//     printf("waiting to send\n");
    OS_EthernetMailBox_Recv();
//     printf("sending\n");
    MAC_SendData(SendBuff, _OS_EthernetMailbox.size, All);
  }
}



void EthernetTest(void) {
//   int i = 0;
  while(1) {
//     myState.state.tacho = IC_buffer[0];
//     myState.state.ping = PingBuff[0];
//     myState.state.IR =  IR_Buff[0];
//     OS_EthernetMailBox_Send(myState.byteArr, sizeof(Fuzzy_State));
//     printf("Sent %d, %d, %d\n", myState.state.tacho, myState.state.ping, myState.state.IR);
//     OS_Sleep(1000);
//     if(i++ % 2) {
//       OS_EthernetMailBox_Send("testtesttesttest", 17);
//     }
//     else {
//       OS_EthernetMailBox_Send("foofoofoofoo", 13);
//     }
//      OS_Sleep(1000);
  }
}

// ******** OS_EthernetMailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_EthernetMailBox_Init(void) {
  _OS_EthernetMailbox.size = 0;
  OS_InitSemaphore(&_OS_EthernetMailbox.hasData, 0);
  OS_InitSemaphore(&_OS_EthernetMailbox.gotData, 1);
}


// ******** OS_EthernetMailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_EthernetMailBox_Send(unsigned char* buffer, unsigned long size) {
  OS_bWait(&_OS_EthernetMailbox.gotData);
  size = (size > MAXBUF) ? MAXBUF : size;
  _OS_EthernetMailbox.size = size;
  memcpy(SendBuff, buffer, size);
  OS_bSignal(&_OS_EthernetMailbox.hasData);
}

// ******** OS_EthernetMailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
void OS_EthernetMailBox_Recv(void) {
  OS_bWait(&_OS_EthernetMailbox.hasData);
  OS_bSignal(&_OS_EthernetMailbox.gotData);
}

void OS_EthernetSendState(void) {
  OS_EthernetMailBox_Send(myState.byteArr, sizeof(Fuzzy_State));
}

void OS_EthernetSendVal(long *val){
	OS_EthernetMailBox_Send((unsigned char*) val, sizeof(unsigned long));
}
