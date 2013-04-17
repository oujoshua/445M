//**************tachometerTimer1.c****************
//Uses edge triggered GPIO interrupts and 32-bit timer1 to measure the period of 2 tachometers
//Uses ports PC5, PC7 as tachometer input ports
//Edge triggered interrupts are priority 0

#include "lm3s8962.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "OS.h"
#include "eFile.h"
#include "eDisk.h"
#include <stdio.h>


volatile unsigned long period7, period5; //periods in units of 5 us
volatile unsigned long speed7, speed5; //speed of wheel rotation in units of .01 ticks per second (8 ticks per 1 wheel rotation)
unsigned long first7, first5;  //initial measurement times
volatile unsigned char Done7, Done5;    //set on each rising edge of PC5,7
char *DEBUG[64];
#pragma O0
void Timer1_Init(void)
{
	int nop = 5;
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1;	/* Activate timer1 */
	nop *= SYSCTL_RCGC1_TIMER2;							/* Wait for clock to activate */
	nop *= SYSCTL_RCGC1_TIMER2;							/* Wait for clock to activate */
	TIMER1_CTL_R &= ~0x00000001;						/* Disable timer1 during setup */
	TIMER1_CFG_R = 0x0000000;							/* Configure for 32 bit operation */
	TIMER1_TAMR_R = 0x00000002;							/* Configure for periodic mode */
	TIMER1_TAPR_R = 249;											/* 5us timer2A */
	//TIMER1_ICR_R |= 0x00000001;							/* Clear timer2A timeout flag */
	TIMER1_TAILR_R = 0xFFFFFFFF;										/* max reload times */
	TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;				/* disable timeout interrupt */
	TIMER1_CTL_R |= 0x00000001;           //enable Timer1

}


unsigned long tacho_Get_Time(void){
	return TIMER1_TAR_R;
}

unsigned long tacho_Time_Diff(unsigned long start, unsigned long stop){
	if(start > stop) {
    return start - stop;
  }
  else {
    return start + (0xFFFFFFFF - stop); // 0xFFFFFFFF is the maximum timer value
  }
}

/*********tacho_init*************/
//initializes tachometers for GPIO edge triggered interrupts
//Uses pins PC5, PC7
#pragma O0
void tacho_Init(void){ int nop = 5;
	Timer1_Init();
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOC;
	nop *= SYSCTL_RCGC2_GPIOC;							/* Wait for clock to activate */
	nop *= SYSCTL_RCGC2_GPIOC;							/* Wait for clock to activate */
	GPIO_PORTC_DEN_R |= 0xA0  ;              //digital enable on GPIO C5, C7
	GPIO_PORTC_DIR_R &= ~0xA0;               //inputs
	GPIO_PORTC_IS_R &= ~0xA0;                //PC5,7 is edge sensitive
	GPIO_PORTC_IBE_R &= ~0xA0;               //interrupt not on both edges
	GPIO_PORTC_IEV_R |= 0xA0;                //interrupt on rising edge of PC5,7
	GPIO_PORTC_ICR_R |= 0xA0; //clear interrupt flag
	GPIO_PORTC_IM_R |= 0xA0; //enable interrupts
	
	period5 = 0; period7 = 0;
	first5 = 0; first7 = 0;
	
	NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFFFF)|0x00000000; // 2nd location from left, priority 0
  NVIC_EN0_R |= NVIC_EN0_INT2;    // enable interrupt 18 in NVIC (vector 18, number 2 for port C)
}

GPIOPortC_Handler(){unsigned long newTime;
	//see which tachometer triggered the interrupt
	newTime = tacho_Get_Time();
	if((GPIO_PORTC_MIS_R&0x80)){//if masked interrupt status pin 7 is high, it caused the trigger
			GPIO_PORTC_ICR_R |= 0x80; //clear PC7 interrupt
		  period7 = (tacho_Time_Diff(first7, newTime))/250;
		  speed7 = 20000000/period7;
		  first7 = newTime;
		  Done7 = 0xFF;
	}
	
	if((GPIO_PORTC_MIS_R&0x20)){//if interrupted because of pin 5
			GPIO_PORTC_ICR_R |= 0x20; //clear PC5 interrupt
		  period5 = (tacho_Time_Diff(first5, newTime))/250;
		  speed5 = 20000000/period5;
		  first5 = newTime;
		  Done5 = 0xFF;
	}

}



void tachoRead(void){unsigned long i;
	eFile_Init();
	eFile_Delete("tachLog.txt");
	eFile_RedirectToFile("tachLog.txt");
	i = 0;
	while(i < 5){
		while(!(Done7&Done5)){}
			printf("tacho7 P=%d T=%d\r\n", period7, OS_Time());
			printf("tacho5 P=%d T=%d\r\n", period5, OS_Time());
			Done5 = 0; Done7 = 0;
			
		i++;
	}
	printf("DONE\r\n\r\n\r\n");
	eFile_EndRedirectToFile();
	

	OS_Kill();
	OS_Delay(OS_ARBITRARY_DELAY);
	
}

//tachometer test code
int testmain(void){
	OS_Init();
		
	tacho_Init();
	
	OS_Add_Periodic_Thread(&disk_timerproc,10,0);   // time out routines for disk
  OS_AddThread(&tachoRead, 128, 0);
  OS_Launch(TIME_2MS);
	
	return 0;
}
