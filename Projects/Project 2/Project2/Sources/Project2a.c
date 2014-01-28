#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "atd.h"
#include "lcd.h"

volatile unsigned int distances8[8] = 0;
volatile unsigned int distances50[50] = 0;

void main(void) {
  /* put your own code here */
  unsigned int res = 10;
  unsigned int i, j, val;

    
  SYNR = 2;                    // 24mHz clock
  REFDV = 0;
  while (!(CRGFLG&0x08)){};
  CLKSEL |= 0x80;
  
  RTICTL = 0b01110101;         // rti every 50ms
  CRGINT = 0b10000000;

  atd0_powerOn();              // Powers on ATD module 0
  atd0_setFFC(1);              // set fast flag clearing on/off
  atd0_setLength(1);           // sets conversion sequence length of ATD module 0
  atd0_setFifo(0);             // turns FIFO mode on or off (1/0)
  atd0_setResolution(res);     // sets ATD module 0 resolution to 8/10bit
  atd0_setJustification('r');  // 'L' for left, 'R for right
  atd0_setScan(1);             // sets ATD Module 0 SCAN bit on/off (1/0)
  atd0_setMulti(0);            // sets ATD Module 0 MULT bit on/off (1/0)
  atd0_setStart(2);            // sets starting ch
  
	EnableInterrupts;
	
  lcd_setup();                 // sets up LCD to write
  	
	
  while(1)
  {   
    if(i > 7)
      i = 0; 
    if(j > 49)
      j = 0;
    
    val = atd0_readChX(0);      // reads the IR sensors atd channel
    distances8[i] = val;        // store value in 8 element array
    distances50[j] = val;	      // store value in 50 element array
    i++;
    j++;   
  }
	

  for(;;) {
    _FEED_COP(); /* feeds the dog */
  } /* loop forever */
  /* please make sure that you never leave main */
}


#pragma TRAP_PROC
#pragma CODE_SEG __SHORT_SEG NON_BANKED
interrupt VectorNumber_Vrti void rtiInterrupt () {
  static unsigned int rti_count = 0;
  float avg8 = 0;
  float avg50 = 0;
  unsigned int i, j;
  
  // ALWAYS CLEAR THE FLAG FIRST!
  CRGFLG |= 0x80;
  
  rti_count++;
  if (rti_count >= 2)     // every 100ms
  {
    rti_count = 0;
    avg8 = 0;
    avg50 = 0;
  
    // average the last 8 reads
    for(i = 0; i < 7; i++)         
      avg8 += distances8[i];
    avg8 = avg8 / (float)8.0;
    
    // average the last 50 reads
    for(j = 0; j < 49; j++)        
      avg50 += distances50[j];
    avg50 = avg50 / (float)54.0;
    
    // write the value to the screen
    lcd_clear();
    lcd_outputFloat(avg8);
    lcd_newLine();
    lcd_outputFloat(avg50);
  }
}