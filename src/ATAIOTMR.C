//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOTMR.C
//
// by Hale Landis (www.ata-atapi.com)
//
// There is no copyright and there are no restrictions on the use
// of this ATA Low Level I/O Driver code.  It is distributed to
// help other programmers understand how the ATA device interface
// works and it is distributed without any warranty.  Use this
// code at your own risk.
//
// This code is based on the ATA-2, ATA-3 and ATA-4 standards and
// on interviews with various ATA controller and drive designers.
//
// This code has been run on many ATA (IDE) drives and
// MFM/RLL controllers.  This code may be a little
// more picky about the status it sees at various times.  A real
// BIOS probably would not check the status as carefully.
//
// Compile with one of the Borland C or C++ compilers.
//
// This C source file contains functions to access the BIOS
// Time of Day information and to set and check the command
// time out period.
//********************************************************************

#include <i86.h>
#include <dos.h>

#include "ataio.h"

//**************************************************************

// INT 1A increments the clock ticks by timer interrupt at 18.2065 times per second
// http://www.piclist.com/techref/int/1af/00.htm
#define BIOS_TIMER_INTERRUPTS_PER_SECOND     ( 18L )

//**************************************************************

long tmr_time_out = 20L;      // max command execution time in seconds

long tmr_cmd_start_time;      // command start time - see the
                              // tmr_set_timeout() and
                              // tmr_chk_timeout() functions.

long tmr_1s_count;            // number of I/O port reads required
                              //    for a 1s delay
long tmr_1ms_count;           // number of I/O port reads required
                              //    for a 1ms delay
long tmr_1us_count;           // number of I/O port reads required
                              //    for a 1us delay
long tmr_500ns_count;         // number of I/O port reads required
                              //    for a 500ns delay

//**************************************************************
//
// tmr_get_command_timeout() - function to get ATA command
//                             timeout.
//
//**************************************************************
long tmr_get_command_timeout( void )
{
   return ( tmr_time_out );
}

//**************************************************************
//
// tmr_set_command_timeout() - function to set ATA command
//                             timeout. Useful for commands that
//                             take a long time like security
//                             erase or foreground SMART OLI.
//
//**************************************************************
void tmr_set_command_timeout( long timeoutInSeconds )
{
   tmr_time_out = timeoutInSeconds;
}

//**************************************************************
//
// tmr_read_bios_timer() - function to read the BIOS timer
//
//**************************************************************

long tmr_read_bios_timer( void )

{
   long curTime;

   // Pointer to the low order word
   // of the BIOS time of day counter at
   // location 40:6C in the BIOS data area.
   #if defined(__WATCOMC__)
      #if defined(__386__)
         static volatile long *todPtr = ( long * ) 0x000046C0;
      #else
         static volatile long *todPtr = ( long * ) 0x0000046C;
      #endif
   #else
      static volatile long far * todPtr = MK_FP( 0x40, 0x6c );
   #endif // __WATCOMC__

   // loop so we get a valid value without
   // turning interrupts off and on again
   do
   {
      curTime = * todPtr;
   } while ( curTime != * todPtr );
   return curTime;
}

//**************************************************************
//
// tmr_set_timeout() - get the command start time
//
//**************************************************************

void tmr_set_timeout( void )
{
   // get the command start time
   tmr_cmd_start_time = tmr_read_bios_timer();
}

//**************************************************************
//
// tmr_chk_timeout() - check for command timeout.
//
// Gives non-zero return if command has timed out.
//
//**************************************************************

int tmr_chk_timeout( void )

{
   long curTime;

   // get current time
   curTime = tmr_read_bios_timer();

   // if we have passed midnight, restart
   if ( curTime < tmr_cmd_start_time )
   {
      tmr_cmd_start_time = curTime;
      return 0;
   }

   // timed out yet ?
   if ( curTime >= ( tmr_cmd_start_time + ( tmr_time_out * BIOS_TIMER_INTERRUPTS_PER_SECOND ) ) )
      return 1;      // yes

   // no timeout yet
   return 0;
}

//**************************************************************
//
// tmr_get_delay_counts - compute the number calls to
//    tmr_waste_time() required for 1s, 1ms, 1us and 500ns delays.
//
//**************************************************************

// our 'waste time' function (do some 32-bit multiply/divide)

static long tmr_waste_time( long p );

static long tmr_waste_time( long p )

{
   long lc = 100;

   return ( lc * lc ) / ( ( p * p ) + 1 );
}

// get the delay counts

void tmr_get_delay_counts( void )

{
   long count;
   long curTime, startTime, endTime;
   int loop;
   int retry;

   // do only once
   if ( tmr_1s_count )
      return;

   // outside loop to handle crossing midnight
   count = 0;
   retry = 1;
   while ( retry )
   {
      // wait until the timer ticks
      startTime = tmr_read_bios_timer();
      while ( 1 )
      {
         curTime = tmr_read_bios_timer();
         if ( curTime != startTime )
            break;
      }
      // waste time for 1 second
      endTime = curTime + 18L;
      while ( 1 )
      {
         for ( loop = 0; loop < 100; loop ++ )
            tmr_waste_time( 7 );
         count += 100 ;
         // check timer
         curTime = tmr_read_bios_timer();
         // pass midnight?
         if ( curTime < startTime )
         {
            count = 0;  // yes, zero count
            break;      // do again
         }
         // one second yet?
         if ( curTime >= endTime )
         {
            retry = 0;  // yes, we have a count
            break;      // done
         }
      }
   }

   // save the 1s count
   tmr_1s_count = count;
   // divide by 1000 and save 1ms count
   tmr_1ms_count = count = count / 1000L;
   // divide by 1000 and save 1us count
   tmr_1us_count = count = count / 1000L;
   // divide by 2 and save 500ns count
   tmr_500ns_count = count / 2;

   // make sure 1us count is at least 2
   if ( tmr_1us_count < 2L )
      tmr_1us_count = 2L;
   // make sure 500ns count is at least 1
   if ( tmr_500ns_count < 1L )
      tmr_500ns_count = 1L;
}

//**************************************************************
//
// tmr_delay_1ms - delay approximately 'count' milliseconds
//
//**************************************************************

void tmr_delay_1ms( long count )

{
   long loopcnt = tmr_1ms_count * count;

   while ( loopcnt > 0 )
   {
      tmr_waste_time( 7 );
      loopcnt -- ;
   }
}

//**************************************************************
//
// tmr_delay_1us - delay approximately 'count' microseconds
//
//**************************************************************

void tmr_delay_1us( long count )

{
   long loopcnt = tmr_1us_count * count;

   while ( loopcnt > 0 )
   {
      tmr_waste_time( 7 );
      loopcnt -- ;
   }
}

//**************************************************************
//
// tmr_ata_delay - delay approximately 500 nanoseconds
//
//**************************************************************

void tmr_delay_ata( void )

{
   long loopcnt = tmr_500ns_count;

   while ( loopcnt > 0 )
   {
      tmr_waste_time( 7 );
      loopcnt -- ;
   }
}

//**************************************************************
//
// tmr_atapi_delay() - delay for ~80ms so that poorly designed
//                     atapi device have time to updated their
//                     status.
//
//**************************************************************

void tmr_delay_atapi( int dev )

{

   if ( reg_config_info[dev] == REG_CONFIG_TYPE_ATA )
      return;
   if ( ! reg_atapi_delay_flag )
      return;
   trc_llt( 0, 0, TRC_LLT_DELAY1 );
   tmr_delay_1ms( 80L );
}

//**************************************************************
//
// tmr_xfer_delay() - random delay until the bios timer ticks
//                    (from 0 to 55ms).
//
//**************************************************************

void tmr_delay_xfer( void )

{
   long lw;

   trc_llt( 0, 0, TRC_LLT_DELAY2 );
   lw = tmr_read_bios_timer();
   while ( lw == tmr_read_bios_timer() )
      /* do nothing */ ;
}

// end ataiotmr.c
