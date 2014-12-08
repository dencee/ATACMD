//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOINT.C
//
// by Hale Landis (www.ata-atapi.com)
//
// There is no copyright and there are no restrictions on the use
// of this ATA Low Level I/O Driver code.  It is distributed to
// help other programmers understand how the ATA device interface
// works and it is distributed without any warranty.  Use this
// code at your own risk.
//
// This code is based on the ATA-x, and ATA/ATAPI-x standards and
// on interviews with various ATA controller and drive designers.
//
// This code has been run on many ATA (IDE) drives and
// MFM/RLL controllers.  This code may be a little
// more picky about the status it sees at various times.  A real
// BIOS probably would not check the status as carefully.
//
// Compile with one of the Borland C or C++ compilers.
//
// This C source contains the low level interrupt set up and
// interrupt handler functions.
//********************************************************************

#include <dos.h>

#include "ataio.h"

//*************************************************************
//
// Global interrupt mode data
//
//*************************************************************

int int_use_intr_flag = 0;    // use INT mode if != 0

volatile int int_intr_cntr;   // interrupt counter, incremented
                              // each time there is an interrupt

volatile int int_intr_flag;   // interrupt flag, incremented
                              // for each device interrupt

unsigned int int_bmide_addr;  // BMIDE status reg i/o address

volatile unsigned char int_bm_status;     // BMIDE status

unsigned int int_ata_addr;    // ATA status reg I/O address

volatile unsigned char int_ata_status;    // ATA status

//*************************************************************
//
// Local data
//
//*************************************************************

// interrupt handler function info...

// Save area for the system's INT vector
#ifdef    __WATCOMC__
   static void interrupt ( *int_org_int_vect ) ();
#else
   static void interrupt ( far *int_org_int_vect ) ();
#endif


static void far interrupt int_handler( void );        // our INT handler

// our interrupt handler's data...

static int int_got_it_now = 0;      // != 0, our interrupt handler
                                    //is installed now

static int int_irq_number = 0;      // IRQ number in use, 1 to 15
static int int_int_vector = 0;      // INT vector in use,
                                    // INT 8h-15h and INT 70H-77H.
static int int_shared = 0;          // shared flag

// system interrupt controller data...

#define PIC0_CTRL 0x20           // PIC0 i/o address
#define PIC0_MASK 0x21           // PIC0 i/o address
#define PIC1_CTRL 0xA0           // PIC1 i/o address
#define PIC1_MASK 0xA1           // PIC1 i/o address

#define PIC0_ENABLE_PIC1 0xFB    // mask to enable PIC1 in PIC0

static int pic_enable_irq[8] =       // mask to enable
   { 0xFE, 0xFD, 0xFB, 0xF7,     //   IRQ 0-7 in PIC 0 or
     0xEF, 0xDF, 0xBF, 0x7F  };  //   IRQ 8-15 in PIC 1

#define PIC_EOI      0x20        // end of interrupt

//*************************************************************
//*   In-line assembly
//*************************************************************

extern void PopAll(void);
#pragma aux PopAll =     \
   "pop   bp "           \
   "pop   edi"           \
   "pop   esi"           \
   "pop   ds "           \
   "pop   es "           \
   "pop   edx"           \
   "pop   ecx"           \
   "pop   ebx"           \
   "pop   eax"           \
   modify [bp di si ds es dx cx bx ax];

extern void ChainInt(void);
#pragma aux ChainInt =                            \
   "push  ax                                    " \
   "push  ax                                    " \
   "push  ax                                    " \
   "push  bp                                    " \
   "push  ds                                    " \
   "push  ds                                    " \
   "mov   bp,seg int_org_int_vect               " \
   "mov   ds,bp                                 " \
   "mov   bp,sp                                 " \
   "mov   ax,ds:[int_org_int_vect]              " \
   "mov   [bp+6], ax                            " \
   "mov   ax,ds:[int_org_int_vect+2]            " \
   "mov   [bp+8], ax                            " \
   "pop   ds                                    " \
   "pop   bp                                    " \
   "pop   ax                                    " \
   "retf                                        " \
   modify [ax bp ds]                            ;

//*************************************************************
//
// Enable interrupt mode -- get the IRQ number we are using.
//
// The function MUST be called before interrupt mode can
// be used!
//
// If this function is called then the int_disable_irq()
// function MUST be called before exiting to DOS.
//
//*************************************************************

int int_enable_irq( int shared, int irqNum,
                    unsigned int bmAddr, unsigned int ataAddr )

//  shared: 0 is not shared, != 0 is shared
//  irqNum: 1 to 15
//  bmAddr: i/o address for BMIDE Status register
//  ataAddr: i/o address for the ATA Status register

{

   // error if interrupts enabled now
   // error if invalid irq number
   // error if bmAddr is < 100H
   // error if shared and bmAddr is 0
   // error if ataAddr is < 100H

   if ( int_use_intr_flag )
      return 1;
   if ( ( irqNum < 1 ) || ( irqNum > 15 ) )
      return 2;
   if ( irqNum == 2 )
      return 2;
   if ( bmAddr && ( bmAddr < 0x0100 ) )
      return 3;
   if ( shared && ( ! bmAddr ) )
      return 4;
   if ( ataAddr < 0x0100 )
      return 5;

   // save the input parameters

   int_shared     = shared;
   int_irq_number = irqNum;
   int_bmide_addr  = bmAddr;
   int_ata_addr   = ataAddr;

   // convert IRQ number to INT number
   // and
   // enable the interrupt in the PIC
   // See: http://wiki.osdev.org/8259_PIC

   if ( irqNum < 8 )
   {
      int_int_vector = irqNum + 8;              // 8 is the vector offset for master PIC
      // In PIC0 change the IRQ 0-7 enable bit to 0
      _OUTP( PIC0_MASK, ( _INP( PIC0_MASK )
                         & pic_enable_irq[ irqNum ] ) );
   }
   else
   {
      int_int_vector = 0x70 + ( irqNum - 8 );   // 70h is vector offset for slave PIC (its IRQs are 0-7)
      // In PIC0 change the PIC1 enable bit to 0 (enable IRQ 2) so the interrupt can cascade down
      // In PIC1 change the IRQ enable bit to 0
      _OUTP( PIC0_MASK, ( _INP( PIC0_MASK ) & PIC0_ENABLE_PIC1 ) );
      _OUTP( PIC1_MASK, ( _INP( PIC1_MASK )
                         & pic_enable_irq[ irqNum - 8 ] ) );
   }

   // interrupts use is now enabled

   int_use_intr_flag = 1;
   int_got_it_now = 0;

   // Done.

   return 0;
}

//*************************************************************
//
// Disable interrupt mode.
//
// If the int_enable_irq() function has been called,
// this function MUST be called before exiting to DOS.
//
//*************************************************************

void int_disable_irq( void )

{

   // if our interrupt handler is installed now,
   // restore the system's (the original) interrupt handler.

   if ( int_got_it_now )
   {

      // Disable interrupts.
      // Restore the system's interrupt vector.
      // Enable interrupts.

      _DISABLE();
      _SETVECT( int_int_vector, int_org_int_vect );
      _ENABLE();
   }

   // Reset all the interrupt data.

   int_use_intr_flag = 0;
   int_got_it_now = 0;
}

//*************************************************************
//
// Install our interrupt handler.
//
// Interrupt mode MUST be setup by calling int_enable_irq()
// before calling this function.
//
//*************************************************************

void int_save_int_vect( void )

{

   // Do nothing if interrupt use not enabled now.
   // Do nothing if our interrupt handler is installed now.

   if ( ! int_use_intr_flag )
      return;
   if ( int_got_it_now )
      return;

   // Disable interrupts.
   // Save the interrupt vector.

   _DISABLE();

   int_org_int_vect = _GETVECT( int_int_vector );

   // install our interrupt handler

   _SETVECT( int_int_vector, int_handler );

   // Our interrupt handler is installed now.

   int_got_it_now = 1;

   // Reset the interrupt flag.

   int_intr_cntr = 0;
   int_intr_flag = 0;

   // Enable interrupts.

   _ENABLE();
}

//*************************************************************
//
// Restore the interrupt vector.
//
// Interrupt mode MUST be setup by calling int_enable_irq()
// before calling this function.
//
//*************************************************************

void int_restore_int_vect( void )

{

   // Do nothing if interrupt useis disabled now.
   // Do nothing if our interrupt handler is not installed.

   if ( ! int_use_intr_flag )
      return;
   if ( ! int_got_it_now )
      return;

   // Disable interrupts.
   // Restore the interrupt vector.
   // Enable interrupts.

   _DISABLE();
   _SETVECT( int_int_vector, int_org_int_vect );
   _ENABLE();

   // Our interrupt handler is not installed now.

   int_got_it_now = 0;
}

//*************************************************************
//
// ATADRVR's Interrupt Handler.
//
//*************************************************************

static void far interrupt int_handler( void )

{

   // increment the interrupt counter

   int_intr_cntr ++ ;

   // if BMIDE present read the BMIDE status
   // else just read the device status.

   if ( int_bmide_addr )
   {
      // PCI ATA controller...
      // ... read BMIDE status
      int_bm_status = _INP( int_bmide_addr );
      //... check if Interrupt bit = 1
      if ( int_bm_status & BM_SR_MASK_INT )
      {
         // ... Interrupt=1...
         // ... increment interrupt flag,
         // ... read ATA status,
         // ... reset Interrupt bit.
         int_intr_flag ++ ;
         int_ata_status = _INP( int_ata_addr );
         _OUTP( int_bmide_addr, BM_SR_MASK_INT );
      }
   }
   else
   {
      // legacy ATA controller...
      // ... increment interrupt flag,
      // ... read ATA status.
      int_intr_flag ++ ;
      int_ata_status = _INP( int_ata_addr );
   }

   // if interrupt is shared, jump to the original handler...

   if ( int_shared )
   {

      // pop all regs

      #ifdef   __WATCOMC__

         PopAll();

      #else

         asm pop   bp
         asm pop   edi
         asm pop   esi
         asm pop   ds
         asm pop   es
         asm pop   edx
         asm pop   ecx
         asm pop   ebx
         asm pop   eax

      #endif

      // put cs:ip of next interrupt handler on stack

      #ifdef   __WATCOMC__

         ChainInt();

      #else

         asm push  ax            // will be bp+8
         asm push  ax            // will be bp+6
         asm push  ax
         asm push  bp
         asm push  ds
         asm mov   bp,DGROUP
         asm mov   ds,bp
         asm mov   bp, sp
         asm mov   ax, word ptr DGROUP:int_org_int_vect
         asm mov   [bp+6], ax
         asm mov   ax, word ptr DGROUP:int_org_int_vect+2
         asm mov   [bp+8], ax
         asm pop   ds
         asm pop   bp
         asm pop   ax

         // jump to the original interrupt handler

         asm retf

      #endif

      // never get here

   }

   // interrupt is not shared...
   // send End-of-Interrupt (EOI) to the interrupt controller(s).

   _OUTP( PIC0_CTRL, PIC_EOI );
   if ( int_irq_number >= 8 )
      _OUTP( PIC1_CTRL, PIC_EOI );

   // IRET here (return from interrupt)
}

// end ataioint.c
