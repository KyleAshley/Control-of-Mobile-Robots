                               #include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "atd.h"
#include "lcd.h"
#include "filtersFixed.h"
#include "filters.h"
#include <stdio.h>
#include "delays.h"

#define FAR 100
// global array and counter for averaging reads
volatile unsigned int mt_ovf = 0;
unsigned int d;
butterworth_2pole_t butter;
butterworth_2pole_ft butterF;
long distance1;


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
  
  RTICTL = 0b01110101;     // every 50ms
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
  butterworth_2pole_init(&butter, butter_feedback, butter_feedforward);
  
	EnableInterrupts;
	
  lcd_setup();     // set up the LCD to write

  for(;;) {
  
  if(distance1 > FAR)
  {
     motor_stepForward(1,1,600);
  }
  
    //val = atd0_readChX(0);
    //sliding_window_50_put(&slide, val);
    //butterworth_2pole_put(&butter, val);
    _FEED_COP(); /* feeds the dog */
  } /* loop forever */
  /* please make sure that you never leave main */
}


#pragma TRAP_PROC
#pragma CODE_SEG __SHORT_SEG NON_BANKED
interrupt VectorNumber_Vrti void rtiInterrupt () {
  static unsigned int rti_count = 0;
  long val;                                       // sensor read
  float avgButter = 0;                            // floating filter val
  int avgButter2 = 0;                             // fixed filter val
  unsigned char avgBuf[9] = {0},                  // output buffers
                timeBuf[9] = {0}, 
                avgBufF[9] = {0};
  unsigned int i, j;
  unsigned int start, end;
  int time_butter, time_butter2;
  
  // ALWAYS CLEAR THE FLAG FIRST!
  CRGFLG |= 0x80;
  
  rti_count++;
  if (rti_count >= 2)   // every 100ms
  {
    // reset the counters and timing variables
    rti_count = 0;
    avgButter = 0;
    start = 0;
    end = 0;

    val = atd0_readChX(0);                        // read atd ch0
    
    mt_ovf = 0;
    start = TCNT;
   
    butterworth_2pole_put(&butter, val);               // compute float filter
    avgButter = butterworth_2pole_get_float(&butter);
    
    end = TCNT;
    
    // compute the time it took to average float filter
    if(mt_ovf == 0)
       time_butter = (end - start) / (double)24.0;
    else
      time_butter = (((((mt_ovf) * 65536) + (end - start)) * 4) / ((double)24.0));
    
    start = TCNT;
    
    butterworth_2pole_putF(&butterF, val);        //compute fixed filter
    avgButter2 = butterworth_2pole_get_longF(&butterF);
    distance1=avgButter2;
    end = TCNT;
    
    // compute the time it took to average fixed filter
    if(mt_ovf == 0)
       time_butter2 = (end - start) / (double)24.0;
    else
      time_butter2 = (((((mt_ovf) * 65536) + (end - start)) * 4) / ((double)24.0));
    
    sprintf(avgBuf, "%3.0f", avgButter);
    sprintf(avgBufF, "%3d", avgButter2);   
    sprintf(timeBuf, "%3d %3d", time_butter, time_butter2); 
    //output the average and time on the LCD
    lcd_clear();   
    lcd_outputString(avgBuf);
    lcd_outputChar(' ');
    lcd_outputString(avgBufF);
    lcd_newLine();
    lcd_outputString(timeBuf);
  }
}

// used for timing the averaging
interrupt VectorNumber_Vtimovf void ovfInterrupt () 
{
  
  // ALWAYS CLEAR THE FLAG FIRST!
  TFLG2 |= 0x80;
  
  mt_ovf++;          // count ovfs
}
