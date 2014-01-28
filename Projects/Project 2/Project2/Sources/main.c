#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "atd.h"
#include "lcd.h"

volatile unsigned int distances8[8];
volatile unsigned int distances50[50];

void main(void) {
  /* put your own code here */
  unsigned int res = 10;
  unsigned int i, j, val;

  
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
  atd0_setStart(2);               // sets starting cha

  // set up MDCU
  MCCTL = 0xCF;
  MCCNT = 1500;
  MCCTL = 0xCF;
  
  lcd_setup();
  
	EnableInterrupts;
	
	while(1)
	{
	  for(j = 0; j < 50; i++, j++)
	  {  
	    val = atd0_readChX(0);
	    distances8[i] = val;
	    distances50[j] = val;
	    if(i >= 7)
	      i = 0;
	  }
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
  unsigned int avg8 = 0;
  unsigned int avg50 = 0;
  unsigned int i, j;
  
  // ALWAYS CLEAR THE FLAG FIRST!
  MCFLG |= 0x80; 
  if (mdcu_count >= 100)
  {
    mdcu_count = 0;
  
    for(i = 0; i < 8; i++)
      avg8 += distances8[i];
    avg8 /= 8;
    
    for(j = 0; j < 50; j++)
      avg50 += distances50[j];
    avg50 /= 50;
    
    lcd_outputFloat(avg8);
    lcd_newLine();
    lcd_outputFloat(avg50);
  }
  else
    mdcu_count++;
}