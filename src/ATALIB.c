//******************************************************************************
// ATA command library -- ATALIB.C
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
// Special thanks to:
//    Hale Landis (http://ata-atapi.com/)
//    Randy Maude (OFF THE GRID)
//    Logan Straatemeier (straatemeier@gmail.com)
//
// Purpose:
//---------
// The ATALIB library contains all the ATA command related functions
// utilizing the Hale Landis freeware driver found at:
// (http://ata-atapi.com/). The driver has been modified to compile with
// the Open Watcom IDE (http://www.openwatcom.org/index.php/Main_Page).
//
// The purpose of this library is to provide common functions for other
// programs to create ATA drive diagnostic software to be used for testing.
//
// History:
//---------
// Originally this library was created to be used for scripting, as is evident
// in the global return variables and print/log options for some of the
// functions. Newly added code does away with this focus and is more geared
// more towards acting as support/helper code for larger programs. I do plan to
// update all the code to reflect this intent, but until then the difference in
// style between some of the functions will remain. I also plan to remove the
// types-Hungarian prefixes, another relic of my early programming days ;)
//
// Many parts of this code were inspired by HDDErase, another program I wrote
// while at CMRR, UCSD: http://cmrr.ucsd.edu/people/Hughes/secure-erase.html
//
// Limitations of this software:
// -----------------------------
// * Functions in this library are not optimized for performance and therefore
// shouldn't be used as the basis for an IOPS or throughput test.
// * Currently there is no support for either SCSI or enterprise drives. Such
// drives operate by a different standard than ATA (see http://www.t10.org/).
// * No multi-thread support. Due to this library's scripting origins some of
// the functions are not thread safe.
//
// References:
// -----------
// http://www.t13.org/ --> projects --> working drafts --> ATA-6 or ATA-7
//
//******************************************************************************

//----------------------------------[INCLUDES]----------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>        // for offsetof()
#include <string.h>
#include <dos.h>
#include <malloc.h>

#ifndef   __PCIMAP_H__
#include   "PCIMap.h"
#define   __PCIMAP_H__
#endif // __PCIMAP_H__

#ifndef   __ATAIO_H__
#include   "ataio.h"
#define   __ATAIO_H__
#endif // __ATAIO_H__

#ifndef   __TOOLS_H__
#include   "tools.h"
#define   __TOOLS_H__
#endif // __TOOLS_H__

#ifndef   __ATALIB_H__
#include   "atalib.h"
#define   __ATALIB_H__
#endif // __ATALIB_H__

//----------------------------------[STRUCTS]-----------------------------------


//------------------------------[GLOBAL VARIABLES]------------------------------

// Variables
int ukDevicePosition;
int ukMulti       = 0;
int ukQuietMode   = OFF;             // Controls all the printing by ATALIB.c
int ukPrintOutput = PRINT_SCREEN;    // Controls where everything is printed in ATALIB.c
int ukTotalErrors = 0;               // Global error counter for ATALIB.c
int uIRQNum       = INVALID_VALUE;   // IRQ num of active/selected device
int uActiveDeviceIndex = -1;

int ukReturnValue1 = -1;
int ukReturnValue2 = -1;
int ukReturnValue3 = -1;
int ukReturnValue4 = -1;
int ukReturnValue5 = -1;
int ukReturnValue6 = -1;

unsigned long ugReturnValue1 = -1;
unsigned long ugReturnValue2 = -1;
unsigned long ugReturnValue3 = -1;
unsigned long ugReturnValue4 = -1;
unsigned long ugReturnValue5 = -1;

// Arrarys
unsigned char buffer[BUFFER_SIZE];   // The global data buffer for ATALIB.c
unsigned char wcMaxLBA[8];           // Holds the max LBA from different ATALIB.c functions
char wcPrintBuffer[NUMBER_OF_CHARACTERS_IN_DOS_LINE+1];   // Allocates memory for ATALIB.c printing
char wcDriveString[3];
char wcPasswordString[33];
char wcDisclaimerReply[5];
char wcUserReply[5];

static struct StorageDevice_t wtStorageDevices[ MAX_STORAGE_DEVICES ];

// Pointers
FILE* upLog;
char* upPrintString = wcPrintBuffer;
unsigned char far* upBufferPtr;

//-----------------------------[LOCAL DECLARATIONS]-----------------------------


//------------------------------[ATALIB FUNCTIONS]------------------------------

//------------------------------------------------------------------------------
// Description: Sends a non-data command.
//
// Input:  cmd          - command register
//         feat         - features register
//         secCnt       - sector count
//         cylinder     - cylinder address
//         head         - head number
//         secNum       - sector number
//
// Output: NO_ERROR = successful; ERROR = unsuccessful
//------------------------------------------------------------------------------
int SendNonDataCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned int cylinder, unsigned int head, unsigned int secNum )
{
   return ( reg_non_data_chs( ukDevicePosition, cmd, feat, secCnt, cylinder, head, secNum ) );
}

//------------------------------------------------------------------------------
// Description: Sends LBA 28 data-in command.
//
// Input:  cmd          - command register
//         feat         - features register
//         secCnt       - sector count
//         lba          - 32-bit lba
//
// Output: NO_ERROR = successful; ERROR = unsuccessful
//------------------------------------------------------------------------------
int SendLBA28DataInCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lba )
{
   // Clear the buffer so there's no remnant data in buffer before reading
   memset( buffer, 0, sizeof( buffer ) );

   return ( reg_pio_data_in_lba28( ukDevicePosition, cmd, feat, secCnt, lba, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt, ukMulti ) );
}

//------------------------------------------------------------------------------
// Description: Sneds LBA 48 data-in command.
//
// Input:  cmd          - command register
//         feat         - features register
//         secCnt       - sector count
//         lba low      - lower 32-bit lba address
//         lba high     - upper 32-bit lba address
//
// Output: NO_ERROR = successful; ERROR = unsuccessful
//------------------------------------------------------------------------------
int SendLBA48DataInCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lbaLow, unsigned long lbaHigh )
{
   // Clear the buffer so there's no remnant data in buffer before reading
   memset( buffer, 0, sizeof( buffer ) );

   return ( reg_pio_data_in_lba48( ukDevicePosition, cmd, feat, secCnt, lbaHigh, lbaLow, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt, ukMulti ) );
}

//------------------------------------------------------------------------------
// Description: Sends LBA 28 data-out command.
//
// Input:  cmd          - command register
//         feat         - features register
//         secCnt       - sector count
//         lba          - 32-bit lba
//
// Output: NO_ERROR = successful; ERROR = unsuccessful
//------------------------------------------------------------------------------
int SendLBA28DataOutCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lba )
{
   return ( reg_pio_data_out_lba28( ukDevicePosition, cmd, feat, secCnt, lba, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt, ukMulti ) );
}

//------------------------------------------------------------------------------
// Description: Sends LBA 28 data-out command.
//
// Input:  cmd          - command register
//         feat         - features register
//         secCnt       - sector count
//         lba low      - lower 32-bit lba address
//         lba high     - upper 32-bit lba address
//
// Output: NO_ERROR = successful; ERROR = unsuccessful
//------------------------------------------------------------------------------
int SendLBA48DataOutCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lbaLow, unsigned long lbaHigh )
{
   return ( reg_pio_data_out_lba48( ukDevicePosition, cmd, feat, secCnt, lbaHigh, lbaLow, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt, ukMulti ) );
}

//------------------------------------------------------------------------------
// Description: Sends LBA 48 DMA command. PCI or ISA depending on I/O address.
//
// Input:  cmd          - command register
//         feat         - features register
//         secCnt       - sector count
//         lba low      - lower 32-bit lba address
//         lba high     - upper 32-bit lba address
//
// Output: NO_ERROR = successful; ERROR = unsuccessful
//------------------------------------------------------------------------------
int SendLBA48DMACommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lbaLow, unsigned long lbaHigh )
{
   int returnStatus;

   if ( ( pio_bmide_base_addr == INVALID_VALUE ) && ( ( pio_base_addr1 == LEGACY_PRIMARY_BASEPORT ) || ( pio_base_addr1 == LEGACY_SECONDARY_BASEPORT ) ) ) {
      returnStatus = EnableISADMA();

      if ( returnStatus == NO_ERROR ) {
         dma_isa_lba48( ukDevicePosition, cmd, feat, secCnt, lbaHigh, lbaLow, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt );
      }
   } else {
      returnStatus = EnableInterrupt();

      if ( returnStatus == NO_ERROR ) {
         returnStatus = EnablePCIDMA();
      }

      if ( returnStatus == NO_ERROR ) {
         dma_pci_lba48( ukDevicePosition, cmd, feat, secCnt, lbaHigh, lbaLow, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt );

         if ( ( pio_base_addr1 == LEGACY_PRIMARY_BASEPORT ) || ( pio_base_addr1 == LEGACY_SECONDARY_BASEPORT ) ) {
            DisableInterrupt();
         }
      }
   }

   return ( returnStatus );
}

//------------------------------------------------------------------------------
// Description: Sends LBA 28 DMA command. PCI or ISA depending on I/O address.
//
// Input:  cmd          - command register
//         feat         - features register
//         secCnt       - sector count
//         lba          - 32-bit lba address
//
// Output: NO_ERROR = successful; ERROR = unsuccessful
//------------------------------------------------------------------------------
int SendLBA28DMACommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lba )
{
   int returnStatus;

   if ( ( pio_bmide_base_addr == INVALID_VALUE ) && ( ( pio_base_addr1 == LEGACY_PRIMARY_BASEPORT ) || ( pio_base_addr1 == LEGACY_SECONDARY_BASEPORT ) ) ) {
      returnStatus = EnableISADMA();

      if ( returnStatus == NO_ERROR ) {
         dma_isa_lba28( ukDevicePosition, cmd, feat, secCnt, lba, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt );
      }
   } else {
      returnStatus = EnableInterrupt();

      if ( returnStatus == NO_ERROR ) {
         returnStatus = EnablePCIDMA();
      }

      if ( returnStatus == NO_ERROR ) {
         dma_pci_lba28( ukDevicePosition, cmd, feat, secCnt, lba, FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), secCnt );

         if ( ( pio_base_addr1 == LEGACY_PRIMARY_BASEPORT ) || ( pio_base_addr1 == LEGACY_SECONDARY_BASEPORT ) ) {
            DisableInterrupt();
         }
      }
   }

   return ( returnStatus );
}

//------------------------------------------------------------------------------
// Description: Issue an Identify Device command.
//
// Input:  None
//
// Output: ukReturnValue1 - NO_ERROR = successful
//                          ERROR    = unsuccessful
//
//         *buffer        - 512-byte ID data
//------------------------------------------------------------------------------
void IdentifyDevice()
{
   int returnStatus;

   // Clear the buffer
   memset (buffer, 0, sizeof (buffer));

   if (ukQuietMode == OFF)
   {
      upPrintString = "\n\nIssuing IDENTIFY DEVICE command";
      PrintString (ukPrintOutput);
   }

   // Identify Device in LBA28 mode
   returnStatus = reg_pio_data_in_lba28 (
      ukDevicePosition, CMD_IDENTIFY_DEVICE,
      0, 0,
      0L,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0
      );

   ukReturnValue1 = returnStatus;
   return;
} // End IdentifyDevice

//------------------------------------------------------------------------------
// Description: Issue a Device Configuration Identify command.
//
// Input:  None
//
// Output: ukReturnValue1 - NO_ERROR = successful
//                          ERROR    = unsuccessful
//
//         *buffer        - 512-byte DC ID data
//------------------------------------------------------------------------------
void DeviceConfigurationIdentify()
{
   int returnStatus;

   // Clear the buffer
   memset( buffer, 0, sizeof( buffer ) );

   if (ukQuietMode == OFF)
   {
      upPrintString = "\n\nIssuing DEVICE CONFIGURATION IDENTIFY command";
      PrintString (ukPrintOutput);
   }

   // Device Configuration Identify in LBA28 mode
   returnStatus = reg_pio_data_in_lba28 (
     ukDevicePosition, 0xB1,
     0xC2, 0,
     0L,
     FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
     1, 0
     );

   ukReturnValue1 = returnStatus;
   return;
} // End DeviceConfigurationIdentify

//------------------------------------------------------------------------------
// Description: Initialize pointers, variables, etc. required by ATALIB.c and
//              Landis driver.  THIS NEEDS TO BE CALLED AT THE BEGINNING OF
//              EVERY PROGRAM!
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void ATALIB_Initialize()
{
   // Initialize far pointer to the I/O buffer
   upBufferPtr = (unsigned char far *) buffer;

   // Tell ATADRVR how big the buffer is
   reg_buffer_size = BUFFER_SIZE;

   // Allocate memory for the global print string
   upPrintString = (char *)malloc( NUMBER_OF_CHARACTERS_IN_DOS_LINE + 1 );

} // End ATALIB_Initialize

//------------------------------------------------------------------------------
// Description: Restores interrupts and anything else before exiting program.
//              THIS NEEDS TO BE CALLED AT THE END OF EVERY EVERY PROGRAM!
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void ATALIB_CleanUp()
{
   DisableInterrupt();
}

//------------------------------------------------------------------------------
// Description: Get the Serial Number from Identify Device words 10-19.
//
// Input:  pIDData            - GET_ID_DATA to issue ID and store in 'buffer'
//         pSerialNum         - Pointer to serial number string
//         buffSizeInBytes    - Size of buffer pointed to by pFirmwareRevision
//                              Here in order to make sure there's enough space
//                              in the memory location pointed to
//
// Output: None
//------------------------------------------------------------------------------
void GetSerialNumber( void* pIDData, char* const pSerialNum, unsigned int buffSizeInBytes )
{
   int kSerialCounter, kIndex, firstChar;
   char ch;

   // Check there's enough memory at location to hold all 10 words + 1 byte for '\0'
   if ( buffSizeInBytes < ( ( 2 * ( 19 - 10 + 1 ) ) + 1 ) )
   {
      printf( "ERROR: Buffer size %d, not enough to hold serial number", buffSizeInBytes );
      return;
   }

   // Initialize the array and array index
   memset( pSerialNum, 0, buffSizeInBytes );
   kIndex = 0;

   // Issue Identify Device command to fill buffer with ID data
   if ( pIDData == GET_ID_DATA ) {
      IdentifyDevice();
   }

   firstChar = FALSE;

   // Print the Id word 10-19 - Serial Number.
   for ( kSerialCounter = 10; kSerialCounter <= 19; kSerialCounter++ )
   {
      // First char in word
      ch = *( buffer + ( ( kSerialCounter * 2 ) + 1 ) );

      if ( ( firstChar == TRUE ) || ( ch != ' ' ) )
      {
         firstChar = TRUE;
         *( pSerialNum + kIndex ) = ch;
         kIndex++;
      }

      // Second char in word
      ch = *( buffer + ( ( kSerialCounter * 2 ) ) );

      if ( ( firstChar == TRUE ) || ( ch != ' ' ) )
      {
         firstChar = TRUE;
         *( pSerialNum + kIndex ) = ch;
         kIndex++;
      }
   }

   // Remove whitespace
   TOOLS_RemoveTrailingSpaces( pSerialNum );

   return;
} // End GetSerialNumber

//------------------------------------------------------------------------------
// Description: Print the Serial Number from Identify Device words 10-19.
//              print to the next immediate character, not on a new line.
//
// Input:  None
//
// Output: Serial Number printed to screen (stdout)
//------------------------------------------------------------------------------
void PrintSerialNumber()
{
   char wcSerialNumber[25]; // words 10-19, so size must be >= 21 bytes

   // Populate with serial number
   GetSerialNumber( GET_ID_DATA, wcSerialNumber, sizeof( wcSerialNumber ) );

   // Copy string to global string and print
   sprintf( upPrintString, "%s", wcSerialNumber );
   PrintString( ukPrintOutput );

   return;
} // End PrintSerialNumber

//------------------------------------------------------------------------------
// Description: Get the Firmware Revision from Identify Device words 23-26.
//
// Input:  pIDData            - GET_ID_DATA to issue ID and store in 'buffer'
//         pFirmwareRevision  - Pointer to firmware revision string
//         buffSizeInBytes    - Size of buffer pointed to by pFirmwareRevision
//                              Here in order to make sure there's enough space
//                              in the memory location pointed to
//
// Output: None
//------------------------------------------------------------------------------
void GetFirmwareRevision( void* pIDData, char* const pFirmwareRevision, unsigned int buffSizeInBytes )
{
   int kFirmwareCounter, kIndex, firstChar;
   char ch;

  // Check there's enough memory at location to hold all 4 words + 1 byte for '\0'
   if ( buffSizeInBytes < ( ( 2 * ( 26 - 23 + 1 ) ) + 1 ) )
   {
      printf( "ERROR: Buffer size %d, not enough to hold firmware revision", buffSizeInBytes );
      return;
   }

   // Initialize the array and array index
   memset( pFirmwareRevision, 0, buffSizeInBytes );
   kIndex = 0;

   // Issue Identify Device command to fill buffer with ID data
   if ( pIDData == GET_ID_DATA ) {
      IdentifyDevice();
   }

   firstChar = FALSE;

   // Print the Id word 23-26 - Firmware Rev.
   for ( kFirmwareCounter = 23; kFirmwareCounter <= 26; kFirmwareCounter++ )
   {
      // First byte in word
      ch = *( buffer + ( ( kFirmwareCounter * 2 ) + 1 ) );

      if ( ( firstChar == TRUE ) || ( ch != ' ' ) )
      {
         *( pFirmwareRevision + kIndex ) = ch;
         kIndex++;
      }

      // Second byte in word
      ch = *( buffer + ( kFirmwareCounter * 2 ) );

      if ( ( firstChar == TRUE ) || ( ch != ' ' ) )
      {
         *( pFirmwareRevision + kIndex ) = ch;
         kIndex++;
      }
   }

   // Remove whitespace
   TOOLS_RemoveTrailingSpaces( pFirmwareRevision );

   return;
} // End GetFirmwareRevision

//------------------------------------------------------------------------------
// Description: Print the Firmware Revision from Identify Device words 23-26.
//              print to the next immediate character, not on a new line.
//
// Input:  None
//
// Output: Firmware Revision printed to screen (stdout)
//------------------------------------------------------------------------------
void PrintFirmwareRevision()
{
   char wcFirmwareRevision[10]; // words 23-26, so size must be >= 9 bytes

   // Populate with firmware revision string
   GetFirmwareRevision( GET_ID_DATA, wcFirmwareRevision, sizeof( wcFirmwareRevision ) );

   // Copy string to global string and print
   sprintf( upPrintString, "%s", wcFirmwareRevision );
   PrintString( ukPrintOutput );

   return;
}// End PrintFirmwareRevision

//------------------------------------------------------------------------------
// Description: Get the model string from Identify Device words 27-46.
//
// Input:  pIDData            - GET_ID_DATA to issue ID and store in 'buffer'
//         pModelNum          - Pointer to model string
//         buffSizeInBytes    - Size of buffer pointed to by pFirmwareRevision
//                              Here in order to make sure there's enough space
//                              in the memory location pointed to
//
// Output: None
//------------------------------------------------------------------------------
void GetModelString( void* pIDData, char* const pModelNum, unsigned int buffSizeInBytes )
{
   int kModelCounter, kIndex, firstChar;
   char ch;

   // Check there's enough memory at location to hold all 20 words + 1 byte for '\0'
   if ( buffSizeInBytes < ( ( 2 * ( 46 - 27 + 1 ) ) + 1 ) )
   {
      printf( "ERROR: Buffer size %d, not enough to hold model string", buffSizeInBytes );
      return;
   }

   // Initialize the array and array index
   memset( pModelNum, 0, buffSizeInBytes );
   kIndex = 0;

   // Issue Identify Device command to fill buffer with ID data
   if ( pIDData == GET_ID_DATA ) {
      IdentifyDevice();
   }

   firstChar = FALSE;

   // Increment by 16-bit words
   for ( kModelCounter = 27; kModelCounter <= 46; kModelCounter++ )
   {
      // HOB
      ch = *( buffer + ( ( kModelCounter * 2 ) + 1 ) );

      if ( ( firstChar == TRUE ) || ( ch != ' ' ) )
      {
         *( pModelNum + kIndex ) = ch;
         kIndex++;
      }

      // LOB
      ch = *( buffer + ( kModelCounter * 2 ) );

      if ( ( firstChar == TRUE ) || ( ch != ' ' ) )
      {
         *( pModelNum + kIndex ) = ch;
         kIndex++;
      }
   }

   // Remove whitespace
   TOOLS_RemoveTrailingSpaces( pModelNum );

   return;
} // End GetModelString

//------------------------------------------------------------------------------
// Description: Print the model string from Identify Device words 27-46.  Will
//              print to the next immediate character, not on a new line.
//
// Input:  None
//
// Output: Model string printed to screen (stdout)
//------------------------------------------------------------------------------
void PrintModelString()
{
   char wcModelString[45]; // words 27-46, so size must be >= 41 bytes

   // Populate with model number
   GetModelString( GET_ID_DATA, wcModelString, sizeof( wcModelString ) );

   // Copy string to global string and print
   sprintf( upPrintString, "%s", wcModelString );
   PrintString( ukPrintOutput );

   return;
} // End PrintModelString

//------------------------------------------------------------------------------
// Description: Checks if word 83 bit 10, 48-bit Address Feature Set,
//              is set.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = 48-Bit addressing NOT supported
//                          ON  = 48-Bit addressing supported
//------------------------------------------------------------------------------
void Check48BitAddressingSupported()
{
   int k48BitAddressingFlag;
   unsigned char cIDWord83, cBit10Mask;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Check word 83 from ID buffer
   cIDWord83 = *(buffer + ( 83 * 2 + 1 ) );
   cBit10Mask = 0x04;
   if ((cIDWord83 & cBit10Mask) == 0)
   {
      k48BitAddressingFlag = OFF;
   }
   else
   {
      k48BitAddressingFlag = ON;
   }

   ukReturnValue1 = k48BitAddressingFlag;
   return;
} // End Check48BitAddressingSupported

//------------------------------------------------------------------------------
// Description: Checks if word 82 bit 10, Host Protected Area Feature Set,
//              is set.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = HPA NOT supported
//                          ON  = HPA supported
//------------------------------------------------------------------------------
void CheckHPASupported()
{
   int kHPASupportedFlag;
   unsigned char cIDWord82, cBit10Mask;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Check word 82 from ID buffer
   cIDWord82 = *(buffer + ( 82 * 2 + 1 ) );
   cBit10Mask = 0x04;
   if ((cIDWord82 & cBit10Mask) == 0)
   {
      kHPASupportedFlag = OFF;
   }
   else
   {
      kHPASupportedFlag = ON;
   }

   ukReturnValue1 = kHPASupportedFlag;
   return;
} // End CheckHPASupported

//------------------------------------------------------------------------------
// Description: Checks if word 83 bit 11, Device Configuration Overlay
//              Feature Set, is set.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = DCO NOT Supported
//                          ON  = DCO Supported
//------------------------------------------------------------------------------
void CheckDCOSupported()
{
   int kDCOSupportedFlag;
   unsigned char cIDWord83, cBit11Mask;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Check word 83 from ID buffer
   cIDWord83 = *(buffer + ( 83 * 2 + 1 ) );
   cBit11Mask = 0x08;
   if ((cIDWord83 & cBit11Mask) == 0)
   {
      kDCOSupportedFlag = OFF;
   }
   else
   {
      kDCOSupportedFlag = ON;
   }

   ukReturnValue1 = kDCOSupportedFlag;
   return;
} // End CheckDCOSupported

//------------------------------------------------------------------------------
// Description: Checks if word 128 bit 0, Security Mode Feature Set
//              Support, is set.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = Security NOT supported
//                          ON  = Security supported
//------------------------------------------------------------------------------
void CheckSecuritySupported()
{
   int kSecuritySupportedFlag;
   unsigned char cIDWord128Low;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get first byte of security word 128
   cIDWord128Low = *( buffer + ( 128 * 2 ) );

   if ((cIDWord128Low & SECURITY_SUPPORTED) == 0)
   {
      kSecuritySupportedFlag = OFF;
   }
   else
   {
      kSecuritySupportedFlag = ON;
   }

   ukReturnValue1 = kSecuritySupportedFlag;
   return;
} // End CheckSecuritySupported

//------------------------------------------------------------------------------
// Description: Checks if word 128 bit 5, Enhanced Secure Erase Support,
//              is set.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = Enhanced secure erase NOT supported
//                          ON  = Enhanced secure erase supported
//------------------------------------------------------------------------------
void CheckEnhancedSecureEraseSupported()
{
   int kEnhancedSecureEraseFlag;
   unsigned char cIDWord128Low;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get first byte of security word 128
   cIDWord128Low = *( buffer + ( 128 * 2 ) );

   if ((cIDWord128Low & SECURITY_ENHANCED_SECURITY) == 0)
   {
      kEnhancedSecureEraseFlag = OFF;
   }
   else
   {
      kEnhancedSecureEraseFlag = ON;
   }

   ukReturnValue1 = kEnhancedSecureEraseFlag;
   return;
} // End CheckEnhancedSecureEraseSupported

//------------------------------------------------------------------------------
// Description: Get the maximum LBA from Identify Device Words 100-103 if
//              48-bit addressing is supported, or from Words 60-61 if 48-
//              bit addressing is NOT supported.
//
// Input:  None
//
// Output: ugReturnValue1 - Contains lower 32 bits of maximum number of
//                          LBAs from Identify Device
//         ugReturnValue2 - Contains upper 32 bits of maximum number of
//                          LBAs from Identify Device
//         ugReturnValue3 - Contains lower 32 bits of maximum LBA from
//                          Identify Device
//         ugReturnValue4 - Contains upper 32 bits of maximum LBA from
//                          Identify Device
//         ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void GetMaxLBAFromIdentifyDevice()
{
   int returnStatus, k48BitAddressingFlag;
   int kMaxLBACounter, kMaxLBABufferIndex;
   unsigned long gMaxNumberOfLBAsFromIDLow, gMaxNumberOfLBAsFromIDHigh;
   unsigned long gMaxLBAFromIDLow, gMaxLBAFromIDHigh;

   // Initialize the variables
   kMaxLBABufferIndex  = 0;          // Start at index 0
   returnStatus       = NO_ERROR;   // Default is no error

   // Check 48-bit addressing supported
   Check48BitAddressingSupported ();
   k48BitAddressingFlag = ukReturnValue1;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Clear the buffer that will hold the max lba
   memset (wcMaxLBA, 0, sizeof (wcMaxLBA));

   // Check which LBA mode the drive supports
   switch (k48BitAddressingFlag)
   {
      case (OFF):
         // 48-bit Addressing is NOT supported

         // Words 60-61 contain max number of LBAs
         for (kMaxLBACounter = 60; kMaxLBACounter <= 61; kMaxLBACounter++)
         {
            // For each word, copy two bytes to the buffer
            wcMaxLBA[kMaxLBABufferIndex] = *(buffer + (kMaxLBACounter*2));
            kMaxLBABufferIndex++;
            wcMaxLBA[kMaxLBABufferIndex] = *(buffer + (kMaxLBACounter*2 + 1));
            kMaxLBABufferIndex++;
         }
         break;

      case (ON):
         // 48-bit Addressing is supported

         // Words 100-103 contain max number of LBAs
         for (kMaxLBACounter = 100; kMaxLBACounter <= 103; kMaxLBACounter++)
         {
            // For each word, copy two bytes to the buffer
            wcMaxLBA[kMaxLBABufferIndex] = *(buffer + (kMaxLBACounter*2));
            kMaxLBABufferIndex++;
            wcMaxLBA[kMaxLBABufferIndex] = *(buffer + (kMaxLBACounter*2 + 1));
            kMaxLBABufferIndex++;
         }
         break;

      default:
         returnStatus = ERROR;
         break;
   } // End switch

   // Set the pointer length and then dereference.  In the future the upper
   // 32-bits may have to be used.
   gMaxNumberOfLBAsFromIDLow = wcMaxLBA[3];
   gMaxNumberOfLBAsFromIDLow <<= 8;
   gMaxNumberOfLBAsFromIDLow |= wcMaxLBA[2];
   gMaxNumberOfLBAsFromIDLow <<= 8;
   gMaxNumberOfLBAsFromIDLow |= wcMaxLBA[1];
   gMaxNumberOfLBAsFromIDLow <<= 8;
   gMaxNumberOfLBAsFromIDLow |= wcMaxLBA[0];
   gMaxNumberOfLBAsFromIDHigh = wcMaxLBA[7];
   gMaxNumberOfLBAsFromIDHigh <<= 8;
   gMaxNumberOfLBAsFromIDHigh |= wcMaxLBA[6];
   gMaxNumberOfLBAsFromIDHigh <<= 8;
   gMaxNumberOfLBAsFromIDHigh |= wcMaxLBA[5];
   gMaxNumberOfLBAsFromIDHigh <<= 8;
   gMaxNumberOfLBAsFromIDHigh |= wcMaxLBA[4];

   // ID returns max number of LBAs (starting from LBA 0), so max LBA is -1
   gMaxLBAFromIDLow  = gMaxNumberOfLBAsFromIDLow - 1;
   gMaxLBAFromIDHigh = gMaxNumberOfLBAsFromIDHigh;

   ugReturnValue1 = gMaxNumberOfLBAsFromIDLow;
   ugReturnValue2 = gMaxNumberOfLBAsFromIDHigh;
   ugReturnValue3 = gMaxLBAFromIDLow;
   ugReturnValue4 = gMaxLBAFromIDHigh;
   ukReturnValue1 = returnStatus;
   return;
} // End GetMaxLBAFromIdentifyDevice

//------------------------------------------------------------------------------
// Description: Get the maximum LBA from the LBA registers after a Read
//              Native Max EXT if 48-bit addressing is supported, or from
//              a Read Native Max if 48-bit addressing is NOT supported.
//
// Input:  None
//
// Output: ugReturnValue1 - Contains lower 32-bits ofmaximum LBA from Read
//                          Native Max Adddress (EXT)
//         ugReturnValue2 - Contains upper 32-bits ofmaximum LBA from Read
//                          Native Max Adddress (EXT)
//         ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void GetMaxLBAFromReadNativeMax()
{
   int returnStatus, k48BitAddressingFlag;
   unsigned long gMaxLBAFromReadNativeMaxHigh, gMaxLBAFromReadNativeMaxLow;

   // Check 48-bit addressing supported
   Check48BitAddressingSupported ();
   k48BitAddressingFlag = ukReturnValue1;

   // Check which LBA mode the drive supports
   switch (k48BitAddressingFlag)
   {
      case (OFF):
         // 48-bit Addressing is NOT supported

         if (ukQuietMode == OFF)
         {
            sprintf(upPrintString, "\n\nIssuing READ NATIVE MAX ADDRESS command");
            PrintString (ukPrintOutput);
         }

         // Get value from Read Native Max Address
         returnStatus = reg_non_data_lba28( ukDevicePosition, 0xF8, 0, 0, 0L );

         // Lower 32-bits of LBA Address
         gMaxLBAFromReadNativeMaxLow = reg_cmd_info.lbaLow2;

         // Upper 32-bits of LBA Address
         gMaxLBAFromReadNativeMaxHigh = reg_cmd_info.lbaHigh2;
         break;

      case (ON):
         // 48-bit Addressing is supported

         if (ukQuietMode == OFF)
         {
            sprintf(upPrintString, "\n\nIssuing READ NATIVE MAX ADDRESS EXT command");
            PrintString (ukPrintOutput);
         }

         // Use value from Read Native Max Address EXT
         returnStatus = reg_non_data_lba48 (
            ukDevicePosition, 0x27,
            0, 0,
            0L, 0L
            );

         // Lower 32-bits of LBA Address
         gMaxLBAFromReadNativeMaxLow = reg_cmd_info.lbaLow2;

         // Upper 32-bits of LBA Address
         gMaxLBAFromReadNativeMaxHigh = reg_cmd_info.lbaHigh2;
         break;

      default:
         returnStatus = ERROR;
         break;
   } // End switch

   // Current drive capacity < 2^32.  In the future the higher 32-bits
   // may have to be used as well.
   ugReturnValue1 = gMaxLBAFromReadNativeMaxLow;
   ugReturnValue2 = gMaxLBAFromReadNativeMaxHigh;
   ukReturnValue1 = returnStatus;
   return;
} // End GetMaxLBAFromReadNativeMax

//------------------------------------------------------------------------------
// Description: Get the maximum LBA from Device Configuration Identify
//              words 3-6.
//
// Input:  None
//
// Output: ugReturnValue1 - Contains lower 32 bits of maximum LBA from
//                          Device Configuration Identify
//         ugReturnValue2 - Contains upper 32 bits of maximum LBA from
//                          Device Configuration Identify
//------------------------------------------------------------------------------
void GetMaxLBAFromDCO()
{
   int kMaxLBACounter, kMaxLBABufferIndex;
   unsigned long gMaxLBAFromDCOLow, gMaxLBAFromDCOHigh;

   // Initialize the Variables
   kMaxLBABufferIndex = 0;   // Start at index 0

   // Issue DC identify command to fill buffer with DCO data
   DeviceConfigurationIdentify ();

   // Clear the buffer
   memset (wcMaxLBA, 0, sizeof (wcMaxLBA));

   // Words 3-6 contain max LBA number
   for (kMaxLBACounter = 3; kMaxLBACounter <= 6; kMaxLBACounter++)
   {
      // For each word, copy two bytes to the buffer
      wcMaxLBA[kMaxLBABufferIndex] = *(buffer + (kMaxLBACounter*2));
      kMaxLBABufferIndex++;
      wcMaxLBA[kMaxLBABufferIndex] = *(buffer + (kMaxLBACounter*2 + 1));
      kMaxLBABufferIndex++;
   }

   // Set the pointer length and then dereference.  In the future the upper
   // 32-bits may have to be used.
   gMaxLBAFromDCOLow = wcMaxLBA[3];
   gMaxLBAFromDCOLow <<= 8;
   gMaxLBAFromDCOLow |= wcMaxLBA[2];
   gMaxLBAFromDCOLow <<= 8;
   gMaxLBAFromDCOLow |= wcMaxLBA[1];
   gMaxLBAFromDCOLow <<= 8;
   gMaxLBAFromDCOLow |= wcMaxLBA[0];
   gMaxLBAFromDCOHigh = wcMaxLBA[7];
   gMaxLBAFromDCOHigh <<= 8;
   gMaxLBAFromDCOHigh |= wcMaxLBA[6];
   gMaxLBAFromDCOHigh <<= 8;
   gMaxLBAFromDCOHigh |= wcMaxLBA[5];
   gMaxLBAFromDCOHigh <<= 8;
   gMaxLBAFromDCOHigh |= wcMaxLBA[4];

   ugReturnValue1 = gMaxLBAFromDCOLow;
   ugReturnValue2 = gMaxLBAFromDCOHigh;
   return;
} // End GetMaxLBAFromDCO

//------------------------------------------------------------------------------
// Description: Checks if a host protected area has been set by comparing the
//              max LBA from identify device and the max LBA from read native
//              max.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = HPA is NOT set
//                          ON  = HPA is set
//------------------------------------------------------------------------------
void CheckHPASet()
{
   int kHPAEnabledFlag;
   unsigned long gMaxLBAFromReadNativeMaxLow, gMaxLBAFromIDLow;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get max LBA from ID
   GetMaxLBAFromIdentifyDevice();
   gMaxLBAFromIDLow = ugReturnValue3;

   // Get the max LBA from read max
   GetMaxLBAFromReadNativeMax();
   gMaxLBAFromReadNativeMaxLow = ugReturnValue1;

   // Compare only lower 32 bits
   if ( gMaxLBAFromReadNativeMaxLow == gMaxLBAFromIDLow )
   {
      kHPAEnabledFlag = OFF;
   }
   else
   {
      kHPAEnabledFlag = ON;
   }

   ukReturnValue1 = kHPAEnabledFlag;
   return;
} // End CheckHPASet

//------------------------------------------------------------------------------
// Description: Read native max address (EXT) command.
//
// Input:  kCommandType  - ON  = EXT command
//                         OFF = Non-EXT command
//
// Output: ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void ReadNativeMaxAddress( int kCommandType )
{
   int returnStatus;

   // Default is no error
   returnStatus = NO_ERROR;

   switch (kCommandType)
   {
      case LBA48_MODE:
         if (ukQuietMode == OFF)
         {
            sprintf(upPrintString, "\n\nIssuing READ NATIVE MAX ADDRESS EXT command");
            PrintString (ukPrintOutput);
         }

         returnStatus = reg_non_data_lba48 (
            ukDevicePosition, 0x27,
            0, 0,
            0L, 0L
            );
         break;

      case LBA28_MODE:
         if (ukQuietMode == OFF)
         {
            sprintf(upPrintString, "\n\nIssuing READ NATIVE MAX ADDRESS command");
            PrintString (ukPrintOutput);
         }

         returnStatus = reg_non_data_lba28 (
            ukDevicePosition, 0xF8,
            0, 0,
            0L
            );
         break;

      default:
         returnStatus = ERROR;
         break;
   } // End switch

   ukReturnValue1 = returnStatus;
   return;
} // End ReadNativeMaxAddress

//------------------------------------------------------------------------------
// Description: Set max address (EXT) command.  To be used immediately after the
//              appropiate read native max (EXT) command.
//
// Input:  kCommandType   - ON  = EXT command
//                          OFF = Non-EXT command
//         kVolatility    - 1   = Non-volatile
//                          0   = Volatile
//         gLBA           - LBA to set
//
// Output: ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void SetMaxAddress( int kCommandType, int kVolatility, unsigned long gLBA )
{
   int returnStatus;

   // Default is no error
   returnStatus = NO_ERROR;

   switch (kCommandType)
   {
      case LBA48_MODE:
         if (ukQuietMode == OFF)
         {
            sprintf( upPrintString, "\n\nIssuing SET MAX ADDRESS EXT command" );
            PrintString (ukPrintOutput);
         }

         // Set max address EXT
         returnStatus = reg_non_data_lba48 (
            ukDevicePosition, 0x37,
            0, kVolatility,
            0L, gLBA
            );
         break;

      case LBA28_MODE:
         if (ukQuietMode == OFF)
         {
            sprintf(upPrintString, "\n\nIssuing SET MAX ADDRESS command");
            PrintString (ukPrintOutput);
         }

         // Set max address
         returnStatus = reg_non_data_lba28 (
            ukDevicePosition, 0xf9,
            0, kVolatility,
            gLBA
            );
         break;

      default:
         returnStatus = ERROR;
         break;
   } // End switch

   ukReturnValue1 = returnStatus;
   return;
} // End SetMaxAddress

//------------------------------------------------------------------------------
// Description: Sets HPA, i.e. new max HPA LBA.
//
// Input:  kCommandType   - ON  = EXT command
//                          OFF = Non-EXT command
//         kVolatility    - 1   = Non-volatile
//                          0   = Volatile
//         gLBA           - LBA to set
//
// Output: ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void SetHPA( int kCommandType, int kVolatility, unsigned long gLBA )
{
   int returnStatus;

   returnStatus = ERROR;

   ReadNativeMaxAddress( kCommandType );
   returnStatus = ukReturnValue1;

   if ( returnStatus == NO_ERROR )
   {
      SetMaxAddress( kCommandType, kVolatility, gLBA );
      returnStatus = ukReturnValue1;
   }

   ukReturnValue1 = returnStatus;
   return;
}

//------------------------------------------------------------------------------
// Description: Remove the 48-bit and 28-bit Host Protected Area, if
//              either exist.  Tries to remove using volatile commands,
//              and then non-volatile.  This is because if the capacity
//              of a drive > 137GB, then a non-EXT HPA can be set, causing
//              the user to have to first reset the non-EXT HPA (to 137GB)
//              and then finally reset the EXT HPA (to true native capacity).
//              Therefore the volatile commands perform the first HPA reset,
//              whether they are necessary or not.
//              Note - This problem is fixed in all drives that support ATA-8.
//
// Input:  None
//
// Output: ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void RemoveHPA ()
{
   int returnStatus;

   // 28-bit volatile
   ReadNativeMaxAddress (LBA28_MODE);
   SetMaxAddress (LBA28_MODE, HPA_VOLATILE, reg_cmd_info.lbaLow2);

   // 48-bit volatile
   ReadNativeMaxAddress (LBA48_MODE);
   SetMaxAddress (LBA48_MODE, HPA_VOLATILE, reg_cmd_info.lbaLow2);

   // 28-bit non-volatile
   ReadNativeMaxAddress (LBA28_MODE);
   SetMaxAddress (LBA28_MODE, HPA_NON_VOLATILE, reg_cmd_info.lbaLow2);
   returnStatus = ukReturnValue1;

   // No 28-bit HPA.  Try to reset 48-bit HPA with Set Max EXT
   if (returnStatus == ERROR)
   {
      // Try to reset HPA with Set Max EXT

      // Read Native Max Address EXT
      // Non-volatile Set Max Address EXT
      ReadNativeMaxAddress (LBA48_MODE);
      SetMaxAddress (LBA48_MODE, HPA_NON_VOLATILE, reg_cmd_info.lbaLow2);
   }

   ukReturnValue1 = returnStatus;
   return;
} // End RemoveHPA

//------------------------------------------------------------------------------
// Description: Turns on support for the security mode feature set using
//              the device configuration set command.
//
// Input:  kSecurityTurnOn - ON/OFF
//
// Output: ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void ChangeSecuritySupportViaDCO (int kSecurityTurnOn)
{
   int returnStatus;
   int kCheckSumCounter;
   unsigned char cDCIDWord7, cBit3Mask, cCheckSum;

   // Initialize checksum to 0
   cCheckSum = 0;
   cBit3Mask = 0x08;

   // Put drive in DCO0: factory_config, state
   DeviceConfigurationRestore ();

   // Fill the buffer with factory default values
   DeviceConfigurationIdentify ();

   //Set security feature set support--word 7, bit 3
   if (kSecurityTurnOn == ON)
   {
      cDCIDWord7        = *(buffer + ( 2 * 7 ) );
      *(buffer + (2*7)) = (cBit3Mask | cDCIDWord7);
   }
   else
   {
      cBit3Mask         = ~cBit3Mask;
      cDCIDWord7        = *( buffer + ( 2 * 7 ) );
      *(buffer + (2*7)) = (cBit3Mask & cDCIDWord7);
   }

   // Change checksum in word 255 bits 8:15
   for (kCheckSumCounter = 0; kCheckSumCounter <= 510; kCheckSumCounter++)
   {
      cCheckSum = cCheckSum + buffer[kCheckSumCounter];
   }

   // Checksum = two's complement = one's complement + 01h
   cCheckSum = ~cCheckSum + 0x01;

   // Replace old checksum with the new checksum
   buffer[511] = cCheckSum;

   // Device configuration set command to try and support/unsupport security
   // feature set
   if (ukQuietMode == OFF)
   {
      sprintf(upPrintString, "\n\nIssuing DEVICE CONFIGURATION SET command");
      PrintString (ukPrintOutput);
   }

   returnStatus = reg_pio_data_out_lba28 (
      ukDevicePosition, 0xB1,
      0xC3, 0,
      0L,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0
      );

   ukReturnValue1 = returnStatus;
   return;
} // End TurnOnSecuritySupportViaDCO

//------------------------------------------------------------------------------
// Description: Changes the capacity of the drive using a device configuration
//              set command.  Issues a device configuration restore, erasing the
//              effects of any prior device configuration set command, before
//              the new set command.
//
// Input:  gNewCapacity - New Capacity
//
// Output: ukReturnValue1 - function status, ERROR/NO ERROR
//         ugReturnValue1 - read native max (EXT) value after device config set
//------------------------------------------------------------------------------
void ChangeDriveCapacityViaDCO (unsigned long gNewCapacity)
{
   int returnStatus, kCheckSumCounter;
   unsigned long gDefaultCapacity, gReadNativeMaxValue;
   char cCheckSum;

   // Default status is failed
   returnStatus = ERROR;
   cCheckSum = 0;

   // Get the max default capacity
   GetMaxLBAFromDCO ();
   gDefaultCapacity = ugReturnValue1;

   // New capacity can not be greater than the factory default capacity
   if ( gNewCapacity >= gDefaultCapacity ) {
      // Update error information
      returnStatus = ERROR;
      ukTotalErrors++;

      if (ukQuietMode == OFF) {
         PrintFailMessage();
         sprintf( upPrintString, "Requested Capacity: %lu >= factory default capacity: %lu", gNewCapacity, gDefaultCapacity );
         PrintString (ukPrintOutput);
      }

      // Exit the function
      return;
   }

   // Put drive in DCO0: factory_config, state
   DeviceConfigurationRestore();

   // Fill the buffer with factory default values
   DeviceConfigurationIdentify();

   // Fill words 3-4 (4 bytes) with new capacity
//   *(unsigned long *) (buffer + (2*3)) = gNewCapacity;
   buffer[ 6 ] = ( gNewCapacity & 0xFF );
   buffer[ 7 ] = ( ( gNewCapacity >> 8 ) & 0xFF );
   buffer[ 8 ] = ( ( gNewCapacity >> 16 ) & 0xFF );
   buffer[ 9 ] = ( ( gNewCapacity >> 24 ) & 0xFF );

   // Clear words 5-6 (4 bytes) to (long) zero; in the future upper 32-bits may
   // be used
   buffer[ 10 ] = 0;
   buffer[ 11 ] = 0;
   buffer[ 12 ] = 0;
   buffer[ 13 ] = 0;

   // Calculate checksum for word 255 bits 8:15
   for ( kCheckSumCounter = 0; kCheckSumCounter <= 510; kCheckSumCounter++ ) {
      cCheckSum = cCheckSum + buffer[ kCheckSumCounter ];
   }

   // Checksum = two's complement = one's complement + 01h
   cCheckSum = ~cCheckSum + 0x01;

   // Replace old checksum with the new checksum
   buffer[ 511 ] = cCheckSum;

   if ( ukQuietMode == OFF ) {
      sprintf(upPrintString, "\n\nIssuing DEVICE CONFIGURATION SET command");
      PrintString (ukPrintOutput);
   }

   // Device configuration set command
   returnStatus = reg_pio_data_out_lba28 (
      ukDevicePosition, 0xB1,
      0xC3, 0,
      0L,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0
      );

   if ( ( returnStatus == ERROR ) && ( ukQuietMode == OFF ) ) {
      ukTotalErrors++;
      sprintf( upPrintString, "\n" );
      PrintString( ukPrintOutput );
      PrintDataBufferHex( 512, PRINT_WORD );
      PrintFailMessage();
   } else {
      // Check the returned capacity from read native max (EXT) has chagned
      GetMaxLBAFromReadNativeMax();
      gReadNativeMaxValue = ugReturnValue1;

      // Compare and report the expected and actual capacity
      if ( gNewCapacity != gReadNativeMaxValue ) {
         returnStatus = ERROR;
         ukTotalErrors++;

         if ( ukQuietMode == OFF ) {
            PrintFailMessage ();

            sprintf( upPrintString, "\n\nRequested capacity = %lu", gNewCapacity);
            PrintString (ukPrintOutput);

            sprintf( upPrintString, "\n\nValue from read native max (EXT) = %lX (%lu)", gReadNativeMaxValue, gReadNativeMaxValue );
            PrintString (ukPrintOutput);
         }
      }
   }

   ukReturnValue1 = returnStatus;
   ugReturnValue1 = gReadNativeMaxValue;
   return;
} // End ChangeDriveCapacityViaDCO

//------------------------------------------------------------------------------
// Description: Issue a device configuration restore command.
//
// Input:  None
//
// Output: ukReturnValue1 - function status, ERROR/NO ERROR
//------------------------------------------------------------------------------
void DeviceConfigurationRestore ()
{
   int returnStatus;

   if ( ukQuietMode == OFF ) {
      sprintf(upPrintString, "\n\nIssuing DEVICE CONFIGURATION RESTORE command");
      PrintString (ukPrintOutput);
   }

   returnStatus = reg_non_data_lba48 (
      ukDevicePosition, 0xB1,
      0xc0, 0,
      0L, 0L
      );

   ukReturnValue1 = returnStatus;
   return;
} // End DeviceConfigurationRestore

//------------------------------------------------------------------------------
// Description: Check if drive security is enabled.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = Drive Security NOT Enabled
//                          ON  = Drive Security Enabled
//------------------------------------------------------------------------------
void CheckSecurityEnabled ()
{
   int kSecurityEnabledFlag;
   unsigned char cIDWord128Low;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get first byte of security word 128
   cIDWord128Low = *( buffer + ( 128 * 2 ) );

   // Check if drive security is enabled
   if ((cIDWord128Low & SECURITY_ENABLED) == 0) {
      kSecurityEnabledFlag = OFF;
   } else {
      kSecurityEnabledFlag = ON;
   }

   ukReturnValue1 = kSecurityEnabledFlag;
   return;
} // End CheckSecurityEnabled

//------------------------------------------------------------------------------
// Description: Check if drive is locked.
//
// Input:  None
//
// Output: ukReturnValue1 - 0 = Drive NOT Locked
//                          1 = Drive Locked
//------------------------------------------------------------------------------
void CheckSecurityLocked ()
{
   int kSecurityLockedFlag;
   unsigned char cIDWord128Low;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get first byte of security word 128
   cIDWord128Low = *( buffer + ( 128 * 2 ) );

   // Check if drive is locked
   if ((cIDWord128Low & SECURITY_LOCKED) == 0) {
      kSecurityLockedFlag = OFF;
   } else {
      kSecurityLockedFlag = ON;
   }

   ukReturnValue1 = kSecurityLockedFlag;
   return;
} // End CheckSecurityLocked

//------------------------------------------------------------------------------
// Description: Checks if drive is frozen.
//
// Input:  None
//
// Output: ukReturnValue1 - OFF = Drive NOT Frozen
//                          ON  = Drive Frozen
//------------------------------------------------------------------------------
void CheckSecurityFrozen ()
{
   int kSecurityFrozenFlag;
   unsigned char cIDWord128Low;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get first byte of security word 128
   cIDWord128Low = *( buffer + ( 128 * 2 ) );

   // Check if drive is frozen
   if ( ( cIDWord128Low & SECURITY_FROZEN ) == 0 ) {
      kSecurityFrozenFlag = OFF;
   } else {
      kSecurityFrozenFlag = ON;
   }

   ukReturnValue1 = kSecurityFrozenFlag;
   return;
} // End CheckSecurityFrozen

//------------------------------------------------------------------------------
// Description: Checks if drive security count is expired.
//
// Input:  None
//
// Output: ukReturnValue1 - 0 = Security Count NOT Expired
//                          1 = Security Count Expired
//------------------------------------------------------------------------------
void CheckSecurityCountExpired ()
{
   int kSecurityCountExpiredFlag;
   unsigned char cIDWord128Low;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get first byte of security word 128
   cIDWord128Low = *( buffer + ( 128 * 2 ) );

   // Check if drive security count is expired
   if ((cIDWord128Low & SECURITY_COUNT_EXPIRED) == 0) {
      kSecurityCountExpiredFlag = OFF;
   } else {
      kSecurityCountExpiredFlag = ON;
   }

   ukReturnValue1 = kSecurityCountExpiredFlag;
   return;
} // End CheckSecurityCountExpired

//------------------------------------------------------------------------------
// Description: Check drive security level.
//
// Input:  None
//
// Output: ukReturnValue1 - SECURITY_LEVEL_HIGH
//                          SECURITY_LEVEL_MAXIMUM
//------------------------------------------------------------------------------
void CheckSecurityLevel ()
{
   int kSecurityLevel;
   unsigned char cIDWord128High;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get second byte of security word 128
   cIDWord128High = *( buffer + ( 128 * 2 + 1 ) );

   // Check drive security level
   if ((cIDWord128High & SECURITY_LEVEL_MAXIMUM) == 1) {
      kSecurityLevel = SECURITY_LEVEL_MAXIMUM;
   } else {
      kSecurityLevel = SECURITY_LEVEL_HIGH;
   }

   ukReturnValue1 = kSecurityLevel;
   return;
} // End CheckSecurityLevel

//------------------------------------------------------------------------------
// Description: Unlock drive with user or master password.
//
// Input:  *wcPasswordString   - String containing the password to use
//         kPasswordType       - USER_PASSWORD   = use user password
//                               MASTER_PASSWORD = use master password
//
// Output: ukReturnValue1 - NO ERROR = Unlock successful
//                          ERROR    = Unlock unsuccessful
//         ukReturnValue2 - OFF      = Drive Unlocked
//                          ON       = Drive Locked
//------------------------------------------------------------------------------
void SecurityUnlockPassword (const char* wcPasswordString, int kPasswordType)
{
   int kSecurityLockedFlag, returnStatus;

   // Clear the buffer
   memset( buffer, 0, sizeof( buffer ) );

   if ( kPasswordType == USER_PASSWORD ) {
      *buffer = USER_PASSWORD;         // Use user Password
   } else {
      *buffer = MASTER_PASSWORD;       // Use master Password
   }

   // Copy the password to word 1 of the buffer
   strcpy( buffer + 2, wcPasswordString );

   if ( ukQuietMode == OFF ) {
      sprintf(upPrintString, "\n\nIssuing SECURITY UNLOCK command");
      PrintString (ukPrintOutput);
   }

   // issue the security unlock command
   returnStatus = reg_pio_data_out_lba28 (
      ukDevicePosition, 0xf2,
      0, 0,
      0L,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0
      );

// TEMP ----DMC
   // Check if the drive is still locked
//   CheckSecurityLocked();
//   kSecurityLockedFlag = ukReturnValue1;
kSecurityLockedFlag = OFF;

   ukReturnValue1 = returnStatus;
   ukReturnValue2 = kSecurityLockedFlag;
   return;
} // End SecurityUnlockPassword

//------------------------------------------------------------------------------
// Description: Disable drive security with user or master password.
//
// Input:  *wcPasswordString   - String containing the password to use
//         kPasswordType       - USER_PASSWORD   = use user password
//                               MASTER_PASSWORD = use master password
//
// Output: ukReturnValue1 - NO ERROR = Security disable successful
//                          ERROR    = Security disable unsuccessful
//         ukReturnValue2 - OFF      = Drive security disabled
//                          ON       = Drive security enabled
//------------------------------------------------------------------------------
void SecurityDisablePassword( const char* wcPasswordString, int kPasswordType )
{
   int kSecurityEnabledFlag, returnStatus;

   // Clear the buffer
   memset( buffer, 0, sizeof( buffer ) );

   if ( kPasswordType == USER_PASSWORD ) {
      *buffer = USER_PASSWORD;      // Use user Password
   } else {
      *buffer = MASTER_PASSWORD;    // Use master Password
   }

   // Copy the password to word 1 of the buffer
   strcpy( ( buffer + 2 ), wcPasswordString );

   if ( ukQuietMode == OFF ) {
      sprintf(upPrintString, "\n\nIssuing SECURITY DISABLE PASSWORD command");
      PrintString (ukPrintOutput);
   }

   // issue the security disable password command
   returnStatus = reg_pio_data_out_lba28(
      ukDevicePosition, 0xf6,
      0, 0,
      0L,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0
      );

// TEMP ----DMC
   // Check if the drive security is still enabled
//   CheckSecurityEnabled();
//   kSecurityEnabledFlag = ukReturnValue1;
kSecurityEnabledFlag = OFF;

   ukReturnValue1 = returnStatus;
   ukReturnValue2 = kSecurityEnabledFlag;
   return;
} // End SecurityDisablePassword

//------------------------------------------------------------------------------
// Description: Set a user or master password.
//
// Input:  *wcPasswordString    - String containing the password to use
//         kPasswordType        - Either user or master password
//         kSecurityLevel       - Either high or max
//
// Output: ukReturnValue1 - 0 = Security not enabled in ID
//                          1 = Security enabled in ID
//         ukReturnValue2 - 0 = Password set unsuccessfully
//                          1 = Password set successfully
//------------------------------------------------------------------------------
void SecuritySetPassword (const char* wcPasswordString, int kPasswordType, int kSecurityLevel)
{
   int returnStatus, kSecurityEnabledFlag;

   // Clear the buffer
   memset( buffer, 0, sizeof( buffer ) );

   if (kPasswordType == USER_PASSWORD) {
      *buffer = USER_PASSWORD;                  // Use User Password
   } else {
      *buffer = MASTER_PASSWORD;                // Use Master Password
   }

   if ( kSecurityLevel == USER_PASSWORD ) {
      *( buffer + 1 ) = SECURITY_LEVEL_HIGH;      // Set security level high
   } else {
      *( buffer + 1 ) = SECURITY_LEVEL_MAXIMUM;   // Set security level max
   }

   // Copy the password to word 1 of the buffer
   strcpy( buffer + 2, wcPasswordString );

   if ( ukQuietMode == OFF ) {
      sprintf(upPrintString, "\n\nIssuing SECURITY SET PASSWORD command");
      PrintString (ukPrintOutput);
   }

   // Issue the security set password command
   returnStatus = reg_pio_data_out_lba28 (
      ukDevicePosition, 0xF1,
      0, 0,
      0L,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0
      );

   // Check if the drive security is enabled in ID
   CheckSecurityEnabled();
   kSecurityEnabledFlag = ukReturnValue1;

   ukReturnValue1 = kSecurityEnabledFlag;
   ukReturnValue2 = returnStatus;
   return;
} // End SecuritySetPassword

//------------------------------------------------------------------------------
// Description: Perform a security erase prepare, then either a normal or
//              enhanced secure erase.
//
// Input:  None
//
// Output: ukReturnValue1   - Secure erase time in minutes
//         ukReturnValue2   - Enhanced secure erase time in minutes
//------------------------------------------------------------------------------
void GetEstimatedSecureEraseTimesInMin()
{
   short int kSecureEraseTimeInMin, kEnhancedSecureEraseTimeInMin;

   // Issue Identify Device command to fill buffer with ID data
   IdentifyDevice();

   // Get secure erase completion time from word 89
   kSecureEraseTimeInMin = *(short int *)( buffer + ( 89 * 2 ) );

   // ATA-6 spec defines this value x 2 is required time in min
   kSecureEraseTimeInMin *= 2;

   // Get enhanced secure erase completion time from word 90
   kEnhancedSecureEraseTimeInMin = *(short int *)( buffer + ( 90 * 2 ) );

   // ATA-6 spec defines this value x 2 is required time in min
   kEnhancedSecureEraseTimeInMin *= 2;

   ukReturnValue1 = kSecureEraseTimeInMin;
   ukReturnValue2 = kEnhancedSecureEraseTimeInMin;
   return;
}

//------------------------------------------------------------------------------
// Description: Perform a security erase prepare, then either a normal or
//              enhanced secure erase.
//
// Input:  *wcPasswordString    - String containing the password to use
//         kPasswordType        - Either user or master password
//         kEraseType           - Either normal or enhanced
//
// Output: ukReturnValue1       - 0 = Secure Erase successful
//                                1 = Secure Erase unsuccessful
//------------------------------------------------------------------------------
void SecureErase( const char* wcPasswordString, int kPasswordType, int kEraseType )
{
   int returnStatus;
   unsigned int kWord0;

   // Clear the buffer
   memset( buffer, 0, sizeof( buffer ) );

   // Copy the erase options to word 0 of the buffer
   kWord0 =( kPasswordType | kEraseType );
   *buffer = kWord0;

   // Copy the password to word 1 of the buffer
   strcpy( ( buffer + 2 ), wcPasswordString );

   if (ukQuietMode == OFF) {
      sprintf( upPrintString, "\n\nIssuing SECURITY ERASE PREPARE command" );
      PrintString( ukPrintOutput );
   }

   // Security erase prepare
   returnStatus = reg_non_data_lba28( ukDevicePosition, 0xf3, 0, 0, 0L );

   if (ukQuietMode == OFF) {
      sprintf(upPrintString, "\n\nIssuing SECURITY ERASE command");
      PrintString (ukPrintOutput);
   }

   // Secure erase command
   returnStatus = reg_pio_data_out_lba28 (
      ukDevicePosition, 0xf4,
      0, 0,
      0L,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0
      );

   ukReturnValue1 = returnStatus;
   return;
} // End SecureErase
/*
void SetHighestUDMAMode ()
{
   while ()
   {
      SetUDMAMode ();
   }
}

void SetUDMAMode (kUDMAMode)
{
   // Issue set features command
   kUDMAMode
}
*/
//------------------------------------------------------------------------------
// Description: Write n number of sectors starting a specific CHS or LBA.
//              Three write modes, LBA28, LBA48, CHS.  Contents in array
//              "buffer" are written to the specified location.
//
// Input:  kCylinder            - Cylinder address
//         kHead                - Head address
//         kSector              - Sector address
//         gLBA                 - LBA address
//         kNumberOfSectors     - Number of sectors to write
//         kWriteMode           - The write mode
//                                 LBA48_MODE = Write sectors EXT
//                                 LBA28_MODE = Write sectors
//                                 CHS_MODE   = CHS write sectors (legacy)
//
// Output: ukReturnValue1       - NO_ERROR = Write successful
//                                ERROR    = Write unsuccessful
//------------------------------------------------------------------------------
void WriteSectors (unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gLBA, unsigned long gNumberOfSectors, int kWriteMode)
{
   int returnStatus;
   unsigned int kFeaturesRegister;
   unsigned long gSectorCountRegister;
   unsigned long gLBALow, gLBAHigh;

   // Configure input registers
   kFeaturesRegister     = IGNORE_VALUE;
   gSectorCountRegister  = gNumberOfSectors;
   gLBALow               = gLBA;
   gLBAHigh              = IGNORE_VALUE;

   if (ukQuietMode == OFF)
   {
      sprintf(upPrintString, "\n\nIssuing WRITE SECTOR(S)command");
      PrintString (ukPrintOutput);
   }

   switch (kWriteMode)
   {
      case (LBA28_MODE):
         returnStatus = reg_pio_data_out_lba28(
            ukDevicePosition, CMD_WRITE_SECTORS,
            kFeaturesRegister, gSectorCountRegister,
            gLBALow,
            FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
            gNumberOfSectors, 0
            );
         break;

      case (LBA48_MODE):
         returnStatus = reg_pio_data_out_lba48(
            ukDevicePosition, CMD_WRITE_SECTORS_EXT,
            kFeaturesRegister, gSectorCountRegister,
            gLBAHigh, gLBALow,
            FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
            gNumberOfSectors, 0
            );
         break;

      case (CHS_MODE):
         returnStatus = reg_pio_data_out_chs(
            ukDevicePosition, CMD_WRITE_SECTORS,
            kFeaturesRegister, gSectorCountRegister,
            kCylinder, kHead, kSector,
            FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
            gNumberOfSectors, 0
            );
         break;

      default:
         // Invalid selection
         returnStatus = ERROR;
         break;
   }

   ukReturnValue1 = returnStatus;
   return;
} // End WriteSectors

//------------------------------------------------------------------------------
// Description: Write n number of sectors starting a specific LBA in LBA48 mode.
//
// Input:  gLBA                 - LBA address to write
//         kNumberOfSectors     - Number of sectors to write from starting LBA
//
// Output: ukReturnValue1       - NO_ERROR = Write successful
//                                ERROR    = Write unsuccessful
//------------------------------------------------------------------------------
void WriteSectorsInLBA48 (unsigned long gLBA, unsigned long gNumberOfSectors)
{
   int returnStatus;
   int kCylinder, kHead, kSector, kWriteMode;

   // Configure the input parameters
   kCylinder  = IGNORE_VALUE;
   kHead      = IGNORE_VALUE;
   kSector    = IGNORE_VALUE;
   kWriteMode = LBA48_MODE;

   // Issue a write sectors EXT
   WriteSectors (kCylinder, kHead, kSector, gLBA, gNumberOfSectors, kWriteMode);
   returnStatus = ukReturnValue1;

   ukReturnValue1 = returnStatus;
   return;
} // End WriteSectorsInLBA48

//------------------------------------------------------------------------------
// Description: Write n number of sectors starting a specific LBA in LBA28 mode.
//
// Input:  gLBA                 - LBA address to write
//         kNumberOfSectors     - Number of sectors to write from starting LBA
//
// Output: ukReturnValue1       - NO_ERROR = Write successful
//                                ERROR    = Write unsuccessful
//------------------------------------------------------------------------------
void WriteSectorsInLBA28 (unsigned long gLBA, unsigned long gNumberOfSectors)
{
   int returnStatus;
   int kCylinder, kHead, kSector, kWriteMode;

   // Configure the input parameters
   kCylinder  = IGNORE_VALUE;
   kHead      = IGNORE_VALUE;
   kSector    = IGNORE_VALUE;
   kWriteMode = LBA28_MODE;

   // Issue a write sectors
   WriteSectors (kCylinder, kHead, kSector, gLBA, gNumberOfSectors, kWriteMode);
   returnStatus = ukReturnValue1;

   ukReturnValue1 = returnStatus;
   return;
} // End WriteSectorsInLBA28

//------------------------------------------------------------------------------
// Description: Write n number of sectors starting a specific CHS in CHS mode.
//
// Input:  kCylinder            - Cylinder address
//         kHead                - Head address
//         kSector              - Sector address
//         kNumberOfSectors     - Number of sectors to write
//
// Output: ukReturnValue1       - NO_ERROR = Write successful
//                                ERROR    = Write unsuccessful
//------------------------------------------------------------------------------
void WriteSectorsInCHS (unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gNumberOfSectors)
{
   int returnStatus, kWriteMode;
   unsigned long gLBA;

   // Configure the input parameters
   gLBA       = IGNORE_VALUE;
   kWriteMode = CHS_MODE;

   // Issue a write sectors in CHS mode
   WriteSectors (kCylinder, kHead, kSector, gLBA, gNumberOfSectors, kWriteMode);
   returnStatus = ukReturnValue1;

   ukReturnValue1 = returnStatus;
   return;
} // End WriteSectorsInCHS

//------------------------------------------------------------------------------
// Description: Write n number of sectors starting a specific LBA using UDMA
//              transfer in 48-bit mode if supported, else 28-bit mode.  The
//              UDMA mode must be set before using this command (SetUDMAMode).
//
// Input:  gLBA                 - LBA address to write
//         kNumberOfSectors     - Number of sectors to write from starting LBA
//
// Output: ukReturnValue1       - NO_ERROR = Write successful
//                                ERROR    = Write unsuccessful
//------------------------------------------------------------------------------
void WriteDMA( unsigned long lba, unsigned long numberOfSectors )
{
   int returnStatus;
   unsigned int featuresRegister, sectorCountRegister;
   unsigned int lbaHigh;

   // Configure the registers for the write DMA command
   featuresRegister    = IGNORE_VALUE;
   sectorCountRegister = numberOfSectors;
   lbaHigh             = IGNORE_VALUE;

   if (ukQuietMode == OFF) {
      sprintf(upPrintString, "\n\nIssuing WRITE DMA EXT command");
      PrintString( ukPrintOutput );
   }

   returnStatus = SendLBA48DMACommand( CMD_WRITE_DMA_EXT, featuresRegister, sectorCountRegister, lba, lbaHigh );

   if ( returnStatus == ERROR ) {
      if ( ukQuietMode == OFF ) {
         sprintf( upPrintString, "\n\nIssuing WRITE DMA command" );
         PrintString( ukPrintOutput );
      }

      returnStatus = SendLBA28DMACommand( CMD_WRITE_DMA, featuresRegister, sectorCountRegister, lba );
   }

   ukReturnValue1 = returnStatus;
   return;
} // WriteDMA

//------------------------------------------------------------------------------
// Description: Read n number of sectors starting a specific CHS or LBA.
//              Three read modes, LBA28, LBA48, CHS.
//
// Input:  kCylinder            - Cylinder address
//         kHead                - Head address
//         kSector              - Sector address
//         gLBA                 - LBA address
//         kNumberOfSectors     - Number of sectors to read
//         kReadMode            - The read mode
//                                 LBA48_MODE = Read sectors EXT
//                                 LBA28_MODE = Read sectors
//                                 CHS_MODE   = CHS read sectors (legacy)
//
// Output: ukReturnValue1       - NO_ERROR = Read successful
//                                ERROR    = Read unsuccessful
//         *buffer              - Buffer with read data
//------------------------------------------------------------------------------
void ReadSectors (unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gLBA, unsigned long gNumberOfSectors, int kReadMode)
{
   int returnStatus;
   unsigned int kFeaturesRegister;
   unsigned long gSectorCountRegister;
   unsigned long gLBALow, gLBAHigh;

   // Configure input registers
   kFeaturesRegister     = IGNORE_VALUE;
   gSectorCountRegister  = gNumberOfSectors;
   gLBALow               = gLBA;
   gLBAHigh              = IGNORE_VALUE;

   if (ukQuietMode == OFF)
   {
      sprintf(upPrintString, "\n\nIssuing READ SECTOR(S)command");
      PrintString (ukPrintOutput);
   }

   switch (kReadMode)
   {
      case (LBA28_MODE):
         returnStatus = reg_pio_data_in_lba28(
            ukDevicePosition, CMD_READ_SECTORS,
            kFeaturesRegister, gSectorCountRegister,
            gLBALow,
            FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
            gNumberOfSectors, 0
            );
         break;

      case (LBA48_MODE):
         returnStatus = reg_pio_data_in_lba48(
            ukDevicePosition, CMD_READ_SECTORS_EXT,
            kFeaturesRegister, gSectorCountRegister,
            gLBAHigh, gLBALow,
            FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
            gNumberOfSectors, 0
            );
         break;

      case (CHS_MODE):
         returnStatus = reg_pio_data_in_chs(
            ukDevicePosition, CMD_READ_SECTORS,
            kFeaturesRegister, gSectorCountRegister,
            kCylinder, kHead, kSector,
            FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
            gNumberOfSectors, 0
            );
         break;

      default:
         // Invalid Selection
         returnStatus = ERROR;
         break;
   }

   ukReturnValue1 = returnStatus;
   return;
} // End ReadSectors

//------------------------------------------------------------------------------
// Description: read n number of sectors starting a specific LBA in LBA48 mode.
//
// Input:  gLBA                 - LBA address to read
//         kNumberOfSectors     - Number of sectors to read from starting LBA
//
// Output: ukReturnValue1       - NO_ERROR = read successful
//                                ERROR    = read unsuccessful
//------------------------------------------------------------------------------
void ReadSectorsInLBA48 (unsigned long gLBA, unsigned long gNumberOfSectors)
{
   int returnStatus;
   int kCylinder, kHead, kSector, kReadMode;

   // Configure the input parameters
   kCylinder  = IGNORE_VALUE;
   kHead      = IGNORE_VALUE;
   kSector    = IGNORE_VALUE;
   kReadMode  = LBA48_MODE;

   // Issue a read sectors EXT
   ReadSectors (kCylinder, kHead, kSector, gLBA, gNumberOfSectors, kReadMode);
   returnStatus = ukReturnValue1;

   ukReturnValue1 = returnStatus;
   return;
} // End ReadSectorsInLBA48

//------------------------------------------------------------------------------
// Description: Read n number of sectors starting a specific LBA in LBA28 mode.
//
// Input:  gLBA                 - LBA address to read
//         kNumberOfSectors     - Number of sectors to read from starting LBA
//
// Output: ukReturnValue1       - NO_ERROR = Read successful
//                                ERROR    = Read unsuccessful
//------------------------------------------------------------------------------
void ReadSectorsInLBA28 (unsigned long gLBA, unsigned long gNumberOfSectors)
{
   int returnStatus;
   int kCylinder, kHead, kSector, kReadMode;

   // Configure the input parameters
   kCylinder  = IGNORE_VALUE;
   kHead      = IGNORE_VALUE;
   kSector    = IGNORE_VALUE;
   kReadMode  = LBA28_MODE;

   // Issue a read sectors
   ReadSectors (kCylinder, kHead, kSector, gLBA, gNumberOfSectors, kReadMode);
   returnStatus = ukReturnValue1;

   ukReturnValue1 = returnStatus;
   return;
} // End ReadSectorsInLBA28

//------------------------------------------------------------------------------
// Description: Read n number of sectors starting a specific CHS in CHS mode.
//
// Input:  kCylinder            - Cylinder address
//         kHead                - Head address
//         kSector              - Sector address
//         kNumberOfSectors     - Number of sectors to read
//
// Output: ukReturnValue1       - NO_ERROR = Read successful
//                                ERROR    = Read unsuccessful
//------------------------------------------------------------------------------
void ReadSectorsInCHS (unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gNumberOfSectors)
{
   int returnStatus, kReadMode;
   unsigned long gLBA;

   // Configure the input parameters
   gLBA       = IGNORE_VALUE;
   kReadMode  = CHS_MODE;

   // Issue a read sectors in CHS mode
   ReadSectors (kCylinder, kHead, kSector, gLBA, gNumberOfSectors, kReadMode);
   returnStatus = ukReturnValue1;

   ukReturnValue1 = returnStatus;
   return;
} // End ReadSectorsInCHS

//------------------------------------------------------------------------------
// Description: Read n number of sectors starting a specific LBA using UDMA
//              transfer in 48-bit mode if supported, else 28-bit mode.  The
//              UDMA mode must be set before using this command (SetUDMAMode).
//
// Input:  gLBA                 - LBA address to read
//         kNumberOfSectors     - Number of sectors to read from starting LBA
//
// Output: ukReturnValue1       - NO_ERROR = Read successful
//                                ERROR    = Read unsuccessful
//------------------------------------------------------------------------------
void ReadDMA( unsigned long lba, unsigned long numberOfSectors )
{
   int returnStatus;
   unsigned int featuresRegister, sectorCountRegister;
   unsigned long lbaHigh;

   // Configure the registers for the read DMA command
   featuresRegister    = IGNORE_VALUE;
   sectorCountRegister = numberOfSectors;
   lbaHigh             = IGNORE_VALUE;

   if ( ukQuietMode == OFF ) {
      sprintf( upPrintString, "\n\nIssuing READ DMA EXT command" );
      PrintString( ukPrintOutput );
   }

   returnStatus = SendLBA48DMACommand( CMD_READ_DMA_EXT, featuresRegister, sectorCountRegister, lba, lbaHigh );

   if ( returnStatus == ERROR ) {
      if ( ukQuietMode == OFF ) {
         sprintf( upPrintString, "\n\nIssuing READ DMA command" );
         PrintString( ukPrintOutput );
      }

      returnStatus = SendLBA28DMACommand( CMD_READ_DMA, featuresRegister, sectorCountRegister, lba );
   }

   ukReturnValue1 = returnStatus;
   return;
} // End ReadDMAEXT

//------------------------------------------------------------------------------
// Description: Issue a software reset command.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void SoftwareReset()
{
   int returnStatus;

   if (ukQuietMode == OFF)
   {
      sprintf(upPrintString, "\n\nIssuing SOFTWARE RESET command");
      PrintString (ukPrintOutput);
   }

   // Issue a soft reset (SRST) command
   returnStatus = reg_reset( 0, ukDevicePosition );

   ukReturnValue1 = returnStatus;
} // End SoftwareReset

//------------------------------------------------------------------------------
// Description: Issue a READ LOG EXT command to the given log address with
//              a given transfer length.
//
// Input:  gLogAddress                - The log address to read
//         gTransferLengthInSectors   - Number of sectors to read
//
// Output: ukReturnValue1             - NO_ERROR = Command successful
//                                      ERROR    = Command failed
//         *buffer                    - buffer with the log data
//------------------------------------------------------------------------------
void ReadLogEXT (unsigned long gLogAddress, unsigned long gTransferLengthInSectors)
{
   int returnStatus;
   unsigned int kFeaturesRegister;
   unsigned long gSectorCountRegister, gLBAHigh, gLBALow, gNumberOfSectors;

   // Set up the registers before sending the command
   kFeaturesRegister    = IGNORE_VALUE;
   gSectorCountRegister = gTransferLengthInSectors;   // Transfer length in sectors
   gLBAHigh             = 0;                          // Byte offset 0
   gLBALow              = gLogAddress;                // Log address
   gNumberOfSectors     = gTransferLengthInSectors;   // Transfer length in sectors

   if (ukQuietMode == OFF)
   {
      sprintf(upPrintString, "\n\nIssuing READ LOG EXT command");
      PrintString (ukPrintOutput);
   }

   // Read log EXT in LBA 48 mode
   returnStatus = reg_pio_data_in_lba48(
      ukDevicePosition, 0x2F,
      kFeaturesRegister, gSectorCountRegister,
      gLBAHigh, gLBALow,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      gNumberOfSectors, 0
      );

   ukReturnValue1 = returnStatus;
} // End ReadLogEXT

//------------------------------------------------------------------------------
// Description: Issue a WRITE LOG EXT command to the given log address with a
//              given transfer length and write pattern in *buffer.
//
// Input:  gLogAddress                - The log address to write
//         gTransferLengthInSectors   - Number of sectors to write
//         *buffer                    - Buffer with the pattern to be written
//
// Output: ukReturnValue1             - NO_ERROR = Command successful
//                                      ERROR    = Command failed
//------------------------------------------------------------------------------
void WriteLogEXT (unsigned long gLogAddress, unsigned long gTransferLengthInSectors)
{
   int returnStatus;
   unsigned int kFeaturesRegister;
   unsigned long gSectorCountRegister, gLBAHigh, gLBALow, gNumberOfSectors;

   // Set up the registers before sending the command
   kFeaturesRegister    = IGNORE_VALUE;
   gSectorCountRegister = gTransferLengthInSectors;   // Transfer length in sectors
   gLBAHigh             = 0;                          // Byte offset 0
   gLBALow              = gLogAddress;                // Log address
   gNumberOfSectors     = gTransferLengthInSectors;   // Transfer length in sectors

   if (ukQuietMode == OFF)
   {
      sprintf(upPrintString, "\n\nIssuing WRITE LOG EXT command");
      PrintString (ukPrintOutput);
   }

   // Write log EXT in LBA 48 mode
   returnStatus = reg_pio_data_out_lba48(
      ukDevicePosition, 0x3F,
      kFeaturesRegister, gSectorCountRegister,
      gLBAHigh, gLBALow,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      gNumberOfSectors, 0
      );

   ukReturnValue1 = returnStatus;
} // End WriteLogEXT

//------------------------------------------------------------------------------
// Description: Checks the alternate status and command registers from the last
//              ATA command.
//
// Input:  expectedStatus       - Expected status register value
//         expectedError        - Expected error register value
//
// Output: ukReturnValue1        - ERROR    = Miscompare
//                                 NO_ERROR = No miscompare
//------------------------------------------------------------------------------
void CheckStatusAndErrorRegisters( char expectedStatus, char expectedError)
{
   int returnStatus;

   // Default is no error
   returnStatus = NO_ERROR;

   // Compare expected status and error registers with those returned from drive
   if ( (expectedStatus != reg_cmd_info.as2) || (expectedError != reg_cmd_info.er2) )
   {
      // Update the error counter
      returnStatus = ERROR;
      ukTotalErrors++;

      // Option: Print detailed error message
      if (ukQuietMode == OFF)
      {
         // Print error counter
         PrintFailMessage ();

         // Print expected stat&err registers
         sprintf(upPrintString, "\n   Expected Status and Error Regs: %02X%02Xh",
                  expectedStatus, expectedError
                  );
         PrintString (ukPrintOutput);

         // Print actual stat&err registers
         PrintStatusAndErrorRegisters ();
      }
   }

   ukReturnValue1 = returnStatus;
} // End CheckStatusAndErrorRegisters

//------------------------------------------------------------------------------
// Description: Primary ATALIB.c print function.  Responsible for all printf and
//              fprintf statements.  Prints the contents of upPrintString to the
//              screen, log (Log.txt), or both.  This will print to the next
//              immediate character, not on a new line.  The first 80 characters
//              (1 printf line in DOS) in upPrintString are cleared after this
//              fucntion is called.
//
//        Note: DO NOT create a string longer than 80 characters (1 printf line
//              in DOS).  Any string longer than 80 characters will not be
//              cleared and will remain in upPrintString for the duration of the
//              program.
//
// Input:  kPrintType   - PRINT_SCREEN         = print to stdout
//                        PRINT_LOG            = print to Log.txt
//                        PRINT_SCREEN_AND_LOG = print to stdout and Log.txt
// Output: None
//------------------------------------------------------------------------------
void PrintString (int kPrintType)
{
   switch (kPrintType)
   {
      case (PRINT_SCREEN):
         printf ("%s", upPrintString);
         break;

      case (PRINT_LOG):
         upLog = fopen ("Log.txt", "a");
         fprintf (upLog, "%s", upPrintString);
         fclose (upLog);
         break;

      case (PRINT_SCREEN_AND_LOG):
         printf ("%s", upPrintString);
         upLog = fopen ("Log.txt", "a");
         fprintf (upLog, "%s", upPrintString);
         fclose (upLog);
         break;

      default:
         printf ("\n\nInvalid print type!!!");
         upLog = fopen ("Log.txt", "a");
         fprintf (upLog, "\n\nInvalid print type!!!");
         fclose (upLog);
         break;
   }

   // Clear buffer after print so no residual stuff is printed
   memset( wcPrintBuffer, 0, sizeof( wcPrintBuffer ) );

   return;
} // End PrintString

//------------------------------------------------------------------------------
// Description: Print a fail message along with counter.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void PrintFailMessage()
{
   // DOS window is 80 characters across, this print line is 78 characters across
   sprintf(upPrintString,
            "\n\n......................................................................Fail [%d]",
            ukTotalErrors
            );

   // Print the fail message
   PrintString (ukPrintOutput);

   return;
} // End PrintFailMessage

//------------------------------------------------------------------------------
// Description: Print the current value in the error and alternate status
//              registers.  Prints values on a new line.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void PrintStatusAndErrorRegisters()
{
   // Print actual stat&err registers
   sprintf( upPrintString, "\n   Actual Status and Error Regs  : %02X%02Xh",
            reg_cmd_info.as2, reg_cmd_info.er2 );
   PrintString( ukPrintOutput );

   return;
} // End PrintFailMessage

//------------------------------------------------------------------------------
// Description: Prints the number of specified bytes from the specified buffer
//              in bytes or 16-bit words.
//
// Input:  pInBuffer       - pointer to buffer with data
//         numberOfBytes   - number of bytes to print
//         printType       - PRINT_BYTE (XX XX XX), PRINT_WORD (XXXX XXXX XXXX)
//
// Output: None
//------------------------------------------------------------------------------
void PrintBuffer( void* pInBuffer, int numberOfBytes, int printType )
{
   char* pBuffer = NULL;
   int byte, word;

   // Check for valid parameters
   if ( pInBuffer == NULL ) { printf( "ERROR: pInBuffer is NULL!!!" ); return; }
   if ( numberOfBytes <= 0 ) { printf( "ERROR: number of bytes entered = %d", numberOfBytes ); return; }

   pBuffer = (char *)pInBuffer;

   switch ( printType )
   {
      case PRINT_BYTE:

         printf( "000h: %02X ", *pBuffer );

         for ( byte = 1; byte < numberOfBytes; byte++ )
         {
            // 2x + ( x - 1 ) <= ( 80 - 8 ); x <= 24
            if ( ( byte % 24 ) == 0 ) {
               printf( "\n%04hX: ", byte );
            }

            printf( "%02X ", *( pBuffer + byte ) );
         }
         break;

      case PRINT_WORD:

         printf( "%02X", *( pBuffer + 1 ) );
         printf( "%02X", *( pBuffer + 0 ) );

         for ( word = 1; word < ( numberOfBytes / 2 ); word++ )
         {
            // 4x + ( x - 1 ) <= 80; x <= 16
            if ( ( word % 16 ) == 0 ) {
               printf( "\n" );
            } else {
               printf( " " );
            }

            printf( "%02X", *( pBuffer + ( word * 2 ) + 1 ) );
            printf( "%02X", *( pBuffer + ( word * 2 ) + 0 ) );
         }

         // Print remainder
         if ( ( numberOfBytes % 2 ) == 1 ) {
            printf( " 00%02X", *( pBuffer + numberOfBytes - 1 ) );
         }
         break;

      default:
         printf( "ERROR: Invalid print parameter!!!\n" );
         break;
   }
}

//------------------------------------------------------------------------------
// Description: Prints data from default data buffer.
//
// Input:  numberOfBytes   - number of bytes to print
//         printType       - PRINT_BYTE (XX XX XX), PRINT_WORD (XXXX XXXX XXXX)
//
// Output: None
//------------------------------------------------------------------------------
void PrintDataBufferHex( int numberOfBytes, int printType )
{
   PrintBuffer( buffer, numberOfBytes, printType );
}

//------------------------------------------------------------------------------
// Description: Get 16-bits of data from ID data.
//
// Input:  pIDBuffer          - GET_ID_DATA or pointer to buffer with ID data
//         byteOffset         - byte offset into data
//
// Output: 16-bit word data
//------------------------------------------------------------------------------
int GetIDWord( char* pIDBuffer, unsigned int byteOffset )
{
   int word;

   if ( pIDBuffer == GET_ID_DATA ) {
      IdentifyDevice();
   }

   //word = *( (short *)buffer + byteOffset );
   word = ( ( *( buffer + byteOffset + 1 ) << 8 ) | ( *( buffer + byteOffset ) ) );

   return ( word );
}

//------------------------------------------------------------------------------
// Description: Get the current value in ID word 128.
//
// Input:  None
//
// Output: 16-bits from ID word 128
//------------------------------------------------------------------------------
int GetDriveSecurityState()
{
   // Get security word 128
   return ( GetIDWord( GET_ID_DATA, ( 128 * 2 ) ) );
} // End GetDriveSecurityState

//------------------------------------------------------------------------------
// Description: Print the current value in ID word 128 along with a description
//              of the security state.  Prints values on a new line.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void PrintDriveSecurityState()
{
   int kIDWord128;

   kIDWord128 = GetDriveSecurityState();

   if ( ukQuietMode == OFF )
   {
      sprintf( upPrintString, "\n   ID word 128 = %04Xh: ", kIDWord128 );
      PrintString( ukPrintOutput );

      switch ( ( kIDWord128 & 0xFF ) )
      {
         case (SECURITY_USUPPORTED):
            upPrintString = "Security not supported";
            break;

         case (SECURITY_SUPPORTED):
            upPrintString = "SEC1:  Security supported and disabled";
            break;

         case (SECURITY_SUPPORTED | SECURITY_FROZEN):
            upPrintString = "SEC2:  Security disabled and frozen";
            break;

         case (SECURITY_SUPPORTED | SECURITY_ENABLED | SECURITY_LOCKED):
            upPrintString = "SEC4:  Security locked and enabled";
            break;

         case (SECURITY_SUPPORTED | SECURITY_ENABLED):
            upPrintString = "SEC5:  Security unlocked and not frozen";
            break;

         case (SECURITY_SUPPORTED | SECURITY_ENABLED | SECURITY_FROZEN):
            upPrintString = "SEC6:  Security unlocked and frozen";
            break;

         default:
            upPrintString = "Unknown security state...";
            break;
      }

      // Print the security state description
      PrintString( ukPrintOutput );

      // Check drive security level
      if ( ( kIDWord128 >> 8 ) & SECURITY_LEVEL_MAXIMUM )
      {
         upPrintString = ", level MAX";
      }
      else
      {
         upPrintString = ", level HIGH";
      }
      // Print the security level description
      PrintString( ukPrintOutput );

   } // End if QuietMode

   return;
} // End PrintDriveSecurityState

//------------------------------------------------------------------------------
// Description: Print a fail message along with counter.
//
// Input:  kErrorFlag   - ERROR      = inc. error counter, print error info.
//                        <all else> = do nothing
//
// Output: None
//------------------------------------------------------------------------------
void HandleError( int kErrorFlag )
{
   if ( kErrorFlag == ERROR )
   {
      // Increment error counter
      ukTotalErrors++;

      if ( ukQuietMode == OFF )
      {
         // Print Fail message
         PrintFailMessage ();

         // Output status and error registers
         PrintStatusAndErrorRegisters ();

         // Issue a soft reset to clear any strange drive states, e.g. 5800h,
         // before issuing the next ATA command
         SoftwareReset ();

         // Print security word and state
         PrintDriveSecurityState ();
      }
   }

   return;
} // End HandleError

//------------------------------------------------------------------------------
// Description: Sends an ATA command with the given input register values.
//              Function will decide which data protocol to use based on the
//              specified command register value.
//
// Input:  pAtaRegs     - pointer to array of long integers containing the
//                        input command parameters
//
// Output: None, scan output registers for command success/error
//------------------------------------------------------------------------------
void SendATACommand( long int* pAtaRegs )
{
   int cmd;
   long int lbaLow, lbaHigh;
   unsigned int feat, secCnt, secNum, cylLow, cylHigh, devHead, devCtrl;
   unsigned int cylinder, head;

   if ( pAtaRegs == NULL ) { printf( "ERROR: pAtaRegs is NULL!" ); return; };

   // Shorthand for registers
   cmd     = *( pAtaRegs + CMD_REG );
   feat    = *( pAtaRegs + FEAT_REG );
   secCnt  = *( pAtaRegs + SECC_REG );
   secNum  = *( pAtaRegs + SECN_REG );
   cylLow  = *( pAtaRegs + CYLL_REG );
   cylHigh = *( pAtaRegs + CYLH_REG );
   devHead = *( pAtaRegs + DEVH_REG );       // This register's data isn't currently in use by this library
   devCtrl = *( pAtaRegs + DEVC_REG );       // This register's data isn't currently in use by this library
   lbaLow  = *( pAtaRegs + LBA_LOW );
   lbaHigh = *( pAtaRegs + LBA_HIGH );

   // Shorthand for CHS/non-data commands
   cylinder = ( ( ( cylHigh & 0xFF ) << 8 ) | ( cylLow & 0xFF ) );
   head = ( devHead & 0x0F );                // This register's data isn't currently in use by this library

/*// FOR DEBUG
   printf( "\n" );
   printf( "cmd:     %02Xh\n", pAtaRegs[ CMD_REG ] );
   printf( "feat:    %02Xh\n", pAtaRegs[ FEAT_REG ] );
   printf( "secCnt:  %02Xh\n", pAtaRegs[ SECC_REG ] );
   printf( "secNum:  %02Xh\n", pAtaRegs[ SECN_REG ] );
   printf( "cylLow:  %02Xh\n", pAtaRegs[ CYLL_REG ] );
   printf( "cylHi:   %02Xh\n", pAtaRegs[ CYLH_REG ] );
   printf( "devHead: %02Xh\n", pAtaRegs[ DEVH_REG ] );
   printf( "devCtrl: %02Xh\n", pAtaRegs[ DEVC_REG ] );
   printf( "LBALow:  %08Xh\n", pAtaRegs[ LBA_LOW ] );
   printf( "LBAHi:   %08Xh\n", pAtaRegs[ LBA_HIGH ] );
*/
   switch ( cmd )
   {
      // *** This case must be at the top because command
      // *** contains sub-commands with different handling
      case CMD_DEVICE_CONFIGURATION:
      {
         if ( ( feat == DEV_CONFIG_RESTORE ) || ( feat == DEV_CONFIG_FREEZE_LOCK ) )
         {
            // Handle as non-data command
            SendNonDataCommand( cmd, feat, secCnt, cylinder, head, secNum );
         }
         else if ( feat == DEV_CONFIG_IDENTIFY )
         {
            // Handle as PIO data-in command
            SendLBA28DataInCommand( cmd, feat, secCnt, lbaLow );
         }
         else if ( feat == DEV_CONFIG_SET )
         {
            // Handle as PIO data-out command
            SendLBA28DataOutCommand( cmd, feat, secCnt, lbaLow );
         }
         else
         {
            // Undefined sub-command -- handle as non-data by default
            SendNonDataCommand( cmd, feat, secCnt, cylinder, head, secNum );
         }

         break;
      }

      // *** This case must be at the top because command
      // *** contains sub-commands with different handling
      case CMD_SMART:
      {
         if ( ( feat == SMART_ENABLE_DISABLE_AUTOSAVE ) || ( feat == SMART_OFFLINE_IMMEDIATE ) || ( feat == SMART_ENABLE_OPERATIONS ) || ( feat == SMART_DISABLE_OPERATIONS ) || ( feat == SMART_RETURN_STATUS ) )
         {
            // Handle as non-data command
            SendNonDataCommand( cmd, feat, secCnt, cylinder, head, secNum );
         }
         else if ( ( feat == SMART_READ_DATA ) || ( feat == SMART_READ_LOG ) )
         {
            // Handle as PIO data-in command
            SendLBA28DataInCommand( cmd, feat, secCnt, lbaLow );
         }
         else if ( feat == SMART_WRITE_LOG )
         {
            // Handle as PIO data-out command
            SendLBA28DataOutCommand( cmd, feat, secCnt, lbaLow );
         }
         else
         {
            // Undefined sub-command -- handle as non-data by default
            SendNonDataCommand( cmd, feat, secCnt, cylinder, head, secNum );
         }

         break;
      }

      case CMD_SECURITY_ERASE_PREPARE:
      case CMD_CHECK_POWER_MODE1:
      case CMD_CHECK_POWER_MODE2:
      case CMD_DEVICE_RESET:
      case CMD_EXECUTE_DEVICE_DIAGNOSTIC:
      case CMD_FLUSH_CACHE:
      case CMD_FLUSH_CACHE_EXT:
      case CMD_IDLE1:
      case CMD_IDLE2:
      case CMD_IDLE_IMMEDIATE1:
      case CMD_IDLE_IMMEDIATE2:
      case CMD_INITIALIZE_DEVICE_PARAMETERS:
      case CMD_NOP:
      case CMD_RECALIBRATE:
      case CMD_SEEK:
      case CMD_SET_FEATURES:
      case CMD_SET_MULTIPLE_MODE:
      case CMD_SLEEP1:
      case CMD_SLEEP2:
      case CMD_STANDBY1:
      case CMD_STANDBY2:
      case CMD_STANDBY_IMMEDIATE1:
      case CMD_STANDBY_IMMEDIATE2:
      {
         // -----------------------------------------------------------------
         // Non-data commands
         // -----------------------------------------------------------------

         SendNonDataCommand( cmd, feat, secCnt, cylinder, head, secNum );
         break;
      }

      case CMD_IDENTIFY_DEVICE:
      case CMD_READ_BUFFER:
      case CMD_READ_MULTIPLE:
      case CMD_READ_SECTORS:
      case CMD_READ_VERIFY_SECTORS:
      {
         // -----------------------------------------------------------------
         // PIO data-in non-EXT commands
         // -----------------------------------------------------------------

         SendLBA28DataInCommand( cmd, feat, secCnt, lbaLow );
         break;
      }

      case CMD_READ_DMA_EXT:
      case CMD_READ_DMA:
      {
         // -----------------------------------------------------------------
         // DMA (PCI or ISA) read commands
         // -----------------------------------------------------------------

         memset( buffer, 0, sizeof( buffer ) );

         if ( cmd == CMD_READ_DMA_EXT ) {
            SendLBA48DMACommand( cmd, feat, secCnt, lbaLow, lbaHigh );
         } else {
            SendLBA28DMACommand( cmd, feat, secCnt, lbaLow );
         }

         break;
      }

      case CMD_WRITE_DMA:
      case CMD_WRITE_DMA_EXT:
      case CMD_WRITE_DMA_FUA_EXT:
      {
         // -----------------------------------------------------------------
         // DMA (PCI or ISA) write commands
         // -----------------------------------------------------------------

         if ( cmd == CMD_WRITE_DMA_EXT ) {
            SendLBA48DMACommand( cmd, feat, secCnt, lbaLow, lbaHigh );
         } else {
            SendLBA28DMACommand( cmd, feat, secCnt, lbaLow );
         }

         break;
      }

      case CMD_READ_LOG_EXT:
      case CMD_READ_MULTIPLE_EXT:
      case CMD_READ_SECTORS_EXT:
      case CMD_READ_VERIFY_SECTORS_EXT:
      {
         // -----------------------------------------------------------------
         // PIO data-in EXT commands
         // -----------------------------------------------------------------

         SendLBA48DataInCommand( cmd, feat, secCnt, lbaLow, lbaHigh );
         break;
      }

      case CMD_WRITE_SECTORS:
      case CMD_WRITE_BUFFER:
      case CMD_WRITE_MULTIPLE:
      case CMD_WRITE_VERIFY:
      {
         // -----------------------------------------------------------------
         // PIO data-out non-EXT commands
         // -----------------------------------------------------------------

         SendLBA28DataOutCommand( cmd, feat, secCnt, lbaLow );
         break;
      }

      case CMD_WRITE_LOG_EXT:
      case CMD_WRITE_MULTIPLE_EXT:
      case CMD_WRITE_MULTIPLE_FUA_EXT:
      case CMD_WRITE_SECTORS_EXT:
      {
         // -----------------------------------------------------------------
         // PIO data-out EXT commands
         // -----------------------------------------------------------------

         SendLBA48DataOutCommand( cmd, feat, secCnt, lbaLow, lbaHigh );
         break;
      }

      case CMD_SECURITY_ERASE_UNIT:
      {
         reg_pio_data_out_lba28( ukDevicePosition, cmd, feat, secCnt, secCnt,
                                 FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ), 1, ukMulti );
      }

      default:
      {
         // -----------------------------------------------------------------
         // Invalid or unsupported command -- handle as non-data command
         // -----------------------------------------------------------------

         SendNonDataCommand( cmd, feat, secCnt, cylinder, head, secNum );
         break;
      }
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Enable interrupt mode for the device.
//
// Input:  None
//
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int EnableInterrupt()
{
   int error = NO_ERROR;

   if ( int_use_intr_flag != TRUE ) {
      // Only enable if not already enabled

      // All values must be valid before enabling
      if ( ( uIRQNum != INVALID_VALUE ) && ( pio_bmide_base_addr != INVALID_VALUE ) && ( pio_base_addr1 != INVALID_VALUE ) ) {
         error = int_enable_irq( UNSHARED_INTERRUPT, uIRQNum, ( pio_bmide_base_addr + 2 ), ( pio_base_addr1 + 7 ) );
      } else {
         error = ERROR;
      }
   }

   if ( ukQuietMode == OFF ) {
      if ( error == NO_ERROR ) {
         sprintf( upPrintString, "\n\nINT enabled [IRQ %d, BMIDE addr %Xh, IO addr %Xh]", uIRQNum, pio_bmide_base_addr, pio_base_addr1 );
         PrintString( ukPrintOutput );
      } else {
         sprintf( upPrintString, "\n\nERROR: INT enable failed [IRQ %d, BMIDE addr %Xh, IO addr %Xh]", uIRQNum, pio_bmide_base_addr, pio_base_addr1 );
         PrintString( ukPrintOutput );
      }
   }

   return ( error );
}

//------------------------------------------------------------------------------
// Description: Restore original interrupt handler only if ours was installed.
//              Must do this before exiting out of program!
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void DisableInterrupt()
{
   int_disable_irq();
}

//------------------------------------------------------------------------------
// Description: Enable PCI DMA for the device.
//
// Input:  None
//
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int EnablePCIDMA()
{
   int error;

   if ( dma_pci_enabled_flag != 0 ) {
      error = NO_ERROR;                                     // already enabled
   } else if ( pio_bmide_base_addr != INVALID_VALUE ) {
      error = dma_pci_config( pio_bmide_base_addr );        // not enabled, so enable
   } else {
      error = ERROR;                                        // invalid bmide address, can't enable
   }

   if ( ukQuietMode == OFF ) {
      if ( error == NO_ERROR ) {
         sprintf( upPrintString, "\n\nPCI DMA enabled" );
         PrintString( ukPrintOutput );
      } else {
         sprintf( upPrintString, "\n\nERROR: Unable to enable PCI DMA" );
         PrintString( ukPrintOutput );
      }
   }

   if ( error == NO_ERROR ) {
      if ( ( pio_base_addr1 == LEGACY_PRIMARY_BASEPORT ) || ( pio_base_addr1 == LEGACY_SECONDARY_BASEPORT ) ) {
         reg_incompat_flags &= ~REG_INCOMPAT_DMA_POLL;   // Poll alt stat reg
      } else {
         reg_incompat_flags |= REG_INCOMPAT_DMA_POLL;    // Don't poll alt stat reg
      }
   }

   return ( error );
}

//------------------------------------------------------------------------------
// Description: Enable ISA DMA for the device.
//
// Input:  None
//
// Output: NO_ERROR, ERROR
//------------------------------------------------------------------------------
int EnableISADMA()
{
   int error = ERROR;

   if ( ( pio_base_addr1 == LEGACY_PRIMARY_BASEPORT ) || ( pio_base_addr1 == LEGACY_SECONDARY_BASEPORT ) ) {
      error = dma_isa_config( 5 );
   }

   if ( ukQuietMode == OFF ) {
      if ( error == NO_ERROR ) {
         sprintf( upPrintString, "\n\nISA DMA enabled" );
         PrintString( ukPrintOutput );
      } else {
         sprintf( upPrintString, "\n\nERROR: Unable to enable ISA DMA" );
         PrintString( ukPrintOutput );
      }
   }

   return ( error );
}

//------------------------------------------------------------------------------
// Description: Scans the entire PCI bus for storage controllers, then searches
//              for ATA devices. Also searches legacy I/O baseports 170h and
//              370h.
//
// References:  http://www.versalogic.com/kb/KB.asp?KBID=1601
//              http://www.waste.org/~winkles/hardware/pci.htm
//              http://wiki.osdev.org/PCI
//
// Input:  None
//
// Output: Total number of ATA devices found.
//------------------------------------------------------------------------------
unsigned int ScanForStorageDevices()
{
   unsigned int busNum, devNum, funNum;
   unsigned int currDeviceIdx, numATADev, totalDevicesFound, devPos;
   unsigned int subClassCode, legacyChannelPriFound, legacyChannelSecFound;
   unsigned int cmdBase, ctrlBase, bmideBase, irqNum;
   long tempCommandTimeout;

   legacyChannelPriFound = FALSE;
   legacyChannelSecFound = FALSE;
   currDeviceIdx = 0;
   totalDevicesFound = 0;
   cmdBase   = 0xFFFF;
   ctrlBase  = 0xFFFF;
   bmideBase = 0xFFFF;

   // Noticed that occasionally some PCI buses take the full time-out when scanned
   // before they error. Change timer to speed up the scanning process; should only
   // need < 1 second to determine if there's an ATA device attached.
   tempCommandTimeout = tmr_get_command_timeout();
   tmr_set_command_timeout( 1L );

   // Clear all previously found devices so there's no remnant devices
   memset ( wtStorageDevices, 0, sizeof( wtStorageDevices ) );

   for ( busNum = 0; busNum < PCI_MAX_BUS_NUMBER; busNum++ )
   {
      for ( devNum = 0; devNum < PCI_MAX_DEVICE_NUMBER; devNum++ )
      {
         for ( funNum = 0; funNum < PCI_MAX_FUNCTION_NUMBER; funNum++ )
         {
            if ( PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER == GetPciClassCode( busNum, devNum, funNum ) )
            {
               // --------------------------------------------------------------
               // Storage device attached to PCI bus
               // --------------------------------------------------------------

               subClassCode = GetPciSubClassCode( busNum, devNum, funNum );

               if ( ( subClassCode != PCI_SUBCLASS_CODE_IDE_CONTROLLER ) &&
                    ( subClassCode != PCI_SUBCLASS_CODE_ATA_CONTROLLER ) &&
                    ( subClassCode != PCI_SUBCLASS_CODE_SATA_CONTROLLER ) &&
                    ( subClassCode != PCI_SUBCLASS_CODE_UNKNOWN_STORAGE_CONTROLLER ) )
               {
                  continue;
               }

               // --------------------------------------------------------------
               // Set up PCI base address registers
               // --------------------------------------------------------------

               for ( devPos = PRIMARY_CHANNEL; devPos <= SECONDARY_CHANNEL; devPos++ )
               {
                  if ( devPos == PRIMARY_CHANNEL ) {
                     cmdBase  = GetPciWord( busNum, devNum, funNum, offsetof( PCIRegisters_t, BAR0 ) );
                     ctrlBase = GetPciWord( busNum, devNum, funNum, offsetof( PCIRegisters_t, BAR1 ) );
                  } else {
                     cmdBase  = GetPciWord( busNum, devNum, funNum, offsetof( PCIRegisters_t, BAR2 ) );
                     ctrlBase = GetPciWord( busNum, devNum, funNum, offsetof( PCIRegisters_t, BAR3 ) );
                  }

                  bmideBase = GetPciWord( busNum, devNum, funNum, offsetof( PCIRegisters_t, BAR4 ) );

                  if ( ( cmdBase == 0xFFFF ) || ( ctrlBase == 0xFFFF ) || ( bmideBase == 0xFFFF ) ) {
                     continue;
                  }

                  // -DMC look into this step
                  cmdBase   &= 0xFFFE;
                  ctrlBase  &= 0xFFFE;
                  bmideBase &= 0xFFFE;

                  // -DMC look into this step
                  ctrlBase -= 4;

                  // -DMC Look into this step
                  if ( devPos == SECONDARY_CHANNEL ) {
                     bmideBase += 8;
                  }

                  // Get IRQ for PCI device
                  irqNum = GetPciWord( busNum, devNum, funNum, offsetof( PCIRegisters_t, intLine ) );
                  irqNum &= 0xFF;

                  // --------------------------------------------------------------
                  // Valid storage controller found, check for storage devices
                  // --------------------------------------------------------------

                  // Map ATA I/O regs to PCI BARs
                  pio_set_iobase_addr( cmdBase, ctrlBase, bmideBase );

                  // Scan for ATA devices
                  numATADev = reg_config();

                  if ( numATADev < 1 ) {
                     continue;
                  }

                  if ( ( reg_config_info[0] == REG_CONFIG_TYPE_ATA ) || ( reg_config_info[1] == REG_CONFIG_TYPE_ATA ) ) {
                     // ATA device FOUND on first or second postion

                     if ( cmdBase == LEGACY_PRIMARY_BASEPORT ) {
                        legacyChannelPriFound = TRUE;
                     }

                     if ( cmdBase == LEGACY_SECONDARY_BASEPORT ) {
                        legacyChannelSecFound = TRUE;
                     }

                     wtStorageDevices[ currDeviceIdx ].valid = VALID_DEVICE_ENTRY;
                     wtStorageDevices[ currDeviceIdx ].busNum = busNum;
                     wtStorageDevices[ currDeviceIdx ].devNum = devNum;
                     wtStorageDevices[ currDeviceIdx ].funNum = funNum;
                     wtStorageDevices[ currDeviceIdx ].cmdBase = cmdBase;
                     wtStorageDevices[ currDeviceIdx ].ctrlBase = ctrlBase;
                     wtStorageDevices[ currDeviceIdx ].bmideBase = bmideBase;
                     wtStorageDevices[ currDeviceIdx ].irqNum = irqNum;
                     wtStorageDevices[ currDeviceIdx ].devPos = devPos;
                     wtStorageDevices[ currDeviceIdx ].masterSlave = ( reg_config_info[0] == REG_CONFIG_TYPE_ATA ) ? MASTER : SLAVE;
                     wtStorageDevices[ currDeviceIdx ].regInfo0 = reg_config_info[0];
                     wtStorageDevices[ currDeviceIdx ].regInfo1 = reg_config_info[1];                     
                     currDeviceIdx = ( ( currDeviceIdx + 1 ) % MAX_STORAGE_DEVICES );
                     totalDevicesFound++;
                  }
               } // device position (BAR0-1 or BAR2-3)
            } // if mass storage class
         } // for fun
      } // for dev
   } // for bus

   // --------------------------------------------------------------------------
   // Legacy IO ports: scan primary channel
   // --------------------------------------------------------------------------

   if ( legacyChannelPriFound == FALSE ) {
      cmdBase   = LEGACY_PRIMARY_BASEPORT;
      ctrlBase  = 0x3F0;
      bmideBase = IGNORE_VALUE;
      irqNum    = 14;

      // Map ATA I/O regs to I/O ports
      pio_set_iobase_addr( cmdBase, ctrlBase, bmideBase );

      // Scan for ATA devices
      reg_config();

      if ( ( reg_config_info[0] == REG_CONFIG_TYPE_ATA ) || ( reg_config_info[1] == REG_CONFIG_TYPE_ATA ) ) {
         // Master or slave device found

         wtStorageDevices[ currDeviceIdx ].valid = VALID_DEVICE_ENTRY;
         wtStorageDevices[ currDeviceIdx ].busNum = IGNORE_VALUE;
         wtStorageDevices[ currDeviceIdx ].devNum = IGNORE_VALUE;
         wtStorageDevices[ currDeviceIdx ].funNum = IGNORE_VALUE;
         wtStorageDevices[ currDeviceIdx ].cmdBase = cmdBase;
         wtStorageDevices[ currDeviceIdx ].ctrlBase = ctrlBase;
         wtStorageDevices[ currDeviceIdx ].bmideBase = bmideBase;
         wtStorageDevices[ currDeviceIdx ].irqNum = irqNum;
         wtStorageDevices[ currDeviceIdx ].devPos = PRIMARY_CHANNEL;
         wtStorageDevices[ currDeviceIdx ].masterSlave = ( reg_config_info[0] == REG_CONFIG_TYPE_ATA ) ? MASTER : SLAVE;
         wtStorageDevices[ currDeviceIdx ].regInfo0 = reg_config_info[0];
         wtStorageDevices[ currDeviceIdx ].regInfo1 = reg_config_info[1];         
         currDeviceIdx = ( ( currDeviceIdx + 1 ) % MAX_STORAGE_DEVICES );
         totalDevicesFound++;
      }
   }

   // --------------------------------------------------------------------------
   // Legacy IO ports: scan secondary channel
   // --------------------------------------------------------------------------

   if ( legacyChannelSecFound == FALSE ) {
      cmdBase   = LEGACY_SECONDARY_BASEPORT;
      ctrlBase  = 0x370;
      bmideBase = IGNORE_VALUE;
      irqNum    = 15;

      // Map ATA I/O regs to I/O ports
      pio_set_iobase_addr( cmdBase, ctrlBase, bmideBase );

      // Scan for ATA devices
      reg_config();

      if ( ( reg_config_info[0] == REG_CONFIG_TYPE_ATA ) || ( reg_config_info[1] == REG_CONFIG_TYPE_ATA ) ) {
         // Master or slave device found

         wtStorageDevices[ currDeviceIdx ].valid = VALID_DEVICE_ENTRY;
         wtStorageDevices[ currDeviceIdx ].busNum = IGNORE_VALUE;
         wtStorageDevices[ currDeviceIdx ].devNum = IGNORE_VALUE;
         wtStorageDevices[ currDeviceIdx ].funNum = IGNORE_VALUE;
         wtStorageDevices[ currDeviceIdx ].cmdBase = cmdBase;
         wtStorageDevices[ currDeviceIdx ].ctrlBase = ctrlBase;
         wtStorageDevices[ currDeviceIdx ].bmideBase = bmideBase;
         wtStorageDevices[ currDeviceIdx ].irqNum = irqNum;
         wtStorageDevices[ currDeviceIdx ].devPos = SECONDARY_CHANNEL;
         wtStorageDevices[ currDeviceIdx ].masterSlave = ( reg_config_info[0] == REG_CONFIG_TYPE_ATA ) ? MASTER : SLAVE;
         wtStorageDevices[ currDeviceIdx ].regInfo0 = reg_config_info[0];
         wtStorageDevices[ currDeviceIdx ].regInfo1 = reg_config_info[1];
         currDeviceIdx = ( ( currDeviceIdx + 1 ) % MAX_STORAGE_DEVICES );
         totalDevicesFound++;
      }
   }

   // --------------------------------------------------------------------------
   // Restore original settings
   // --------------------------------------------------------------------------
   tmr_set_command_timeout( tempCommandTimeout );

   return ( totalDevicesFound );
} // End ScanForStorageDevices

//------------------------------------------------------------------------------
// Description: Copies the base, controller, and bmide address of the device to
//              the library's global variables so each command sent will be
//              sent to those addresses.
// Note:        Sends NO data to the device so user must make sure a device is
//              attached.
//
// Input:  deviceIndex        - index into array of found devices. Use
//                              ScanForStorageDevices() first to populate this
//                              array
//
// Output: None
//------------------------------------------------------------------------------
void SetActiveDevice( unsigned int deviceIndex )
{
   unsigned int cmdBase, ctrlBase, bmideBase;

   if ( deviceIndex >= MAX_STORAGE_DEVICES ) {
      sprintf( upPrintString, "\nERROR: Storage device index invalid!!!" );
      PrintString( ukPrintOutput );
      return;
   }

   if ( wtStorageDevices[ deviceIndex ].valid != VALID_DEVICE_ENTRY ) {
      sprintf( upPrintString, "\nERROR: No storage device on selected index!!!" );
      PrintString( ukPrintOutput );
      return;
   }

   cmdBase   = wtStorageDevices[ deviceIndex ].cmdBase;
   ctrlBase  = wtStorageDevices[ deviceIndex ].ctrlBase;
   bmideBase = wtStorageDevices[ deviceIndex ].bmideBase;
   
   // Initialize ATALIB device parameters
   ukDevicePosition = wtStorageDevices[ deviceIndex ].masterSlave;
   uIRQNum = wtStorageDevices[ deviceIndex ].irqNum;
   dma_pci_enabled_flag = 0;
   
   // Restore interrupt handler if it was modified with previous device's IRQ number
   DisableInterrupt();
   
   // Align the I/O ports to the driver's variables
   pio_set_iobase_addr( cmdBase, ctrlBase, bmideBase );
   
   // This info is needed by the driver
   reg_config_info[ 0 ] = wtStorageDevices[ deviceIndex ].regInfo0;
   reg_config_info[ 1 ] = wtStorageDevices[ deviceIndex ].regInfo1;
   
   uActiveDeviceIndex = deviceIndex;

   return;
} // End SetActiveDevice

//------------------------------------------------------------------------------
// Description: Copies the base, controller, and bmide address of the device to
//              the library's global variables so each command sent will be
//              sent to those addresses.
//
// Input:  deviceIndex        - index into array of found devices. Use
//                              ScanForStorageDevices() first to populate this
//                              array
//
// Output: None
//------------------------------------------------------------------------------
void DiscoverActiveDevice( unsigned int deviceIndex )
{
   // Set I/O addresses for selected device
   SetActiveDevice( deviceIndex );

   // Let the driver know which devices are attached. This issues
   // commands to the device in order to validate it as an ATA device.
   reg_config();

   uActiveDeviceIndex = deviceIndex;

   return;
} // DiscoverActiveDevice

//------------------------------------------------------------------------------
// Description: Gets a pointer to a found device's information.
//
// Input:  deviceIndex        - index into storage devices, wtStorageDevices
//
// Output: pDeviceInfo*       - pointer to device info
//------------------------------------------------------------------------------
struct StorageDevice_t* GetDeviceInfo( unsigned int deviceIndex )
{
   struct StorageDevice_t* pDeviceInfo = NULL;
   
   if ( deviceIndex < MAX_STORAGE_DEVICES ) {
      pDeviceInfo = &wtStorageDevices[ deviceIndex ];
   }
   
   return ( pDeviceInfo );
}

//------------------------------------------------------------------------------
// Description: Displays all the found devices after ScanForStorageDevices()
//              is called.
//
// Input:  numDevices      - number of devices found;
//                           if SCAN_FOR_DEVICES scan for devices
//
// Output: None
//------------------------------------------------------------------------------
int DisplayConnectedATAStorageDevices( int numDevices )
{
   unsigned int eachDevice;

   // Scan must be called somewhere else no devices will ever be found
   if ( numDevices == SCAN_FOR_DEVICES ) {
      numDevices = ScanForStorageDevices();
   }

   if ( numDevices == 0 ) {
      printf( "No devices found!!!\n" );
   } else {
      printf( "Dev# | Base | bus/dev/fun | Model# [Serial#]\n" );
      printf( "-----+------+-------------+----------------------------------\n" );
      
      for ( eachDevice = 0; eachDevice < numDevices; eachDevice++ )
      {
         if ( wtStorageDevices[eachDevice].valid == VALID_DEVICE_ENTRY )
         {
            char wcModelString[45];  // words 27-46, so size must be >= 41 bytes
            char wcSerialNumber[25]; // words 10-19, so size must be >= 21 bytes
      
            // Setup device I/O ports for ID command
            SetActiveDevice( eachDevice );
      
            // Print device model string
            printf( "  %2d |", ( eachDevice + 1 ) );
            printf( " %04Xh|", wtStorageDevices[eachDevice].cmdBase );
      
            if ( ( wtStorageDevices[eachDevice].busNum == IGNORE_VALUE ) && (  wtStorageDevices[eachDevice].devNum == IGNORE_VALUE ) && (  wtStorageDevices[eachDevice].funNum == IGNORE_VALUE ) )
            {
               printf( "   --/--/--  |" );
            }
            else
            {
               printf( "  %02Xh/%02Xh/%01dh |", wtStorageDevices[eachDevice].busNum, wtStorageDevices[eachDevice].devNum, wtStorageDevices[eachDevice].funNum );
            }
      
            IdentifyDevice();
            GetModelString( upBufferPtr, wcModelString, sizeof( wcModelString ) );
            GetSerialNumber( upBufferPtr, wcSerialNumber, sizeof( wcSerialNumber ) );
      
            printf( " %s [%s]\n", wcModelString, wcSerialNumber );
         }
      }
   }

   return ( numDevices );
}

//------------------------------------------------------------------------------
// Description: Displays all the found devices after ScanForStorageDevices()
//              is called and waits until user selects a device.
//
// Input:  None
//
// Output: exitProgram     - TRUE  = user chose no devices
//                           FALSE = user chose a device
//------------------------------------------------------------------------------
int SelectConnectedATAStorageDevices()
{
   unsigned int eachDevice, numDevices;
   int selectedDev, exitProgram;

   selectedDev = -1;
   eachDevice = 0;
   exitProgram = FALSE;

   // Scan PCI and legacy IO ports
   numDevices = ScanForStorageDevices();

   // --------------------------------------------------------------------------
   // Print out all the devices
   // --------------------------------------------------------------------------

   while ( 1 )
   {
      DisplayConnectedATAStorageDevices( numDevices );

      printf( " Enter 0 to exit the program\n\n" );
      printf( "Enter Dev # to select drive: " );
      fflush( stdout );
      scanf( "%d", &selectedDev );

      TOOLS_DumpLine( stdin );

      if ( ( selectedDev > 0 ) && ( selectedDev < ( numDevices + 1 ) ) )
      {
         // Valid entry, starting number is 1

         SetActiveDevice( ( selectedDev - 1 ) );
         system( "cls" );
         break;
      }
      else if ( selectedDev == 0 )
      {
         // Exit program selected

         exitProgram = TRUE;
         break;
      }
      else
      {
         // Invalid entry

         system( "cls" );
         printf( "Invalid Selection %d!!!\n\n", selectedDev );
         selectedDev = -1;
      }
   }

   return ( exitProgram );
} // End SelectConnectedATAStorageDevices

//------------------------------------------------------------------------------
// Description: Prints PCI info about active device.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void PrintPCIDeviceInfo()
{
   printf( "pci bus/dev/fun ....: %Xh\n", wtStorageDevices[ uActiveDeviceIndex ].busNum, wtStorageDevices[ uActiveDeviceIndex ].devNum, wtStorageDevices[ uActiveDeviceIndex ].funNum );
   printf( "command base addr ..: %Xh\n", wtStorageDevices[ uActiveDeviceIndex ].cmdBase );
   printf( "ctrl base addr .....: %Xh\n", wtStorageDevices[ uActiveDeviceIndex ].ctrlBase );
   printf( "bmide base addr ....: %Xh\n", wtStorageDevices[ uActiveDeviceIndex ].bmideBase );
   printf( "IRQ number .........: %d\n", wtStorageDevices[ uActiveDeviceIndex ].irqNum );
   printf( "master/slave .......: %d (M=0/S=1)\n", wtStorageDevices[ uActiveDeviceIndex ].masterSlave );
   printf( "device position ....: %d\n", wtStorageDevices[ uActiveDeviceIndex ].devPos );
}

//------------------------------------------------------------------------------
// Description: Reads the SMART data from a device.
//
// Input:  None
// Output: ERROR/NO_ERROR
//------------------------------------------------------------------------------
int GetSmartAttributes()
{
   int returnStatus;

   if ( ukQuietMode == OFF ) {
      sprintf( upPrintString, "\n\nIssuing SMART READ DATA command" );
      PrintString( ukPrintOutput );
   }

   // Clear the buffer so there's no remnant data in buffer before reading
   memset( buffer, 0, sizeof( buffer ) );

   returnStatus = reg_pio_data_in_lba28( ukDevicePosition,
      CMD_SMART, SMART_READ_DATA,
      0, 0xC24F00,
      FP_SEG( upBufferPtr ), FP_OFF( upBufferPtr ),
      1, 0 );

   return ( returnStatus );
}
