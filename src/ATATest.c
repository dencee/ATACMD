//******************************************************************************
// Test all ATA commands -- ATATest.c
//
// $Date: 2014-10-31 09:13:14 -0700 (Fri, 31 Oct 2014) $
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
// Disclaimer:
// -----------
// *** USE THIS PROGRAM AT YOUR OWN RISK! ***
// Do not use this program on a drive you care about! This header acts as a
// disclaimer against responsibility for damage caused to your disk drive(s).
// Again, this program is designed for testing only!
//
// Purpose:
// --------
// This program attempts to issue every single ATA command to the drive and log
// the input and output command block registers. Some command codes that are
// either ATA reserved or vendor-allocated will sometimes produce bizzare
// results, hangs, etc. when issued. Since these commands are undocumented by
// the ATA standard, their behavior is unknown.  Therefore, I recommend to only
// use this program on backup/test drives, i.e. drives that don't have data you
// care about!
//
// A copy of the ATA standard outlining the input and output for each command
// can be found at: http://www.t13.org/
//
// References:
// -----------
// http://www.t13.org/ --> projects --> working drafts --> ATA-6 or ATA-7
//
// Limitations of this software:
// -----------------------------
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//******************************************************************************

// Header Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef   __ATAIO_H__
#include   "ataio.h"
#define   __ATAIO_H__
#endif // __ATAIO_H__

#ifndef   __TOOLS_H__
#include   "tools.h"
#define   __TOOLS_H__
#endif // __TOOLS_H__

#ifndef   __DISPLAY_H__
#include   "display.h"
#define   __DISPLAY_H__
#endif // __DISPLAY_H__

#ifndef   __ATALIB_H__
#include   "atalib.h"
#define   __ATALIB_H__
#endif // __ATALIB_H__

static unsigned char* wpDataTypeNames[] =
{
   "NONE",           // TRC_TYPE_NONE
   "Dma In",         // TRC_TYPE_ADMAI
   "DmaOut",         // TRC_TYPE_ADMAO
   "NON DA",         // TRC_TYPE_AND
   "PIO DI",         // TRC_TYPE_APDI
   "PIO DO",         // TRC_TYPE_APDO
   "SRST  ",         // TRC_TYPE_ASR
   "ADmaIn",         // TRC_TYPE_PDMAI
   "ADmaOt",         // TRC_TYPE_PDMAO
   "ANONDA",         // TRC_TYPE_PND
   "APIODI",         // TRC_TYPE_PPDI
   "APIODO",         // TRC_TYPE_PPDO
   "????"
};

void InitializeParams()
{
   ukQuietMode = ON;      // Don't print ATALIB messages
   ukPrintOutput = PRINT_SCREEN;
   ATALIB_Initialize();
   DISPLAY_Initialize();
}

void ClearTrace()
{
   // Clear the command history and low level traces
   trc_cht_dump0();     // zero the command history
   trc_llt_dump0();     // zero the low level trace
}

void ShowAll()
{
   int lc = 0;
   unsigned char * cp;

   printf( "ERROR !\n" );

   // display the command error information
   trc_err_dump1();           // start
   while ( 1 )
   {
      cp = trc_err_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
   }
   DISPLAY_Pause();

   // display the command history
   trc_cht_dump1();           // start
   while ( 1 )
   {
      cp = trc_cht_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
      lc ++ ;
      if ( ! ( lc & 0x000f ) )
         DISPLAY_Pause();
   }

   // display the low level trace
   trc_llt_dump1();           // start
   while ( 1 )
   {
      cp = trc_llt_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
      lc ++ ;
      if ( ! ( lc & 0x000f ) )
         DISPLAY_Pause();
   }

   if ( lc & 0x000f )
      DISPLAY_Pause();
}

void WriteCommandToFile( FILE* pReport )
{
   if ( pReport == NULL ) { printf( "ERROR: file pointer NULL" ); return; }

   // Write out all the input and output registers
   fprintf( pReport, "|%02Xh |", reg_cmd_info.cmd );
   fprintf( pReport, "%02Xh |", reg_cmd_info.fr1 );
   fprintf( pReport, "%02Xh[%02Xh]|", reg_cmd_info.sc1, reg_cmd_info.sc2 );
   fprintf( pReport, "%02Xh[%02Xh]|", reg_cmd_info.sn1, reg_cmd_info.sn2 );
   fprintf( pReport, "%02Xh[%02Xh]|", reg_cmd_info.cl1, reg_cmd_info.cl2 );
   fprintf( pReport, "%02Xh[%02Xh]|", reg_cmd_info.ch1, reg_cmd_info.ch2 );
   fprintf( pReport, "%02Xh[%02Xh]|", reg_cmd_info.dh1, reg_cmd_info.dh2 );
   fprintf( pReport, "%02Xh |", reg_cmd_info.dc1 );
   fprintf( pReport, "%02Xh[%02Xh]|", reg_cmd_info.lbaLow1, reg_cmd_info.lbaLow2 );
   fprintf( pReport, "%02Xh[%02Xh]|", reg_cmd_info.lbaHigh1, reg_cmd_info.lbaHigh2 );
   fprintf( pReport, "%02Xh |", reg_cmd_info.as2 );
   fprintf( pReport, "%02Xh |", reg_cmd_info.er2 );
   fprintf( pReport, "%s|\n", wpDataTypeNames[ reg_cmd_info.ct ]);

   return;
}

int main()
{
   int ataCommand, exitProgram;
   long int ataRegs[ NUM_INPUT_REGS ];
   FILE *pReport;
   time_t currentTime;

   pReport = fopen( "AtaRpt.log", "w" );
   if ( pReport == NULL ) { printf( "ERROR: file pointer NULL" ); return 1; }

   InitializeParams();

   exitProgram = DisplayConnectedATAStorageDevices( SCAN_FOR_DEVICES );

   if ( exitProgram == TRUE )
   {
      return 0;
   }

   // Don't print ATALIB errors
   ukQuietMode = ON;

   time( &currentTime );
   fprintf( pReport, "ATA Commands report\nDate: %s\n", ctime( &currentTime ) );
   fprintf( pReport, "|CMD |FEAT|SECC    |SECN    |CYLL    |CYLH    |DEVH    |DEVC|LBAL    |LBAH    |STAT|ERR |DTYPE |\n" );
   fprintf( pReport, "+----+----+--------+--------+--------+--------+--------+----+--------+--------+----+----+------|\n" );
   printf( "Test started on: %s", ctime( &currentTime ) );

   for ( ataCommand = 0x00; ataCommand <= CMD_LAST; ataCommand++ )
   {
      memset( ataRegs, 0, sizeof( ataRegs ) );

      // Set default register settings
      ataRegs[ CMD_REG ]  = ataCommand;
      ataRegs[ FEAT_REG ] = 0;
      ataRegs[ SECC_REG ] = 0;
      ataRegs[ SECN_REG ] = 0;
      ataRegs[ CYLL_REG ] = 0;
      ataRegs[ CYLH_REG ] = 0;
      ataRegs[ DEVH_REG ] = 0;      // Not used
      ataRegs[ DEVC_REG ] = 0;      // Not used
      ataRegs[ LBA_LOW ]  = 0;
      ataRegs[ LBA_HIGH ] = 0;

      printf( "testing command %3d[%02Xh]...", ataCommand, ataCommand );
      ClearTrace();

      // Software reset to return drive to a known good state
      SoftwareReset();

      // Command specific register modifications
      switch( ataCommand )
      {
         case CMD_DEVICE_CONFIGURATION:
            // Only issue DCID for now; other feat codes later
            ataRegs[ FEAT_REG ] = DEV_CONFIG_IDENTIFY;
            break;

         case CMD_READ_BUFFER:
            // No special handling
            break;

         case CMD_READ_LOG_EXT:
            ataRegs[ SECC_REG ] = 1;
            ataRegs[ LBA_LOW ]  = 0x80;   // HV specific
            break;

         case CMD_READ_DMA:
         case CMD_READ_DMA_EXT:
         case CMD_READ_SECTORS:
         case CMD_READ_SECTORS_EXT:
         case CMD_READ_SECTORS_WITHOUT_RETRY:
         case CMD_READ_VERIFY_SECTORS:
         case CMD_READ_VERIFY_SECTORS_EXT:
         case CMD_READ_VERIFY_SECTORS_WITHOUT_RETRY:
            ataRegs[ SECC_REG ] = 1;
            break;

         case CMD_SET_FEATURES:
            ataRegs[ FEAT_REG ] = SET_FEAT_ENABLE_READ_CACHE;
            break;

         case CMD_SLEEP1:
         case CMD_SLEEP2:
            // No special handling
            break;

         case CMD_STANDBY1:
         case CMD_STANDBY2:
            // Timer disabled
            ataRegs[ SECC_REG ] = 0;
            break;

         case CMD_SMART:
            // Only issue read attrib for now; other feat codes later
            ataRegs[ FEAT_REG ] = SMART_READ_DATA;
            ataRegs[ CYLL_REG ] = 0x4F;
            ataRegs[ CYLH_REG ] = 0xC2;
            break;

         case CMD_WRITE_BUFFER:
            // No special handling
            break;

         case CMD_WRITE_LOG_EXT:
            ataRegs[ SECC_REG ] = 1;
            ataRegs[ LBA_LOW ]  = 0x80;   // HV specific
            break;

         case CMD_WRITE_DMA:
         case CMD_WRITE_DMA_EXT:
         case CMD_WRITE_DMA_FUA_EXT:
         case CMD_WRITE_SECTORS:
         case CMD_WRITE_SECTORS_EXT:
         case CMD_WRITE_SECTORS_WITHOUT_RETRY:
         case CMD_WRITE_VERIFY:
            ataRegs[ SECC_REG ] = 1;
            break;

         case CMD_SET_MULTIPLE_MODE:
            ataRegs[ SECC_REG ] = 16;
            break;

         // Skipped commands (to do later)
         case 0x88: // vendor specific
         case 0x89: // vendor specific
         case 0x8A: // vendor specific
         case 0x8B: // vendor specific
         case CMD_WRITE_SAME:
         case CMD_IDENTIFY_DEVICE_DMA:
         case CMD_READ_FPDMA_QUEUED:
         case CMD_WRITE_FPDMA_QUEUED:
         case CMD_WRITE_LONG:
         case CMD_WRITE_LONG_WITHOUT_RETRY:
         case CMD_READ_LONG:
         case CMD_READ_LONG_WITHOUT_RETRY:
         case CMD_READ_STREAM_DMA_EXT:
         case CMD_READ_STREAM_EXT:
         case CMD_WRITE_STREAM_DMA_EXT:
         case CMD_WRITE_STREAM_EXT:
         case CMD_READ_DMA_WITHOUT_RETRIES:
         case CMD_WRITE_DMA_WITHOUT_RETRIES:
         case CMD_EXECUTE_DEVICE_DIAGNOSTIC:
         case CMD_SEEK:
         case CMD_READ_MULTIPLE:
         case CMD_READ_MULTIPLE_EXT:
         case CMD_WRITE_MULTIPLE:
         case CMD_WRITE_MULTIPLE_EXT:
         case CMD_WRITE_MULTIPLE_FUA_EXT:
         case CMD_CONFIGURE_STREAM:
         case CMD_DOWNLOAD_MICROCODE:
         case CMD_SET_MAX_ADDRESS:
         case CMD_SET_MAX_ADDRESS_EXT:
         case CMD_SECURITY_DISABLE_PWD:
         case CMD_SECURITY_ERASE_PREPARE:
         case CMD_SECURITY_ERASE_UNIT:
         case CMD_SECURITY_FREEZE_LOCK:
         case CMD_SECURITY_SET_PWD:
         case CMD_SECURITY_UNLOCK:
         case CMD_FORMAT_TRACK:
            printf( "Skipping command!\n" );
            fprintf( pReport, "Skipping command!\n" );
            continue;
            break;

         default:
            break;
      }

      // Send ATA command
      SendATACommand( ataRegs );

      // Write the input and output registers to report
      WriteCommandToFile( pReport );
      printf( "done\n" );

      // Stop here and exit if timeout occurred
      if ( reg_cmd_info.to != 0 )
      {
         printf( "Command timeout occurred!\n" );
         fprintf( pReport, "   COMMAND %02Xh timed out\n", ataCommand );
         SoftwareReset();

//         ShowAll();
//         break;
      }

      // Put drive back in original state
      switch( ataCommand )
      {
         case CMD_DEVICE_CONFIGURATION:
            // Todo later -DMC
            break;

         case CMD_EXECUTE_DEVICE_DIAGNOSTIC:
            // Todo later -DMC
            break;

         case CMD_SET_FEATURES:
            // Todo later -DMC
            break;

         case CMD_SLEEP1:
         case CMD_SLEEP2:
            // Put drive in standby
            SoftwareReset();

            // Issue recal to bring the drive back into active/idle
            SendNonDataCommand( CMD_RECALIBRATE, 0, 0, 0, 0, 0 );
            break;

         case CMD_STANDBY_IMMEDIATE1:
         case CMD_STANDBY_IMMEDIATE2:
            // Issue recal to bring the drive back into active/idle
            SendNonDataCommand( CMD_RECALIBRATE, 0, 0, 0, 0, 0 );
            break;

         case CMD_SMART:
            break;

//         case CMD_SECURITY_DISABLE_PWD:
//         case CMD_SECURITY_ERASE_PREPARE:
//         case CMD_SECURITY_ERASE_UNIT:
//         case CMD_SECURITY_FREEZE_LOCK:
//         case CMD_SECURITY_SET_PWD:
//         case CMD_SECURITY_UNLOCK:
//            break;

         default:
            break;
      }
   }

   // Final reset to return drive to active state
   SoftwareReset();

   time( &currentTime );
   fprintf( pReport, "Completed on %s\n", ctime( &currentTime ) );

   fclose( pReport );

   return 0;
}
