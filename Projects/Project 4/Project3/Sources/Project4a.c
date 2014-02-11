                               #include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "atd.h"
#include "lcd.h"
#include "filtersFixed.h"
#include "filters.h"
#include <stdio.h>
#include "delays.h"



// global array and counter for averaging reads
volatile unsigned int mt_ovf = 0;
unsigned int d;
butterworth_2pole_ft butterF;
long distance1;
int avgButter2 = 0;                             // fixed filter val
unsigned char avgBufF[9] = {0};                 // filter output buffer

const int Rstep[] = {0xC0, 0x60, 0x90, 0xA0};
const int Lstep[] = {0x0C, 0x06, 0x09, 0x0A};
int Lspeed = 50;
int Rspeed = 50;

void main(void) {
  /* put your own code here */
  unsigned int res = 8;
  
  float butter_feedback[] = {-1.7786f, 0.8008f};                        // float butterworth A scalars
  float butter_feedforward[] = {0.0055427, 0.011085, 0.0055427};        // float butterworth B scalars
  
  long butter_feedbackF[] = {-116562, 52481};                          // scaling factor Q16.16
  long butter_feedforwardF[] = {363, 726, 363};                        // scaling factor Q16.16
      
      
  SYNR = 2;                 // 24mHz clock
  REFDV = 0;
  while (!(CRGFLG&0x08)){};
  CLKSEL |= 0x80;
  
  RTICTL = 0b00010111;     // every 1ms
  CRGINT = 0b10000000;
  
  TSCR1 = 0x80;            // set up MT
  TSCR2 = 0x82;

  atd0_powerOn();              // Powers on ATD module 0
  atd0_setFFC(1);              // set fast flag clearing on/off
  atd0_setLength(1);           // sets conversion sequence length of ATD module 0
  atd0_setFifo(0);             // turns FIFO mode on or off (1/0)
  atd0_setResolution(res);     // sets ATD module 0 resolution to 8/10bit
  atd0_setJustification('r');  // 'L' for left, 'R for right
  atd0_setScan(1);             // sets ATD Module 0 SCAN bit on/off (1/0)
  atd0_setMulti(0);            // sets ATD Module 0 MULT bit on/off (1/0)
  atd0_setStart(2);            // sets starting ch
  
  butterworth_2pole_initF(&butterF, butter_feedbackF, butter_feedforwardF);
  
	EnableInterrupts;
	
  lcd_setup();     // set up the LCD to write

  for(;;) {
  
    sprintf(avgBufF, "%3d", avgButter2);   
    //output the average and time on the LCD
    lcd_clear();   
    lcd_outputString(avgBufF);
    _FEED_COP(); /* feeds the dog */
  } /* loop forever */
  /* please make sure that you never leave main */
}


#pragma TRAP_PROC
#pragma CODE_SEG __SHORT_SEG NON_BANKED
interrupt VectorNumber_Vrti void rtiInterrupt () {
  static unsigned int rti_count = 0;
  long val;                                       // sensor read
  
  unsigned int i, j;
  unsigned int start, end;
  int time_butter, time_butter2;
  
  static int rIndex = 0;
  static int lIndex = 0;
  static int Rmotcount, Lmotcount;
  unsigned char Rout, Lout;
  static unsigned int Lindex, Rindex;
 
  
  // ALWAYS CLEAR THE FLAG FIRST!
  CRGFLG |= 0x80;
  
  rti_count++;
  Rmotcount++;
  Lmotcount++;
  if (rti_count >= 100)   // every 100ms
  {
    // reset the counters and timing variables
    rti_count = 0;
    //start = 0;
    //end = 0;

    val = atd0_readChX(0);                        // read atd ch0
    
    mt_ovf = 0;
    
    //start = TCNT;
    butterworth_2pole_putF(&butterF, val);        //compute fixed filter
    avgButter2 = butterworth_2pole_get_longF(&butterF);
    distance1=avgButter2;
    //end = TCNT;
    Rspeed = 4;
    Lspeed = 4;
  }
    
  
  if(Rmotcount >= Rspeed || Rmotcount >= Lspeed)
  {
    
    if(Rmotcount >= Rspeed)
    {
      Rout=Rstep[Rindex];
      Rindex++;
      if(Rindex >= 4)
        Rindex = 0;
      Rmotcount = 0;
    }
    
    if(Lmotcount >= Lspeed)
    {
      Lout = Lstep[Lindex];
      Lindex++;
      if(Lindex >=4)
        Lindex=0;
      Lmotcount = 0;
    }
      
    PORTB = Rout + Lout;
  }
}

// used for timing the averaging
interrupt VectorNumber_Vtimovf void ovfInterrupt () 
{
  
  // ALWAYS CLEAR THE FLAG FIRST!
  TFLG2 |= 0x80;
  
  mt_ovf++;          // count ovfs
}
/*
#pragma TRAP_PROC
#pragma CODE_SEG __SHORT_SEG NON_BANKED
interrupt VectorNumber_Vtimmdcu void mdcuInterrupt () {
  static int mdcu_count = 0;
  static int on = 0;
  
  // ALWAYS CLEAR THE FLAG FIRST!
  MCFLG |= 0x80;
  
  mdcu_count++;
  if (mdcu_count >= 100) {
    mdcu_count = 0;
    if (on==1) {
      on = 0;
      PTH_PTH3 = 1;
    } else {
      on = 1;
      PTH_PTH3 = 0;
    }
  }
}
    */