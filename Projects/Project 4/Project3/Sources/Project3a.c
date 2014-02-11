                               #include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "atd.h"
#include "lcd.h"
#include "filters.h"
#include <stdio.h>

// quicksort function declaration
void quickSort(int*, int, int);

// global array and counter for averaging reads
volatile unsigned int mt_ovf = 0;
unsigned int d;
butterworth_2pole_t butter;
sliding_window_50_t slide;

void main(void) {
  /* put your own code here */
  unsigned int res = 8;
  unsigned int val;
  
  float butter_feedback[] = {-1.7786f, 0.8008f};                  // butterworth A scalars
  float butter_feedforward[] = {0.0055427, 0.011085, 0.0055427};  //butterworth B scalars
    
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
  
  butterworth_2pole_init(&butter, butter_feedback, butter_feedforward);
  sliding_window_50_init(&slide);
  
	EnableInterrupts;
	
  lcd_setup();     // set up the LCD to write

  for(;;) {
    _FEED_COP(); /* feeds the dog */
  } /* loop forever */
  /* please make sure that you never leave main */
}


#pragma TRAP_PROC
#pragma CODE_SEG __SHORT_SEG NON_BANKED
interrupt VectorNumber_Vrti void rtiInterrupt () {
  static unsigned int rti_count = 0, val;
  float avgSlide = 0;
  float avgButter = 0;
  unsigned char avgBuf[9] = {0}, timeBuf[9] = {0};
  unsigned int i, j;
  unsigned int start, end;
  double time_slide, time_butter;
  
  // ALWAYS CLEAR THE FLAG FIRST!
  CRGFLG |= 0x80;
  
  rti_count++;
  if (rti_count >= 2)   // every 100ms
  {
    // reset the counters and timing variables
    rti_count = 0;
    avgSlide = 0;
    avgButter = 0;
    start = 0;
    end = 0;

    val = atd0_readChX(0);        // read ATD ch 0
    
    mt_ovf = 0;
    start = TCNT;             // begin timing
    sliding_window_50_put(&slide, val);
    avgSlide = sliding_window_50_get_float(&slide);    // compute sliding window filter
    
    end = TCNT;// end timing 
                
    // compute the time it took to average sliding filter
    if(mt_ovf == 0)
       time_slide = (end - start) / (double)24.0;
    else
      time_slide = (((((mt_ovf) * 65536) + (end - start)) * 4) / ((double)24.0));
    
    start = TCNT;
    butterworth_2pole_put(&butter, val);
    avgButter = butterworth_2pole_get_float(&butter);   // compute butterworth filter (float)
    
    end = TCNT;
    
    // compute the time it took to average sliding filter
    if(mt_ovf == 0)
       time_butter = (end - start) / (double)24.0;
    else
      time_butter = (((((mt_ovf) * 65536) + (end - start)) * 4) / ((double)24.0));
    
    sprintf(avgBuf, "%3.0f %3.0f", avgSlide, avgButter);   
    sprintf(timeBuf, "%3.0f %3.0f", time_slide, time_butter); 
    // output the average and time on the LCD
    
    lcd_clear();   
    lcd_outputString(avgBuf);
    lcd_newLine();
    lcd_outputString(timeBuf);
  }
}

// used for timing the averaging
interrupt VectorNumber_Vtimovf void ovfInterrupt () 
{
  
  // ALWAYS CLEAR THE FLAG FIRST!
  TFLG2 |= 0x80;
  
  mt_ovf++;            // count ovfs
}