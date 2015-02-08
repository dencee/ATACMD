//******************************************************************************
// Low-level tool for erasing ATA HDDs -- ATAErase.c
//
// $Date: 2014-10-31 09:13:14 -0700 (Fri, 31 Oct 2014) $
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
// Special thanks to:
//    Hale Landis (http://ata-atapi.com/)
//    Randy Maude (OFF THE GRID)
//
// Disclaimer:
// -----------
// *** USE THIS PROGRAM AT YOUR OWN RISK! ***
// Do not use this program on a drive you care about! This header acts as a
// disclaimer against responsibility for damage caused to your disk drive(s).
// Again, this program is designed for testing only!
//
// Purpose:
//---------
// The purpose of this program is to act as a diagnostic tool for ATA disk
// drives. This program differs from other HDD diagnostic tools in that it has
// the ability to issue commands at the command block register level. This
// means the program allows the user to send command codes defined by the ATA
// standard (read, writes, etc.) as well as those that are designated as
// reserved/unused/vendor by the ATA standard. Of course those undefined
// command codes have undefined behavior, so if you decide to send those
// commands, you do so at your own risk, as stated above.
//
// For a copy of all the publicly available commands, download one of the ATA
// specifications at: http://www.t13.org/
//
// How to use:
// -----------
// >>ATACMD.exe
//
// References:
// -----------
// http://www.t13.org/ --> projects --> working drafts --> ATA-6 or ATA-7
//
// Limitations of this software:
// -----------------------------
// * This program was not created for performance and therefore shouldn't be
// used as the basis for an IOPS or throughput test.
// * This library currently is not configured to operate on more than one HDD
// at a time.
// * There is currently no support for SCSI or enterprise devices. Those drives
// use a different protocol for sending/receiving commands. Visit:
// http://www.t10.org/ for more information.
//
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
//#include <dos.h>
#include <time.h>
//#include <math.h>
//#include <malloc.h>

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

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

#define DEFAULT_MASTER_PASSWORD                    ( "ataerase" )
#define TIME_DELAY_BETWEEN_CHECKS_IN_SECONDS       ( 10 )
#define ERASE_TIMEOUT_IN_SECONDS                   ( 12 * 60 * 60 )      // 12 hours

// -----------------------------------------------------------------------------
// Local function declarations
// -----------------------------------------------------------------------------

//int GetAndSendATACommand( void );

// -----------------------------------------------------------------------------
// Structs
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Global Variables
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Local Functions
// -----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Description: Initializes libraries and any global variables.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void InitializeParams()
{
   ukQuietMode = ON;      // Don't print ATALIB messages
   ukPrintOutput = PRINT_SCREEN;
   ATALIB_Initialize();
   DISPLAY_Initialize();
}

//------------------------------------------------------------------------------
// Description: Print success/fail message.
//
// Input:  pCommand     - user command line input
// Output: None
//------------------------------------------------------------------------------
void PrintSuccess( int commandSuccess )
{
   if ( commandSuccess == NO_ERROR ) {
      printf( "Success" );
   } else {
      printf( "Error executing command!!!" );
   }
}

//------------------------------------------------------------------------------
// Description: Entry point
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
int main( void )
{
   int exitProgram;

   exitProgram = OFF;
   InitializeParams();

   // --------------------------------------------------------------------------
   // Show available devices
   // --------------------------------------------------------------------------
   exitProgram = DISPLAY_ATAErase();

   system( "cls" );

   if ( exitProgram == OFF ) {
      int eachDevice, erasesInProgress;
      char wActiveDevices[ MAX_STORAGE_DEVICES ];

      erasesInProgress = 0;
      memset( wActiveDevices, 0, MAX_STORAGE_DEVICES );

      // Don't hang up program for erase to complete
      ATAIOREG_DisablePollForPIOCompletion();

      printf( "ATA Erase v1.0\n" );
      printf( "--------------------------------------------------------------------------------\n" );

      // -----------------------------------------------------------------------
      // Start erase(s)
      // -----------------------------------------------------------------------
      for ( eachDevice = 0; eachDevice < MAX_STORAGE_DEVICES; eachDevice++ )
      {
         struct StorageDevice_t* pDeviceInfo = GetDeviceInfo( eachDevice );

         if ( ( pDeviceInfo != NULL ) && ( pDeviceInfo->valid == VALID_DEVICE_ENTRY ) ) {
            // Valid device
            int secSupport, returnStatus;

            // Setup device I/O ports to issue commands
            SetActiveDevice( eachDevice );

            printf( "Checking security support for device %d [", ( eachDevice + 1 ) );
            PrintModelString();
            printf( "]..." );

            // Check security support
            CheckSecuritySupported(); secSupport = ukReturnValue1;

            // Start erase
            if ( secSupport == OFF ) {
               printf( "Not Supported! Aborting.\n" );
            } else {
               printf( "Supported\n" );
               printf( "Setting MASTER password %s...", DEFAULT_MASTER_PASSWORD );
               SecuritySetPassword( DEFAULT_MASTER_PASSWORD, MASTER_PASSWORD, SECURITY_LEVEL_HIGH ); returnStatus = ukReturnValue2;

               if ( returnStatus == ERROR ) {
                  printf( "ERROR! Aborting device %d.\n", ( eachDevice + 1 ) );
               } else {
                  char* pTimeStr;
                  int normalEraseTimeInMin;

                  printf( "Success\n" );
                  printf( "Get expected security erase time..." );
                  GetEstimatedSecureEraseTimesInMin(); normalEraseTimeInMin = ukReturnValue1;

                  if ( normalEraseTimeInMin == 0 ) {
                     printf( "Erase time not defined!\n" );
                  } else {
                     printf( "%d minutes (%d hrs and %d mins)\n", normalEraseTimeInMin, ( normalEraseTimeInMin / 60 ), ( normalEraseTimeInMin % 60 ) );
                  }

                  TOOLS_GetTime( &pTimeStr );

                  printf( "Executing security erase at %s...", pTimeStr );
                  fflush( stdout );
                  SecureErase( DEFAULT_MASTER_PASSWORD, MASTER_PASSWORD, NORMAL_SECURE_ERASE ); returnStatus = ukReturnValue1;

                  if ( returnStatus == ERROR ) {
                     printf( "ERROR! Aborting.\n" );
                  } else {
                     printf( "Issuing command successful\n" );
                     wActiveDevices[ eachDevice ] = 1;
                     erasesInProgress++;
                  }
               }
            }
         }
      }

      // -----------------------------------------------------------------------
      // Poll for erase completion
      // -----------------------------------------------------------------------
      if ( erasesInProgress > 0 ) {
         int eraseInProgress;
         time_t startTimeInSec, currentTimeInSec, tempTimeInSec;

         // Show running clock on program
         DISPLAY_InstallClock();

         printf( "\n" );

         time( &startTimeInSec );

         while ( erasesInProgress > 0 )
         {
            // Check timeout condition
            time( &currentTimeInSec );
            if ( ( currentTimeInSec - startTimeInSec ) > ERASE_TIMEOUT_IN_SECONDS ) {
               // Timeout occurred!
               printf( "Program timeout of %d hours occurred!!!\n", ( ERASE_TIMEOUT_IN_SECONDS / 3600 ) );

               // Report on each device in progress
               for ( eachDevice = 0; eachDevice < MAX_STORAGE_DEVICES; eachDevice++ ) {
                  if ( wActiveDevices[ eachDevice ] != 0 ) {
                     int returnStatus;
                     
                     printf( "Device %d is still processing erase command.\n", ( eachDevice + 1 ) );
                     printf( "Resetting device..." );
                     SoftwareReset(); returnStatus = ukReturnValue1;
                     if ( returnStatus == ERROR ) {
                        printf( "Success" );
                     } else {
                        printf( "ERROR!" );
                     }
                  }
               }

               printf( "Leaving device(s) in current state for debug.\n" );
               fflush( stdout );
               break;
            }

            // Delay between checks
            time( &tempTimeInSec );
            time( &currentTimeInSec );
            while ( ( currentTimeInSec - tempTimeInSec ) < TIME_DELAY_BETWEEN_CHECKS_IN_SECONDS ) {
               time( &currentTimeInSec );
            }

            // Check each device in progress
            for ( eachDevice = 0; eachDevice < MAX_STORAGE_DEVICES; eachDevice++ ) {
               if ( wActiveDevices[ eachDevice ] != 0 ) {
                  // Erase started on this device

                  // Select drive, DOES NOT send data to drive!
                  SetActiveDevice( eachDevice );

                  eraseInProgress = ATAIOREG_CheckForCommandInProgress();

                  if ( eraseInProgress == 0 ) {
                     // Erase completed!
                     char* pTimeStr;

                     TOOLS_GetTime( &pTimeStr );

                     printf( "Erase completed at %s on device %d [", pTimeStr, ( eachDevice + 1 ) );
                     PrintModelString();
                     printf( "]!\n" );

                     printf( "Completion status: %02X%02Xh...", reg_cmd_info.as2, reg_cmd_info.er2 );
                     if ( reg_cmd_info.er2 == 0 ) {
                        printf( "Success\n" );
                     } else {
                        printf( "ERROR!\n" );
                     }

                     fflush( stdout );
                     wActiveDevices[ eachDevice ] = 0;
                     erasesInProgress--;
                  }
               }
            }
         }

         // Remove clock interrupt handler
         DISPLAY_UninstallClock();
      }
   }

   DISPLAY_Pause();

   printf( "Thank you for using ATAErase -- (www.atacmd.com)\n" );

   ATALIB_CleanUp();

   return 0;
}
