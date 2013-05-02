#include "stdio.h"
#include "commands.h"
#include "PWM.h"
#include "eFile.h"
#include "OS.h"
#include "lm3s8962.h"


#define PB4 (*((volatile unsigned long *)0x40005040))
#define PB6 (*((volatile unsigned long *)0x40005100))

#pragma O0
void InitCommands(void){
  volatile long delay;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;
  delay = SYSCTL_RCGC2_R;

  
  GPIO_PORTB_DEN_R |= 0x50;        // enable digital I/O on PB4,6
  GPIO_PORTB_DIR_R |= 0x50;        //set PB4,6 to output
  PB4 = ~0x10;//go forward
  PB6 = ~0x40;//go forward
}

void Reverse(void){
  PB4 = 0x10;//reverse
  PB6 = 0x40;//reverse
}

void Forward(void){
  PB4 = ~0x10;//go forward
  PB6 = ~0x40;//go forward
}

void LeftForward_RightReverse(void){
  PB4 = ~0x10;//go forward
  PB6 = 0x40;//reverse
}

void LeftReverse_RightForward(void){
  PB4 = 0x10;//reverse
  PB6 = ~0x40;//go forward
}

void GoStraight(void) {
//  PWM1_SetADuty(PWM_FAST-2200);
  PWM1_SetADuty(PWM_FAST);
  PWM1_SetBDuty(PWM_FAST);
}

void TurnRight(int speed) {
//  PWM1_SetADuty(PWM_FAST-2200);
  PWM1_SetADuty(PWM_FAST);
  PWM1_SetBDuty(speed);
}

void TurnLeft(int speed) {
//  PWM1_SetADuty(speed-2200);
  PWM1_SetADuty(speed);
  PWM1_SetBDuty(PWM_FAST);
}

void Stop(void) {
  PWM1_SetADuty(1);
  PWM1_SetBDuty(1);
}

// needs to be updated for multiple sensors
/*
void dataHandler(EthernetFrame* frame)
{ 
	unsigned long *dist = frame->Ping_data, *IR_Dist = frame->IR_data;
	eFile_RedirectToFile("DEBUG.txt");
	
	printf("%d\n", OS_MsTime());
	printf("PING: %d     IR: %d,%d\n", dist[0], IR_Dist[0], IR_Dist[1]);
  if(dist[0] < 90)
	{
    // turn whichever direction is more open
    if(IR_Dist[0] > IR_Dist[1]){
      TurnRight(HARD);
		  printf("Hard Right\n");
    }else{
      TurnLeft(HARD);
			printf("Hard Left\n");
		}
  }
	else if(IR_Dist[0] > 10 && IR_Dist[0] < 20)
	{
    TurnLeft(SOFT);
		printf("Soft Left\n");
  }
  else if(IR_Dist[1] > 10 && IR_Dist[1] < 20)
	{
    TurnRight(SOFT);
		printf("Soft Right\n");
  }
  else
	{
    GoStraight();
		printf("Straight\n");
  }
	eFile_EndRedirectToFile();
}*/
