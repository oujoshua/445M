// UART2.c
// Runs on LM3S1968
// Use UART0 to implement bidirectional data transfer to and from a
// computer running HyperTerminal.  This time, interrupts and FIFOs
// are used.
// Daniel Valvano
// October 9, 2011
// Modified by EE345L students Charlie Gough && Matt Hawk
// Modified by EE345M students Agustinus Darmawan && Mingjie Qiu

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011

   Program 5.11 Section 5.6, Program 3.10

 Copyright 2011 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include "OS.h"
#include "shell.h"
#include "UART.h"
#include <stdio.h>
#include <string.h>

#define NVIC_EN0_INT5           0x00000020  // Interrupt 5 enable
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))  // IRQ 0 to 31 Set Enable Register
#define NVIC_PRI1_R             (*((volatile unsigned long *)0xE000E404))  // IRQ 4 to 7 Priority Register
#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
#define UART0_DR_R              (*((volatile unsigned long *)0x4000C000))
#define UART0_FR_R              (*((volatile unsigned long *)0x4000C018))
#define UART0_IBRD_R            (*((volatile unsigned long *)0x4000C024))
#define UART0_FBRD_R            (*((volatile unsigned long *)0x4000C028))
#define UART0_LCRH_R            (*((volatile unsigned long *)0x4000C02C))
#define UART0_CTL_R             (*((volatile unsigned long *)0x4000C030))
#define UART0_IFLS_R            (*((volatile unsigned long *)0x4000C034))
#define UART0_IM_R              (*((volatile unsigned long *)0x4000C038))
#define UART0_RIS_R             (*((volatile unsigned long *)0x4000C03C))
#define UART0_ICR_R             (*((volatile unsigned long *)0x4000C044))
#define UART_FR_RXFF            0x00000040  // UART Receive FIFO Full
#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART_CTL_UARTEN         0x00000001  // UART Enable
#define UART_IFLS_RX1_8         0x00000000  // RX FIFO >= 1/8 full
#define UART_IFLS_TX1_8         0x00000000  // TX FIFO <= 1/8 full
#define UART_IM_RTIM            0x00000040  // UART Receive Time-Out Interrupt
                                            // Mask
#define UART_IM_TXIM            0x00000020  // UART Transmit Interrupt Mask
#define UART_IM_RXIM            0x00000010  // UART Receive Interrupt Mask
#define UART_RIS_RTRIS          0x00000040  // UART Receive Time-Out Raw
                                            // Interrupt Status
#define UART_RIS_TXRIS          0x00000020  // UART Transmit Raw Interrupt
                                            // Status
#define UART_RIS_RXRIS          0x00000010  // UART Receive Raw Interrupt
                                            // Status
#define UART_ICR_RTIC           0x00000040  // Receive Time-Out Interrupt Clear
#define UART_ICR_TXIC           0x00000020  // Transmit Interrupt Clear
#define UART_ICR_RXIC           0x00000010  // Receive Interrupt Clear
#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC1_UART0      0x00000001  // UART0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

void RxFifo_Init(void);
int RxFifo_Put(char data);
char RxFifo_Get(char* data);
long RxFifo_Size(void);
void TxFifo_Init(void);
int TxFifo_Put(char data);
char TxFifo_Get(char* data);
long TxFifo_Size(void);

#define FIFOSIZE   16         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure
                              // create index implementation FIFO (see FIFO.h)

OS_SemaphoreType RxFifoMutex, RxFifoDataAvail, RxFifoDataRoomLeft;
OS_SemaphoreType TxFifoDataAvail, TxFifoMutex, TxFifoDataRoomLeft;
char *RxFifoPutPt, *RxFifoGetPt;
char *TxFifoPutPt;
char *TxFifoGetPt;
char RxFifo[FIFOSIZE];
char TxFifo[FIFOSIZE];
// Initialize UART0
// Baud rate is 115200 bits/sec
void UART_Init(void){
  volatile long delay;
  OS_InitSemaphore(&TxFifoDataAvail,0);
  OS_InitSemaphore(&TxFifoDataRoomLeft, FIFOSIZE);
  OS_InitSemaphore(&TxFifoMutex, 1);
  OS_InitSemaphore(&RxFifoDataAvail,0);
  OS_InitSemaphore(&RxFifoDataRoomLeft, FIFOSIZE);
  OS_InitSemaphore(&RxFifoMutex, 1);
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART0; // activate UART0
  delay = SYSCTL_RCGC1_R;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // activate port A
  delay = SYSCTL_RCGC2_R;
  RxFifo_Init();                        // initialize empty FIFOs
  TxFifo_Init();
  UART0_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART0_IBRD_R = 27;                    // IBRD = int(50,000,000 / (16 * 115,200)) = int(27.1267)
  UART0_FBRD_R = 8;                     // FBRD = int(0.1267 * 64 + 0.5) = 8
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
  UART0_IFLS_R &= ~0x3F;                // clear TX and RX interrupt FIFO level fields
                                        // configure interrupt for TX FIFO <= 1/8 full
                                        // configure interrupt for RX FIFO >= 1/8 full
  UART0_IFLS_R += (UART_IFLS_TX1_8|UART_IFLS_RX1_8);
                                        // enable TX and RX FIFO interrupts and RX time-out interrupt
  UART0_IM_R |= (UART_IM_RXIM|UART_IM_TXIM|UART_IM_RTIM);
  UART0_CTL_R |= UART_CTL_UARTEN;       // enable UART
  GPIO_PORTA_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
  GPIO_PORTA_DEN_R |= 0x03;             // enable digital I/O on PA1-0
                                        // UART0=priority 2
  NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFF00FF)|0x0000E000; // bits 13-15
  NVIC_EN0_R |= NVIC_EN0_INT5;          // enable interrupt 5 in NVIC
}
// copy from hardware RX FIFO to software RX FIFO
// stop when hardware RX FIFO is empty or software RX FIFO is full
void static copyHardwareToSoftware(void){
  char letter;
  while(((UART0_FR_R&UART_FR_RXFE) == 0) && (RxFifo_Size() < (FIFOSIZE - 1))){
    letter = UART0_DR_R;
    RxFifo_Put(letter);
    OS_Signal(&RxFifoDataAvail);
  }
}
// copy from software TX FIFO to hardware TX FIFO
// stop when software TX FIFO is empty or hardware TX FIFO is full
void static copySoftwareToHardware(void){
  char letter;
  while(((UART0_FR_R&UART_FR_TXFF) == 0) && (TxFifo_Size() > 0)){

    TxFifo_Get(&letter);
    UART0_DR_R = letter;
    OS_Signal(&TxFifoDataRoomLeft);
  }
}
// input ASCII character from UART
// spin if RxFifo is empty
unsigned char UART_InChar(void){
   //Semaphore locks acquisition
     char letter;
  OS_Wait(&RxFifoDataAvail);
  OS_Wait(&RxFifoMutex);
  while(RxFifo_Get(&letter) == FIFOFAIL){};
   //Semaphore lock release
  OS_Signal(&RxFifoMutex);
  return(letter);
}
// output ASCII character to UART
// spin if TxFifo is full
void UART_OutChar(unsigned char data){
     //Semaphore locks
  OS_Wait(&TxFifoDataRoomLeft);
  OS_Wait(&TxFifoMutex);
  while(TxFifo_Put(data) == FIFOFAIL){};
  UART0_IM_R &= ~UART_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART0_IM_R |= UART_IM_TXIM;           // enable TX FIFO interrupt
  OS_Signal(&TxFifoMutex);
}
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART receiver has timed out
void UART0_Handler(void){
  if(UART0_RIS_R&UART_RIS_TXRIS){       // hardware TX FIFO <= 2 items
    UART0_ICR_R = UART_ICR_TXIC;        // acknowledge TX FIFO
    // copy from software TX FIFO to hardware TX FIFO
    copySoftwareToHardware();
    if(TxFifo_Size() == 0){             // software TX FIFO is empty
      UART0_IM_R &= ~UART_IM_TXIM;      // disable TX FIFO interrupt
    }
  }
  if(UART0_RIS_R&UART_RIS_RXRIS){       // hardware RX FIFO >= 2 items
    UART0_ICR_R = UART_ICR_RXIC;        // acknowledge RX FIFO
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
  if(UART0_RIS_R&UART_RIS_RTRIS){       // receiver timed out
    UART0_ICR_R = UART_ICR_RTIC;        // acknowledge receiver time out
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();

  }
}


//------------UART_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART_OutString(char *pt){
  while(*pt){
    UART_OutChar(*pt);
    pt++;
  }
}

//------------UART_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART_InUDec(void){
unsigned long number=0, length=0;
char character;
  character = UART_InChar();
  while(character != CR){ // accepts until <enter> is typed
// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART_OutChar(character);
    }
// If the input is a backspace, then the return number is
// changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  return number;
}

//-----------------------UART_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART_OutUDec(n/10);
    n = n%10;
  }
  UART_OutChar(n+'0'); /* n is between 0 and 9 */
}

//---------------------UART_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART_InUHex(void){
unsigned long number=0, digit, length=0;
char character;
  character = UART_InChar();
  while(character != CR){
    digit = 0x10; // assume bad
    if((character>='0') && (character<='9')){
      digit = character-'0';
    }
    else if((character>='A') && (character<='F')){
      digit = (character-'A')+0xA;
    }
    else if((character>='a') && (character<='f')){
      digit = (character-'a')+0xA;
    }
// If the character is not 0-9 or A-F, it is ignored and not echoed
    if(digit <= 0xF){
      number = number*0x10+digit;
      length++;
      UART_OutChar(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if((character==BS) && length){
      number /= 0x10;
      length--;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  return number;
}

//--------------------------UART_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART_OutUHex(unsigned long number){
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
  if(number >= 0x10){
    UART_OutUHex(number/0x10);
    UART_OutUHex(number%0x10);
  }
  else{
    if(number < 0xA){
      UART_OutChar(number+'0');
     }
    else{
      UART_OutChar((number-0x0A)+'A');
    }
  }
}

//------------UART_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --

void UART_InString(char *bufPt, unsigned short max) {
	int length = 0;
	char character;
	char* startPt = bufPt;
  character = UART_InChar();
  while(character != CR && character != LF && character != CTRL_C)
	{
    if(character == BS || character == DEL){
      if(length){
        bufPt--;
        length--;
				printf("\b \b");
      }
    }
		else if(character == CTRL_L)
		{
			int i;
			printf("\f%s", _SH_getVar(SH_PROMPT_NAME));
			for(i = 0; i < length; i++)
				UART_OutChar(startPt[i]);
		}
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  *bufPt = 0;
}



// ******** RxFifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void RxFifo_Init(void){
  //long sr;
  //sr = StartCritical();      // make atomic
  RxFifoPutPt = RxFifoGetPt = &RxFifo[0]; // Empty
  //OS_InitSemaphore(&RxFifoMutex, 1);
  //OS_InitSemaphore(&RxFifoDataAvail, 0);
  //OS_InitSemaphore(&RxFifoDataRoomLeft, FIFOSIZE);
  //EndCritical(sr);
}

// ******** RxFifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int RxFifo_Put(char data){
  char *nextPutPt;
  //OS_Wait(&OSFifoDataRoomLeft);
  nextPutPt = RxFifoPutPt + 1;
  if(nextPutPt == &RxFifo[FIFOSIZE]){
    nextPutPt = &RxFifo[0];
  }
  if(nextPutPt == RxFifoGetPt){
    return 0;
  }
  *(RxFifoPutPt) = data;
  RxFifoPutPt = nextPutPt;
  //OS_Signal(&RxFifoMutex);
 // OS_Signal(&RxFifoDataAvail);
  return 1;
}

// ******** RxFifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
char RxFifo_Get(char* data){
  char retVal;
 // OS_Wait(&RxFifoDataAvail);
  //OS_Wait(&RxFifoMutex);
  if(RxFifoGetPt == RxFifoPutPt){
return 0;
  }
  retVal = *(RxFifoGetPt++);
  *(data) = retVal;
  if(RxFifoGetPt == &RxFifo[FIFOSIZE]){
    RxFifoGetPt = &RxFifo[0];
  }
  //OS_Signal(&RxFifoMutex);
  //OS_Signal(&OSFifoDataRoomLeft);
  return retVal;
}

// ******** RxFifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero  if a call to OS_Fifo_Get will spin or block

long RxFifo_Size(void){
  if(RxFifoPutPt < RxFifoGetPt){
    return ((long)(RxFifoPutPt-RxFifoGetPt+(FIFOSIZE*sizeof(char)))/sizeof(char));
  }
  return ((long)(RxFifoPutPt-RxFifoGetPt)/sizeof(char));
}

// ******** TxFifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void TxFifo_Init(void){
  //long sr;
  //sr = StartCritical();      // make atomic
  TxFifoPutPt = TxFifoGetPt = &TxFifo[0]; // Empty
  //OS_InitSemaphore(&TxFifoMutex, 1);
  //OS_InitSemaphore(&TxFifoDataAvail, 0);
  //OS_InitSemaphore(&TxFifoDataRoomLeft, FIFOSIZE);
  //EndCritical(sr);
}

// ******** TxFifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int TxFifo_Put(char data){
  char *nextPutPt;
  //OS_Wait(&OSFifoDataRoomLeft);
  //OS_Wait(&OSFifoMutex);
  nextPutPt = TxFifoPutPt + 1;
  if(nextPutPt == &TxFifo[FIFOSIZE]){
    nextPutPt = &TxFifo[0];
  }
  if(nextPutPt == TxFifoGetPt){
    return 0;
  }
  *(TxFifoPutPt) = data;
//  *(TxFifoPutPt)=15;// try to see contents
  TxFifoPutPt = nextPutPt;
  //OS_Signal(&TxFifoMutex);
 // OS_Signal(&TxFifoDataAvail);
  return 1;
}

// ******** TxFifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
char TxFifo_Get(char* data){
  char retVal;
 // OS_Wait(&TxFifoDataAvail);
  //OS_Wait(&TxFifoMutex);
  if(TxFifoGetPt == TxFifoPutPt){
    return 0;
  }
  retVal = *(TxFifoGetPt++);
  *(data)=retVal;
  if(TxFifoGetPt == &TxFifo[FIFOSIZE]){
    TxFifoGetPt = &TxFifo[0];
  }
  
  //OS_Signal(&TxFifoMutex);
  //OS_Signal(&OSFifoDataRoomLeft);
  return retVal;
}

// ******** TxFifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero  if a call to OS_Fifo_Get will spin or block
long TxFifo_Size(void){
  if(TxFifoPutPt < TxFifoGetPt){
    return ((long)(TxFifoPutPt-TxFifoGetPt+(FIFOSIZE*sizeof(char)))/sizeof(char));
  }
  return ((long)(TxFifoPutPt-TxFifoGetPt)/sizeof(char));
}
