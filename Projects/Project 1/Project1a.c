#include <hidef.h>      /* common defines and macros */
#include <math.h>
#include "derivative.h"      /* derivative-specific definitions */
#include "atd.h"

unsigned int val = 0;

void main(void) {
  unsigned int res = 8;
                          
  // 24Mhz BUS CLK
  SYNR = 2;
  REFDV = 0;
  while (!(CRGFLG&0x08)){};
  CLKSEL |= 0x80;

  atd0_powerOn();                 // Powers on ATD module 0
  atd0_setFFC(1);                 // set fast flag clearing on/off
  atd0_setLength(1);              // sets conversion sequence length of ATD module 0
  atd0_setFifo(0);                // turns FIFO mode on or off (1/0)
  atd0_setResolution(res);          // sets ATD module 0 resolution to 8/10bit
  atd0_setJustification('r');     // 'L' for left, 'R for right
  atd0_setScan(1);                // sets ATD Module 0 SCAN bit on/off (1/0)
  atd0_setMulti(0);               // sets ATD Module 0 MULT bit on/off (1/0)
  atd0_setStart(2);               // sets starting channel of ATD conversion sequence (call Last)
  
  // set up 7-segment display  
  DDRH = 0xFF;

  // set up MDCU
  MCCTL = 0xCF;
  MCCNT = 40;
  MCCTL = 0xCC;

	EnableInterrupts;
	
	while(1)
	{
	  PTH = 0x00;
	  val = atd0_readChX(0);            // reads data registers of corresponding ATD Data Reg   
	}
  for(;;) {
    _FEED_COP(); /* feeds the dog */
  } /* loop forever */
  /* please make sure that you never leave main */
}

#pragma TRAP_PROC
#pragma CODE_SEG __SHORT_SEG NON_BANKED
interrupt VectorNumber_Vtimmdcu void mdcuInterrupt () {
  static unsigned int mdcu_count = 0;
  
  // ALWAYS CLEAR THE FLAG FIRST!
  MCFLG |= 0x80; 
  if (mdcu_count >= val / 8)
  {
    mdcu_count = 0;       
    PTH = 0x7F; 
  }
  else
    mdcu_count++;
}


