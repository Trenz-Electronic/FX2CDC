/*
Copyright (C) 2012 Trenz Electronic

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.
*/

#pragma NOIV               // Do not generate interrupt vectors
#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"			// SYNCDELAY macro

extern BOOL   GotSUD;         // Received setup data flag
extern BOOL   Sleep;
extern BOOL   Rwuen;
extern BOOL   Selfpwr;

BYTE Configuration;      // Current configuration
BYTE AlternateSetting;   // Alternate settings
void display_product(void);
void TD_Poll(void);
BOOL DR_SetConfiguration();

void  Serial0Init () // dummy function for fw.c compatibility
{
}

void TD_Init(void)             // Called once at startup
{
    CPUCS = ((CPUCS & ~bmCLKSPD) | bmCLKSPD1); // set the CPU clock to 48MHz
    IFCONFIG = 0xE3; SYNCDELAY;     // Enable slave FIFO Interface @48MHz
    REVCTL = 0x03; SYNCDELAY;
    // Endpoints configuration
 	EP1OUTCFG = 0xA0; SYNCDELAY;    // Configure EP1OUT as BULK EP
    EP1INCFG = 0xB0; SYNCDELAY;     // Configure EP1IN as BULK IN EP
	EP2CFG = 0xA2; SYNCDELAY;       // EP2 is DIR=OUT, TYPE=BULK, SIZE=512x2
  	EP8CFG = 0xE0; SYNCDELAY;       // Configure EP8 as BULK IN EP 
	// FIFO Reset
    FIFORESET = 0x80;  SYNCDELAY;   // "NAK-All" requests from host.
    FIFORESET = 0x82;  SYNCDELAY;   // Reset EP2
    FIFORESET = 0x88;  SYNCDELAY;   // Reset EP8
    FIFORESET = 0x00;  SYNCDELAY;   // Resume normal operation.
    // Arm EP2 buffers
    OUTPKTEND = 0x82; SYNCDELAY;    // Arm First buffer
    OUTPKTEND = 0x82; SYNCDELAY;    // Arm Second buffer
    // Configure flags
    EP8FIFOPFH = 0x01; SYNCDELAY;   // programmable flag (FlagC)
    EP8FIFOPFL = 0x00; SYNCDELAY;   // Set slag to half of the buffer
    PINFLAGSAB = 0xF8; SYNCDELAY;	// FlagB = EP8FF,    FlagA = EP2EF
    PINFLAGSCD = 0x07; SYNCDELAY;	// FlagD = not used, FlagC = EP8PF
    // Fifo Config
    EP2FIFOCFG = 0x00; SYNCDELAY;   // Configure EP2 FIFO in 8-bit Manual mode
    EP4FIFOCFG = 0x00; SYNCDELAY;   // Disable Wordwide bit
    EP6FIFOCFG = 0x00; SYNCDELAY;   // Disable Wordwide bit
    EP8FIFOCFG = 0x0C; SYNCDELAY;   // Configure EP8 FIFO in 8-bit AutoIn mode
    // Other settings
    PORTACFG = 0x00; SYNCDELAY;     // used PA7/FLAGD as a port pin, not as a FIFO flag
    FIFOPINPOLAR = 0x3F; SYNCDELAY;  // set all slave FIFO interface pins as active high
    EP8AUTOINLENH = 0x02; SYNCDELAY; // EZ-USB automatically commits data in 512-byte chunks
    EP8AUTOINLENL = 0x00; SYNCDELAY;
    OEA=0x80;                        // Configure FlagD pin as OUT
}

void TD_Poll(void){             // Called repeatedly while the device is idle
  // Serial State Notification that has to be sent periodically to the host
  if (!(EP1INCS & 0x02)){      // check if EP1IN is available
    EP1INBUF[0] = 0x0A;       // if it is available, then fill the first 10 bytes of the buffer with 
	EP1INBUF[1] = 0x20;       // appropriate data. 
	EP1INBUF[2] = 0x00;
	EP1INBUF[3] = 0x00;
	EP1INBUF[4] = 0x00;
	EP1INBUF[5] = 0x00;
	EP1INBUF[6] = 0x00;
	EP1INBUF[7] = 0x02;
    EP1INBUF[8] = 0x00;
	EP1INBUF[9] = 0x00;
  	EP1INBC = 10;            // manually commit once the buffer is filled
  }

  if (!(EP1OUTCS & 0x02)){ // Check if EP1OUT is available
	int bcl,i;
	bcl=EP1OUTBC;                           // Count of data to reload
    IOA ^= 0x80;                            // Drive FLAGD pin
    if( EP24FIFOFLGS & 0x02 ) {             // If we have room in FIFO
        FIFORESET = 0x02; SYNCDELAY;        // Reset to dis-arm EP2
        FIFORESET = 0x00; SYNCDELAY;        // Return to normal
	    for(i=0;i<bcl;i++)                  // All received data
            EP2FIFOBUF[i] = EP1OUTBUF[i];   // reload from EP1 to EP2
        EP2BCH = bcl >> 8; SYNCDELAY;       // High part
        EP2BCL = bcl & 0xff; SYNCDELAY;     // Low part and commit
    }
  	EP1OUTBC = 0x04;                        // Commit packet
  }
}

BOOL TD_Suspend(void)          // Called before the device goes into suspend mode
{
   return(TRUE);
}
BOOL TD_Resume(void)          // Called after the device resumes
{
   return(TRUE);
}
//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------
BOOL DR_GetDescriptor(void)
{
   return(TRUE);
}
BOOL DR_SetConfiguration(void)   // Called when a Set Configuration command is received
{  
   Configuration = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}
BOOL DR_GetConfiguration(void)   // Called when a Get Configuration command is received
{
   EP0BUF[0] = Configuration;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}
BOOL DR_SetInterface(void)       // Called when a Set Interface command is received
{
   AlternateSetting = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}
BOOL DR_GetInterface(void)       // Called when a Set Interface command is received
{
   EP0BUF[0] = AlternateSetting;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}
BOOL DR_GetStatus(void)
{
   return(TRUE);
}
BOOL DR_ClearFeature(void)
{
   return(TRUE);
}
BOOL DR_SetFeature(void)
{
   return(TRUE);
}
BOOL DR_VendorCmnd(void)
{
   return(TRUE);
}
//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------
void ISR_Sudav(void) interrupt 0
{
   GotSUD = TRUE;            // Set flag
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUDAV;         // Clear SUDAV IRQ
}

void ISR_Sutok(void) interrupt 0
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUTOK;         // Clear SUTOK IRQ
}
void ISR_Sof(void) interrupt 0
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSOF;            // Clear SOF IRQ
}
void ISR_Ures(void) interrupt 0
{
   if (EZUSB_HIGHSPEED())
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmURES;         // Clear URES IRQ
}
void ISR_Susp(void) interrupt 0
{
    Sleep = TRUE;
    EZUSB_IRQ_CLEAR();
    USBIRQ = bmSUSP;
}

void ISR_Highspeed(void) interrupt 0
{
   if (EZUSB_HIGHSPEED())
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack(void) interrupt 0
{
}
void ISR_Stub(void) interrupt 0
{
}
void ISR_Ep0in(void) interrupt 0
{
}
void ISR_Ep0out(void) interrupt 0
{
}
void ISR_Ep1in(void) interrupt 0
{
}
void ISR_Ep1out(void) interrupt 0
{
}
void ISR_Ep2inout(void) interrupt 0
{
}
void ISR_Ep4inout(void) interrupt 0
{
}
void ISR_Ep6inout(void) interrupt 0
{
}
void ISR_Ep8inout(void) interrupt 0
{
}
void ISR_Ibn(void) interrupt 0
{
}
void ISR_Ep0pingnak(void) interrupt 0
{
}
void ISR_Ep1pingnak(void) interrupt 0
{
}
void ISR_Ep2pingnak(void) interrupt 0
{
}
void ISR_Ep4pingnak(void) interrupt 0
{
}
void ISR_Ep6pingnak(void) interrupt 0
{
}
void ISR_Ep8pingnak(void) interrupt 0
{
}
void ISR_Errorlimit(void) interrupt 0
{
}
void ISR_Ep2piderror(void) interrupt 0
{
}
void ISR_Ep4piderror(void) interrupt 0
{
}
void ISR_Ep6piderror(void) interrupt 0
{
}
void ISR_Ep8piderror(void) interrupt 0
{
}
void ISR_Ep2pflag(void) interrupt 0
{
}
void ISR_Ep4pflag(void) interrupt 0
{
}
void ISR_Ep6pflag(void) interrupt 0
{
}
void ISR_Ep8pflag(void) interrupt 0
{
}
void ISR_Ep2eflag(void) interrupt 0
{
}
void ISR_Ep4eflag(void) interrupt 0
{
}
void ISR_Ep6eflag(void) interrupt 0
{
}
void ISR_Ep8eflag(void) interrupt 0
{
}
void ISR_Ep2fflag(void) interrupt 0
{
}
void ISR_Ep4fflag(void) interrupt 0
{
}
void ISR_Ep6fflag(void) interrupt 0
{
}
void ISR_Ep8fflag(void) interrupt 0
{
}
void ISR_GpifComplete(void) interrupt 0
{
}
void ISR_GpifWaveform(void) interrupt 0
{
}
