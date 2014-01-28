                               #include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "atd.h"
#include "lcd.h"

// quicksort function declaration
void quickSort(int*, int, int);

// global array and counter for averaging reads
volatile unsigned int distances10[10];   
volatile unsigned int mt_ovf = 0;

void main(void) {
  /* put your own code here */
  unsigned int res = 10;
  unsigned int i, val;

    
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
  
	EnableInterrupts;
	
  lcd_setup();     // set up the LCD to write
  	
	while(1)
	{
	    if(i > 9)
	      i = 0; 
	    
	    // read from ATD channel and store in an array of size 10
	    val = atd0_readChX(0);
	    distances10[i] = val;
	    i++;  
	  
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
  float avg6 = 0;
  unsigned int i, j;
  unsigned int start, end;
  double time;
  unsigned int buffer[10] = {0,0,0,0,0,0,0,0,0,0};
  
  // ALWAYS CLEAR THE FLAG FIRST!
  CRGFLG |= 0x80;
  
  rti_count++;
  if (rti_count >= 2)   // every 100ms
  {
    // reset the counters and timing variables
    rti_count = 0;
    avg6 = 0;
    time = 0;
    start = 0;
    end = 0;
    
    // copy the values to a buffer to be sorted
    for(i = 0; i < 9; i++)
      buffer[i] = distances10[i];
    
    
    mt_ovf = 0;
    start = TCNT;             // begin timing
    quickSort(buffer, 0, 9);  // sort the array from greatest to least
    
    // average the middle 6 elements, disregarding 2 most/least extreme reads              
    for(i = 2; i < 8; i++)
      avg6 += buffer[i];
    avg6 = (float) avg6 / 6;
    end = TCNT;                  // end timing 
    
    // compute the time it took to average
    if(mt_ovf == 0)
       time = (end - start) / (double)24000000.0;
    else
      time = (((((mt_ovf) * 65536) + (end - start)) * 4) / ((double)24000000.0));

    // output the average and time on the LCD
    lcd_clear();
    lcd_outputFloat(avg6);
    lcd_newLine();    lcd_outputDouble(time);
  }
}

// used for timing the averaging
interrupt VectorNumber_Vtimovf void ovfInterrupt () 
{
  
  // ALWAYS CLEAR THE FLAG FIRST!
  TFLG2 |= 0x80;
  
  mt_ovf++;
}

// generic quicksort algorithm
void quickSort(int x[],int first,int last){
    int pivot,j,temp,i;

     if(first<last)
     {
         pivot=first;
         i=first;
         j=last;

         while(i<j)
         {
             while(x[i]<=x[pivot]&&i<last)
                 i++;
             while(x[j]>x[pivot])
                 j--;
             if(i<j){
                 temp=x[i];
                  x[i]=x[j];
                  x[j]=temp;
             }
         }

         temp=x[pivot];
         x[pivot]=x[j];
         x[j]=temp;
         quickSort(x,first,j-1);
         quickSort(x,j+1,last);

    }
}
