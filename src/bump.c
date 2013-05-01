#include "OS.h"
#include "hw_types.h"
#include "lm3s8962.h"
#include <stdlib.h>

void (*bumpTask)(void);

//bump sensor initialization
//WARNING: CANNOT USE WITH PING ON PORT PC57!!!!!!!
void bump_Init(void (*task)(void)){
	
	
	  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOC;

	bumpTask = task;
	GPIO_PORTC_PUR_R |= 0x80;
	GPIO_PORTC_DEN_R |= 0x08;
	GPIO_PORTC_DIR_R &= ~0x80;
	GPIO_PORTC_IS_R &= ~0x80; //edge trigger
	GPIO_PORTC_IBE_R &= ~0x80; //not both edges
	GPIO_PORTC_IEV_R &= ~0x80; //falling edge
	GPIO_PORTC_ICR_R |= 0x80;
	GPIO_PORTC_IM_R  |= 0x80;
	
	  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFF)|0x00000000; // 2nd to top set of bits
  NVIC_EN0_R |= NVIC_EN0_INT2;    // enable interrupt 18 in NVIC	
}


GPIOPortC_Handler(void){
	
	GPIO_PORTC_ICR_R |= 0x80;
	(*bumpTask)();
}
