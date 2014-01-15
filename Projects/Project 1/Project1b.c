#include <hidef.h>      /* common defines and macros */
#include <math.h>
#include "derivative.h"      /* derivative-specific definitions */
#include "7seg.h"
#include "atd.h"
#include "delays.h"


void main(void) {
  /* put your own code here */

    unsigned int digi, val;
    int res = 8;
    
   SYNR = 2;
   REFDV = 0;
   while (!(CRGFLG & 0x08));
   CLKSEL = 0x80;                 // Bus = 24Mhz

    sev_setup();

    atd0_powerOn();                 // Powers on ATD module 0
    atd0_setFFC(0);                 // set fast flag clearing on/off
    atd0_setLength(1);              // sets conversion sequence length of ATD module 0
    atd0_setFifo(0);                // turns FIFO mode on or off (1/0)
    atd0_setResolution(res);          // sets ATD module 0 resolution to 8/10bit
    atd0_setJustification('r');     // 'L' for left, 'R for right
    atd0_setScan(1);                // sets ATD Module 0 SCAN bit on/off (1/0)
    atd0_setMulti(0);               // sets ATD Module 0 MULT bit on/off (1/0)
    atd0_setStart(2);               // sets starting channel of ATD conversion sequence (call Last)

	EnableInterrupts;

    while(1)
    {
        digi = atd0_readChX(0);            // reads data registers of corresponding ATD Data Reg
        val = (int)((digi) / (pow(2, (res - 1))) * 9) + 48;       // breaks digital range into 8 regions ( +49 for numeric ASCII)                
        sev_write(val);
        delay_ms(10);
    }


  for(;;) {
    _FEED_COP(); /* feeds the dog */
  } /* loop forever */
  /* please make sure that you never leave main */
}


