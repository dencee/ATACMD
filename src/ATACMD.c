//******************************************************************************
// Low-level diagnostic tool for ATA HDDs -- ATACMD.c
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
//#include <time.h>
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

#define PRINT_64_BYTES                                   ( 64 )
#define PRINT_258_BYTES                                  ( 258 )  // bytes 0-257

// -----------------------------------------------------------------------------
// Local function declarations
// -----------------------------------------------------------------------------

int GetAndSendATACommand( void );

int ClearBuffer( const char* pCommand );
int FillBuffer( const char* pCommand );
int RPIO( const char* pCommand );
int WPIO( const char* pCommand );
int Id( const char* pCommand );
int Reset( const char* pCommand );
int HpaSetLBA( const char* pCommand );
int DcoSetLBA( const char* pCommand );
int ATASecuritySupport( const char* pCommand );
int ATASecurityUnsupport( const char* pCommand );
int RemoveHPAAndDCO( const char* pCommand );
int SecuritySetMasterPassword( const char* pCommand );
int SecuritySetUserPassword( const char* pCommand );
int SecurityUnlock( const char* pCommand );
int SecurityDisable( const char* pCommand );
int SecurityErase( const char* pCommand );
int ViewBuffer( const char* pCommand );
int DisplayATAInfo( const char* pCommand );
int ScanDrives( const char* pCommand );
int SetDebugModeOn( const char* pCommand );
int SetDebugModeOff( const char* pCommand );
int ATACommand( const char* pCommand );
int PrintDUTInfo( const char* pCommand );
int RDMA( const char* pCommand );
int TraceDisplay( const char* pCommand );
int TraceClear( const char* pCommand );
int SmartAttributes( const char* pCommand );

// -----------------------------------------------------------------------------
// Structs
// -----------------------------------------------------------------------------

struct tEachCommand
{
   const char* pName;
   int (* pFunctionPtr)(const char *);
};

// -----------------------------------------------------------------------------
// Global Variables
// -----------------------------------------------------------------------------

char wcCommand[ NUMBER_OF_CHARACTERS_IN_DOS_LINE ] = { 0 };

// ****************************************************************
//  Add new macro commands to wtAtacmdCommands command array here!
// ****************************************************************
struct tEachCommand wtAtacmdCommands[] = {
   [0].pName =  "read",    [0].pFunctionPtr =  &RPIO,
   [1].pName =  "write",   [1].pFunctionPtr =  &WPIO,
   [2].pName =  "id",      [2].pFunctionPtr =  &Id,
   [3].pName =  "reset",   [3].pFunctionPtr =  &Reset,
   [4].pName =  "hpa",     [4].pFunctionPtr =  &HpaSetLBA,
   [5].pName =  "dco",     [5].pFunctionPtr =  &DcoSetLBA,
   [6].pName =  "secon",   [6].pFunctionPtr =  &ATASecuritySupport,
   [7].pName =  "secoff",  [7].pFunctionPtr =  &ATASecurityUnsupport,
   [8].pName =  "remove",  [8].pFunctionPtr =  &RemoveHPAAndDCO,
   [9].pName =  "setmpw",  [9].pFunctionPtr =  &SecuritySetMasterPassword,
   [10].pName = "setupw",  [10].pFunctionPtr = &SecuritySetUserPassword,
   [11].pName = "unlock",  [11].pFunctionPtr = &SecurityUnlock,
   [12].pName = "pwdis",   [12].pFunctionPtr = &SecurityDisable,
   [13].pName = "viewbuf", [13].pFunctionPtr = &ViewBuffer,
   [14].pName = "X",       [14].pFunctionPtr = &DisplayATAInfo,
   [15].pName = "debon",   [15].pFunctionPtr = &SetDebugModeOn,
   [16].pName = "deboff",  [16].pFunctionPtr = &SetDebugModeOff,
   [17].pName = "atacmd",  [17].pFunctionPtr = &ATACommand,
   [18].pName = "rescan",  [18].pFunctionPtr = &ScanDrives,
   [19].pName = "dut",     [19].pFunctionPtr = &PrintDUTInfo,
   [20].pName = "clrbuf",  [20].pFunctionPtr = &ClearBuffer,
   [21].pName = "rdma",    [21].pFunctionPtr = &RDMA,
   [22].pName = "trcclr",  [22].pFunctionPtr = &TraceClear,
   [23].pName = "trc",     [23].pFunctionPtr = &TraceDisplay,
   [24].pName = "erase",   [24].pFunctionPtr = &SecurityErase,
   [25].pName = "smart",   [25].pFunctionPtr = &SmartAttributes,
   [26].pName = "fillbuf", [26].pFunctionPtr = &FillBuffer,
//   [27].pName = "", [27].pFunctionPtr = &,
//   [28].pName = "", [28].pFunctionPtr = &,
};

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
   Display_Initialize();
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
// Description: Clear global I/O buffer.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR
//------------------------------------------------------------------------------
int ClearBuffer( const char* pCommand )
{
    memset( buffer, 0, sizeof( buffer ) );
    
    return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Fill global I/O buffer with byte value.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR
//------------------------------------------------------------------------------
int FillBuffer( const char* pCommand )
{
   int value;

   value = strtol( ( pCommand + strlen( "fillbuf" ) + 1 ), NULL, 0 );
   
   memset( buffer, ( value & 0xFF ), sizeof( buffer ) );
   
   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Get ATA input registers from user via GUI and send command.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR     - command received and sent successfully
//         ERROR        - command not value/received
//------------------------------------------------------------------------------
int GetAndSendATACommand()
{
   int commandReceived;
   long int ataRegs[ NUM_INPUT_REGS ];

   memset( ataRegs, 0, sizeof( ataRegs ) );

   commandReceived = GetATACommandParameters( ataRegs );

   if ( commandReceived == NO_ERROR ) {
      SendATACommand( ataRegs );
   }

   return ( commandReceived );
}

//------------------------------------------------------------------------------
// Description: Display driver trace for last command.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int TraceDisplay( const char* pCommand )
{
   trc_ShowAll();
   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Clear driver trace.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int TraceClear( const char* pCommand )
{
   trc_ClearTrace();
   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Issues PCI DMA read, e.g. READ DMA EXT
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int RDMA( const char* pCommand )
{
   int commandSuccess;
   unsigned long lba;

   lba = strtol( ( pCommand + strlen( "rdma" ) + 1 ), NULL, 0 );

   printf( "Reading LBA %lu (%lXh)...", lba, lba );

   // Read LBA
   ReadDMA( lba, 1 ); commandSuccess = ukReturnValue1;

   PrintSuccess( commandSuccess );

   if ( commandSuccess == NO_ERROR ) {
      printf( "\n" );
      PrintDataBufferHex( PRINT_64_BYTES, PRINT_BYTE );
   }

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Displays general info about the selected device.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR
//------------------------------------------------------------------------------
int PrintDUTInfo( const char* pCommand )
{
   unsigned long gMaxLBAID, gMaxLBADCO, gMaxLBAHPA;
   int kSecurityWord;

   PrintPCIDeviceInfo();
   printf( "-----------------------------------\n" );

   GetMaxLBAFromIdentifyDevice(); gMaxLBAID = ugReturnValue3;
   GetMaxLBAFromReadNativeMax(); gMaxLBAHPA = ugReturnValue1;
   GetMaxLBAFromDCO(); gMaxLBADCO = ugReturnValue1;

   // Put arrays on own level to not consume lots of stack space
   {
      char wcModelString[45]; // words 27-46, so size must be >= 41 bytes
      GetModelString( wcModelString, sizeof( wcModelString ) );
      printf( "Model # .....: %s\n", wcModelString );
   }
   {
      char wcSerialNumber[25]; // words 10-19, so size must be >= 21 bytes
      GetSerialNumber( wcSerialNumber, sizeof( wcSerialNumber ) );
      printf( "Serial # ....: %s\n", wcSerialNumber );
   }
   {
      char wcFirmwareRevision[10]; // words 23-26, so size must be >= 9 bytes
      GetFirmwareRevision( wcFirmwareRevision, sizeof( wcFirmwareRevision ) );
      printf( "Firmware Rev : %s\n", wcFirmwareRevision );
   }
   printf( "-----------------------------------\n" );
   printf( "Max ID LBA ..: %08lX (%ld)\n", gMaxLBAID, gMaxLBAID );
   printf( "Max HPA LBA .: %08lX (%ld)\n", gMaxLBAHPA, gMaxLBAHPA );
   printf( "Max DCO LBA .: %08lX (%ld)\n", gMaxLBADCO, gMaxLBADCO );
   printf( "--------------------------------------------------------------------\n" );
   kSecurityWord = GetDriveSecurityState();
   printf( "Security W128: %04Xh, ", kSecurityWord );
   switch ( ( kSecurityWord & 0xFF ) )
   {
      case (SECURITY_USUPPORTED):
         printf( "Security not supported, " );
         break;
      case (SECURITY_SUPPORTED):
         printf( "SEC1: Security supported and disabled, " );
         break;
      case (SECURITY_SUPPORTED | SECURITY_FROZEN):
         printf( "SEC2: Security disabled and frozen, " );
         break;
      case (SECURITY_SUPPORTED | SECURITY_ENABLED | SECURITY_LOCKED):
         printf( "SEC4: Security locked and enabled, " );
         break;
      case (SECURITY_SUPPORTED | SECURITY_ENABLED):
         printf( "SEC5: Security unlocked and not frozen, " );
         break;
      case (SECURITY_SUPPORTED | SECURITY_ENABLED | SECURITY_FROZEN):
         printf( "SEC6: Security unlocked and frozen, " );
         break;
      default:
         printf( "Unknown security state..., " );
         break;
   }
   if ( ( kSecurityWord >> 8 ) & SECURITY_LEVEL_MAXIMUM ) {
      printf( "level MAX" );
   } else {
      printf( "level HIGH" );
   }
   printf( "\n" );
   printf( "--------------------------------------------------------------------\n" );

   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Scans for devices.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int ScanDrives( const char* pCommand )
{
   int exitProgram;

   system( "cls" );

   exitProgram = DisplayConnectedATAStorageDevices() ;

   if ( exitProgram == FALSE )
   {
      printf( "Active HDD: " );
      PrintModelString();
   }

   return ( exitProgram );
}

//------------------------------------------------------------------------------
// Description: GUI for user to input ATA command. Command sent when "S" is
//              pressed.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int ATACommand( const char* pCommand )
{
   int commandSuccess = NO_ERROR;

   SaveScreen();

   // Keep accepting commands until ESC key pressed
   while ( commandSuccess == NO_ERROR )
   {
      DisplayATACommandHistory();
      commandSuccess = GetAndSendATACommand();
      DisplayATACommandHistory();
      DisplayATARegs();
   }

   RestoreScreen();

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Turns on ATALIB.c library printing.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR
//------------------------------------------------------------------------------
int SetDebugModeOn( const char* pCommand )
{
   ukQuietMode = OFF;

   return ( NO_ERROR );
}
//------------------------------------------------------------------------------
// Description: Turns off ATALIB.c library printing.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR
//------------------------------------------------------------------------------
int SetDebugModeOff( const char* pCommand )
{
   ukQuietMode = ON;

   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: View the data in global 'buffer'.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR
//------------------------------------------------------------------------------
int ViewBuffer( const char* pCommand )
{
   unsigned long numSect;

   numSect = strtol( ( pCommand + strlen( "viewbuf" ) + 1 ), NULL, 0 );
   
   if ( numSect == 0 ) {
      numSect = 1;
   }
   
   DisplayBuffer( buffer, numSect, PRINT_WORD );
   
   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Displays the last ATA register values and command history.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR
//------------------------------------------------------------------------------
int DisplayATAInfo( const char* pCommand )
{
   SaveScreen();
   DisplayATARegs();
   DisplayATACommandHistory();

   // Wait untill ESC key is pressed
   while ( getch() != KEYBOARD_ESC ) { }

   RestoreScreen();

   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Sets a master password.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int SecuritySetMasterPassword( const char* pCommand )
{
   int commandSuccess;
   const char* pPassword = ( pCommand + strlen( "setmpw" ) + 1 );

   if ( ( pPassword == NULL ) || ( strlen( pPassword ) == 0 ) )
   {
      printf( "ERROR: password is invalid or NULL" );
      return ( ERROR );
   }

   printf( "Setting master password: %s...", pPassword );
   SecuritySetPassword( pPassword, MASTER_PASSWORD, SECURITY_LEVEL_HIGH ); commandSuccess = ukReturnValue2;
   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Sets a user password.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int SecuritySetUserPassword( const char* pCommand )
{
   int commandSuccess;
   const char* pPassword = ( pCommand + strlen( "setupw" ) + 1 );

   if ( ( pPassword == NULL ) || ( strlen( pPassword ) == 0 ) )
   {
      printf( "ERROR: password is invalid or NULL" );
      return ( ERROR );
   }

   printf( "Setting user password: %s...", pPassword );
   SecuritySetPassword( pPassword, USER_PASSWORD, SECURITY_LEVEL_HIGH ); commandSuccess = ukReturnValue2;
   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Unlocks drive with password. Attempts password as both user and
//              master.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int SecurityUnlock( const char* pCommand )
{
   int commandSuccess;
   const char* pPassword = ( pCommand + strlen( "unlock" ) + 1 );

   if ( ( pPassword == NULL ) || ( strlen( pPassword ) == 0 ) ) {
      printf( "ERROR: password is invalid or NULL" );
      return ( ERROR );
   }

   printf( "Unlocking with password %s...", pPassword );

   // Try password as master password
   SecurityUnlockPassword( pPassword, MASTER_PASSWORD ); commandSuccess = ukReturnValue1;

   if ( commandSuccess == ERROR ) {
      // Try password as user password if master password didn't work
      SecurityUnlockPassword( pPassword, USER_PASSWORD ); commandSuccess = ukReturnValue1;
   }

   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Disable password. Tries password as both user and master
//              password.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int SecurityDisable( const char* pCommand )
{
   int commandSuccess;
   const char* pPassword = ( pCommand + strlen( "disable" ) + 1 );

   if ( ( pPassword == NULL ) || ( strlen( pPassword ) == 0 ) ) {
      printf( "ERROR: password is invalid or NULL" );
      return ( ERROR );
   }

   printf( "Disabling with password %s...", pPassword );

   // Try password as master password
   SecurityDisablePassword( pPassword, MASTER_PASSWORD ); commandSuccess = ukReturnValue1;

   if ( commandSuccess == ERROR ) {
      // Try password as user password if master password didn't work
      SecurityDisablePassword( pPassword, USER_PASSWORD ); commandSuccess = ukReturnValue1;
   }

   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Issue security erase with master password.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int SecurityErase( const char* pCommand )
{
   int returnStatus;
   unsigned int originalTimeout, newTimeoutInSeconds;
   unsigned int normalEraseTimeInMin;
   const char* pPassword = ( pCommand + strlen( "erase" ) + 1 );

   if ( ( pPassword == NULL ) || ( strlen( pPassword ) == 0 ) ) {
      printf( "ERROR: password is invalid or NULL" );
      return ( ERROR );
   }
   
   printf( "Setting master password: %s...", pPassword );
   SecuritySetPassword( pPassword, MASTER_PASSWORD, SECURITY_LEVEL_HIGH ); returnStatus = ukReturnValue2;
   PrintSuccess( returnStatus );
   printf( "\n" );
   
   if ( returnStatus == NO_ERROR ) {
      char inChar;
      
      GetEstimatedSecureEraseTimesInMin(); normalEraseTimeInMin = ukReturnValue1;
      originalTimeout = tmr_get_command_timeout();
      
      if ( normalEraseTimeInMin == 0 ) {
         newTimeoutInSeconds = ( DEFAULT_ERASE_TIME_IN_MINUTES * 60 );     // use default, overkill time allowance
      } else {
         newTimeoutInSeconds = ( 2 * ( normalEraseTimeInMin * 60 ) );      // 2x the expected time just in case
      }
     
      printf( "Expected erase time: %d seconds (%d minutes)\n", ( normalEraseTimeInMin * 60 ), normalEraseTimeInMin );
      printf( "Command timeout ...: %d seconds (%d minutes)\n", newTimeoutInSeconds, ( newTimeoutInSeconds / 60 ) );
      
      printf( "\nProceed with erase? (Y/N)" );
      scanf( "%c", &inChar );
      DumpLine( stdin );
      
      if ( ( inChar == 'Y' ) || ( inChar == 'y' ) ) {
         char* pTimeStr;
         
         GetTime( &pTimeStr );
         tmr_set_command_timeout( newTimeoutInSeconds );
         InstallClock();
         
         printf( "Executing security erase at %s...", pTimeStr );
         fflush( stdout );
         SecureErase( pPassword, MASTER_PASSWORD, NORMAL_SECURE_ERASE ); returnStatus = ukReturnValue1;
         PrintSuccess( returnStatus );
         
         if ( returnStatus == NO_ERROR ) {
            GetTime( &pTimeStr );
            printf( " at %s", pTimeStr );  
         }
         
         UninstallClock();
         tmr_set_command_timeout( originalTimeout );
      }
   }
   
   return ( returnStatus );
}

//------------------------------------------------------------------------------
// Description: Displays all SMART attributes.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int SmartAttributes( const char* pCommand )
{
   int commandSuccess;

   printf( "Getting SMART attribute data..." );
   commandSuccess = GetSmartAttributes();
   PrintSuccess( commandSuccess );
   
   if ( commandSuccess == NO_ERROR ) {
      int byte;
      SMARTData_t* pSmartData = (SMARTData_t *)buffer;
      SMARTAttribute_t* pSmartAttribute = (SMARTAttribute_t *)pSmartData->wcAttribs;
   
      printf(    "\nAtt ID | Flags | Value | Worst | Raw" );
      printf(    "\n-------+-------+-------+-------+--------------" );
      for ( byte = 0; byte < sizeof( pSmartData->wcAttribs ); byte += sizeof( SMARTAttribute_t ) )
      {
         if ( pSmartAttribute->idNum != 0 ) {
            printf( "\n   %3d | %04Xh |  %02Xh  |  %02Xh  | ", pSmartAttribute->idNum, pSmartAttribute->flag, pSmartAttribute->curValue, pSmartAttribute->worstValue );
            printf( "%02X%02X%02Xh", pSmartAttribute->rawValueHi, pSmartAttribute->rawValueMid, pSmartAttribute->rawValueLo );
         }
         
         pSmartAttribute += 1;
      }
   }
 
   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Returns drive to factory max LBA, i.e. DCO LBA.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int RemoveHPAAndDCO( const char* pCommand )
{
   int commandSuccess;
   unsigned long gMaxLBAID = 0, gMaxLBADCO = 0;

   printf( "Removing DCO and HPA if they exist..." );

   // Remove HPA
   RemoveHPA();

   // Then DCO, must be in that order
   DeviceConfigurationRestore();

   GetMaxLBAFromIdentifyDevice(); gMaxLBAID = ugReturnValue3;
   GetMaxLBAFromDCO(); gMaxLBADCO = ugReturnValue1;

   // HPA and DCO areas removed if the 2 LBAs match
   if ( gMaxLBAID == gMaxLBADCO ) {
      commandSuccess = NO_ERROR;
      PrintSuccess( commandSuccess );
   }

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Enables support for the security feature set via DCO.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int ATASecuritySupport( const char* pCommand )
{
   int commandSuccess;

   printf( "ENABLING security via DCO..." );
   ChangeSecuritySupportViaDCO( DCO_TURN_SECURITY_ON ); commandSuccess = ukReturnValue1;

   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Unsupports the security feature set via DCO.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int ATASecurityUnsupport( const char* pCommand )
{
   int commandSuccess;

   printf( "DISABLING security via DCO..." );
   ChangeSecuritySupportViaDCO( DCO_TURN_SECURITY_OFF ); commandSuccess = ukReturnValue1;

   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Sets upper HPA lba.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int DcoSetLBA( const char* pCommand )
{
   int commandSuccess;
   unsigned long lba;

   
   lba = strtol( ( pCommand + strlen( "dco" ) + 1 ), NULL, 0 );   
   printf( "Setting DCO to LBA %lu (%lXh)...", lba, lba );
   ChangeDriveCapacityViaDCO( lba ); commandSuccess = ukReturnValue1;

   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Sets upper ID lba.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int HpaSetLBA( const char* pCommand )
{
   int commandSuccess;
   unsigned long lba;

   lba = strtol( ( pCommand + strlen( "hpa" ) + 1 ), NULL, 0 );   

   printf( "Setting HPA to LBA %lu (%lXh)...", lba, lba );
   SetHPA( ON, HPA_NON_VOLATILE, lba); commandSuccess = ukReturnValue1;
   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Issues a software reset.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int Reset( const char* pCommand )
{
   int commandSuccess;

   printf( "Issuing Software Reset..." );
   SoftwareReset(); commandSuccess = ukReturnValue1;
   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Issues an ID and DCID command and displays some of the data.
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int Id( const char* pCommand )
{
   int commandSuccess;
   unsigned long gMaxLBAID, gMaxLBADCO, gMaxLBAHPA;

   printf( "Issuing Identify Device..." );

   // Identify device
   IdentifyDevice(); commandSuccess = ukReturnValue1;

   if ( commandSuccess == NO_ERROR ) {
      printf( "Success\n\n" );
      PrintDataBufferHex( PRINT_258_BYTES, PRINT_WORD );

      printf( "\n" );

      GetMaxLBAFromIdentifyDevice(); gMaxLBAID = ugReturnValue3;
      printf( "Max ID LBA : %08lX (%ld)\n", gMaxLBAID, gMaxLBAID );

      GetMaxLBAFromReadNativeMax(); gMaxLBAHPA = ugReturnValue1;
      printf( "Max HPA LBA: %08lX (%ld)\n", gMaxLBAHPA, gMaxLBAHPA );

      GetMaxLBAFromDCO(); gMaxLBADCO = ugReturnValue1;
      printf( "Max DCO LBA: %08lX (%ld)\n", gMaxLBADCO, gMaxLBADCO );
   }

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Issue 1 block PIO read to selected LBA. >>read <LBA>
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int RPIO( const char* pCommand )
{
   int commandSuccess;
   unsigned long lba;

   lba = strtol( ( pCommand + strlen( "read" ) + 1 ), NULL, 0 );   

   printf( "Reading LBA %lu (%lXh)...", lba, lba );

   // Read LBA
   ReadSectorsInLBA48( lba, 1 ); commandSuccess = ukReturnValue1;

   PrintSuccess( commandSuccess );

   if ( commandSuccess == NO_ERROR ) {
      printf( "\n" );
      PrintDataBufferHex( PRINT_64_BYTES, PRINT_BYTE );
   }

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Issues a 1 block PIO write to the selected lba. >>write <LBA>
//
// Input:  pCommand     - user command line input
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int WPIO( const char* pCommand )
{
   int commandSuccess;
   unsigned long lba;

   lba = strtol( ( pCommand + strlen( "write" ) + 1 ), NULL, 0 );   

   // Write data
   memset( buffer, lba, sizeof( buffer ) );
   buffer[0] = ( lba & 0xFF );
   buffer[1] = ( ( lba >> 8 ) & 0xFF );
   buffer[2] = ( ( lba >> 16 ) & 0xFF );
   buffer[3] = ( ( lba >> 24 ) & 0xFF );

   printf( "Writing %lXh to LBA %lu...", lba, lba );

   // Write LBA
   WriteSectorsInLBA48( lba, 1 ); commandSuccess = ukReturnValue1;

   PrintSuccess( commandSuccess );

   return ( commandSuccess );
}

//------------------------------------------------------------------------------
// Description: Checks user input to a command support by this program, then
//              executes it.
//
// Input:  pCommand     - user command line input
// Output: TRUE         - exit program
//         FALSE        - do not exit program
//------------------------------------------------------------------------------
int DoCommand( const char* pCommand )
{
   int exitProgram = FALSE;
   int commandSuccess = ERROR;
   int commandFound = FALSE;
   int eachAtacmdCommand, numAtacmdCommands;

   if ( pCommand == NULL ) { printf( "NULL command pointer!" ); exitProgram = TRUE; }
   if ( !StringCompareIgnoreCase( pCommand, "ex", strlen( "ex" ) ) ) { exitProgram = TRUE; }

   if ( exitProgram == FALSE ) {
//      printf( "\n" );

      numAtacmdCommands = ( sizeof( wtAtacmdCommands ) / sizeof( struct tEachCommand ) );

      // Search through all the user-defined ATA command macros
      for ( eachAtacmdCommand = 0; eachAtacmdCommand < numAtacmdCommands; eachAtacmdCommand++ )
      {
         if ( !StringCompareIgnoreCase( pCommand, wtAtacmdCommands[ eachAtacmdCommand ].pName, strlen( wtAtacmdCommands[ eachAtacmdCommand ].pName ) ) ) {
            commandFound = TRUE;
            
            commandSuccess = (* wtAtacmdCommands[ eachAtacmdCommand ].pFunctionPtr)( pCommand );
            break;
         }
      }

      if ( commandFound != TRUE ) {
         commandSuccess = NO_ERROR;
         printf( "Unknown command: %s", pCommand );
      }
   }

   return ( exitProgram );
}

//------------------------------------------------------------------------------
// Description: Entry point
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
int main( void )
{
   int exitProgram = FALSE;
   unsigned char charIdx;
   char key;

   charIdx = 0;
   memset( wcCommand, 0, sizeof( wcCommand ) );
   InitializeParams();

   exitProgram = ScanDrives( NULL );

   if ( exitProgram == TRUE ) {
      return 0;
   }

   // Normal ATA commands should take no more than 3 seconds to complete
   tmr_set_command_timeout( 3L );

   printf( "\n" );

   while ( 1 )
   {
      printf( ">" );

      while ( 1 )
      {
         key = getchar();

         if ( ( key == '\r' ) || ( key == '\n' ) ) {
            if ( key == '\r' ) {
               getchar(); // consume the final \n
               wcCommand[ charIdx ] = '\0';
               charIdx++;
            }

            wcCommand[ charIdx ] = '\0';

            exitProgram = DoCommand( wcCommand );

            memset( wcCommand, 0 , sizeof( wcCommand ) );
            charIdx = 0;

            break;
            
//         } else if ( ( key == ) || ( key == ) ){
            // getchar
            
         } else {
            wcCommand[ charIdx ] = key;
            charIdx++;
         }
      }

      if ( exitProgram == TRUE ) {
         break;
      }

// Add user entry to command history

      printf( "\n" );
   }

   ATALIB_CleanUp();

   return 0;
}
