//******************************************************************************
// ATA command library header -- ATALIB.h
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
// 1/27/2008
//
// The ATALIB library contains all the pre-compiler defines, global variables,
// and function declarations used in ATALIB.c.
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

//---------------------------------[DEFINES]------------------------------------

#define BUFFER_SIZE                             ( 32768 )

#define NUMBER_OF_CHARACTERS_IN_DOS_LINE        ( 80 )

#define PRIMARY_MASTER                          ( 0 )
#define PRIMARY_SLAVE                           ( 1 )
#define SECONDARY_MASTER                        ( 2 )
#define SECONDARY_SLAVE                         ( 3 )
#define MAX_LEGACY_BASEPORTS                    ( 4 )

#define LEGACY_PRIMARY_BASEPORT                 ( 0x1F0 )
#define LEGACY_SECONDARY_BASEPORT               ( 0x170 )

#define MASTER                                  ( 0 )
#define SLAVE                                   ( 1 )

#define DCO_TURN_SECURITY_OFF                   ( OFF )
#define DCO_TURN_SECURITY_ON                    ( ON )

#define HPA_VOLATILE                            ( 0 )
#define HPA_NON_VOLATILE                        ( 1 )

#define IGNORE_VALUE                            ( 0 )
#define INVALID_VALUE                           ( 0 )
#define SCAN_FOR_DEVICES                        ( -1 )

#define SECURITY_USUPPORTED                     ( 0x00 )
#define SECURITY_SUPPORTED                      ( 0x01 )
#define SECURITY_ENABLED                        ( 0x02 )
#define SECURITY_LOCKED                         ( 0x04 )
#define SECURITY_FROZEN                         ( 0x08 )
#define SECURITY_COUNT_EXPIRED                  ( 0x10 )
#define SECURITY_ENHANCED_SECURITY              ( 0x20 )

#define SECURITY_LEVEL_HIGH                     ( 0x00 )
#define SECURITY_LEVEL_MAXIMUM                  ( 0x01 )

#define USER_PASSWORD                           ( 0x0000 )
#define MASTER_PASSWORD                         ( 0x0001 )

#define NORMAL_SECURE_ERASE                     ( 0x0000 )
#define ENHANCED_SECURE_ERASE                   ( 0x0002 )

#define NORMAL_SECURE_ERASE_TIME                ( 1 )
#define ENHANCED_SECURE_ERASE_TIME              ( 2 )

#define DEFAULT_ERASE_TIME_IN_MINUTES           ( 5 * 60 )

#define GET_ID_DATA                             ( NULL )
#define UNSHARED_INTERRUPT                      ( 0 )

#define LBA28_MODE                              ( 1 )
#define LBA48_MODE                              ( 2 )
#define CHS_MODE                                ( 3 )

#define PRINT_SCREEN                            ( 1 )
#define PRINT_LOG                               ( 2 )
#define PRINT_SCREEN_AND_LOG                    ( 3 )

#define PRINT_BYTE                              ( 1 )
#define PRINT_WORD                              ( 2 )

#define STATUS_DRIVE_READY                      ( 0x50 )
#define STATUS_DRIVE_ERROR                      ( 0x51 )
#define ERROR_NONE                              ( 0x00 )
#define ERROR_ABORT                             ( 0x04 )

#define MAX_STORAGE_DEVICES                     ( 16 )            // Arbitrary value, can be expanded
#define VALID_DEVICE_ENTRY                      ( 0xDCDC )

//---------------------------------[ENUMS]--------------------------------------

// Enums
enum LegacyIOChannels_t
{
   PRIMARY_CHANNEL = 0,
   SECONDARY_CHANNEL
};

enum DeviceLocations_t
{
   PCI_BUS,
   LEGACY_IO_PORTS
};

//----------------------------[GLOBAL STRUCTURES]-------------------------------

struct StorageDevice_t {
   unsigned int valid;
   unsigned int busNum;
   unsigned int devNum;
   unsigned int funNum;
   unsigned int cmdBase;
   unsigned int ctrlBase;
   unsigned int bmideBase;
   unsigned int irqNum;
   unsigned int devPos;
   unsigned int masterSlave;
   unsigned int regInfo0;
   unsigned int regInfo1;
};

#pragma pack( push, 1 ) 
typedef struct tSMARTData {
   short revNum;                 // ofs 0-1
   char wcAttribs[ 362 ];        // ofs 2-361
   char offLineStat;             // ofs 362
   char dstStat;                 // ofs 363
   short offlineTimeSec;         // ofs 364-365
   char vendSpec1;               // ofs 366
   char offlineCap;              // ofs 367
   short smartCap;               // ofs 368-369
   char errorLogCap;             // ofs 370
   char vendSpec2;               // ofs 371
   char shortDSTPollMin;         // ofs 372
   char extDSTPollMin;           // ofs 373
   char ConveyDSTPollMin;        // ofs 374
   char rsvd[ 11 ];              // ofs 375-385
   char vendSpec3[ 125 ];        // ofs 386-510
   char checksum;                // ofs 511
} SMARTData_t;

typedef struct tSmartAttribute {
   char idNum;                   // ofs 0
   short flag;                   // ofs 1-2
   char curValue;                // ofs 3
   char worstValue;              // ofs 4
   unsigned short rawValueLo;    // ofs 5-6
   unsigned short rawValueMid;   // ofs 7-8
   unsigned short rawValueHi;    // ofs 9-10
   char rsvd;                    // ofs 11
} SMARTAttribute_t;   
#pragma pack( pop )

//----------------------------[GLOBAL VARIABLES]--------------------------------

extern int ukDevicePosition;
extern int ukMulti;                 // Multi count for MULTIPLE commands
extern int ukQuietMode;             // Controls all the printing by ATACMD.c
extern int ukPrintOutput;           // Controls where everything is printed in ATACMD.c
extern int ukTotalErrors;           // Global error counter for ATACMD.c
extern int uIRQNum;                 // IRQ num of active/selected device
extern int uActiveDeviceIndex;

extern int ukReturnValue1;
extern int ukReturnValue2;
extern int ukReturnValue3;
extern int ukReturnValue4;
extern int ukReturnValue5;
extern int ukReturnValue6;

extern unsigned long ugReturnValue1;
extern unsigned long ugReturnValue2;
extern unsigned long ugReturnValue3;
extern unsigned long ugReturnValue4;
extern unsigned long ugReturnValue5;

// Arrarys
extern unsigned char buffer[BUFFER_SIZE];   // The global data buffer for ATACMD.c
extern unsigned char wcMaxLBA[8];           // Holds the max LBA from different ATACMD.c functions
extern char wcPrintBuffer[NUMBER_OF_CHARACTERS_IN_DOS_LINE+1];   // Allocates memory for ATACMD.c printing
extern char wcDriveString[3];
extern char wcPasswordString[33];
extern char wcDisclaimerReply[5];
extern char wcUserReply[5];

// Pointers
extern FILE* upLog;
extern char* upPrintString;                  // The global print string for ATACMD.c
extern unsigned char far* upBufferPtr;

//--------------------------[FUNCTION DECLARATIONS]-----------------------------

// Please add in alphabetical order
extern void ATALIB_CleanUp( void );
extern void ATALIB_Initialize( void );
extern void ChangeDriveCapacityViaDCO( unsigned long gNewCapacity );
extern void ChangeSecuritySupportViaDCO( int kSecurityTurnOn );
extern void CheckDCOSupported( void );
extern void Check48BitAddressingSupported( void );
extern void CheckEnhancedSecureEraseSupported( void );
extern void CheckHPASet( void );
extern void CheckHPASupported( void );
extern void CheckSecurityCountExpired( void );
extern void CheckSecurityEnabled( void );
extern void CheckSecurityFrozen( void );
extern void CheckSecurityLevel( void );
extern void CheckSecurityLocked( void );
extern void CheckSecuritySupported( void );
extern void CheckStatusAndErrorRegisters( unsigned char expectedStatus, char expectedError );
extern void DeviceConfigurationIdentify( void );
extern void DeviceConfigurationRestore( void );
extern void DisableInterrupt( void );
extern void DiscoverActiveDevice( unsigned int deviceIndex );
extern int DisplayConnectedATAStorageDevices( int numDevices );
extern int EnableInterrupt( void );
extern int EnableISADMA( void );
extern int EnablePCIDMA( void );
extern void HandleError( int kErrorFlag );
extern void IdentifyDevice( void );
extern struct StorageDevice_t* GetDeviceInfo( unsigned int deviceIndex );
extern int GetDriveSecurityState( void );
extern void GetEstimatedSecureEraseTimesInMin( void );
extern void GetFirmwareRevision( void* pIDData, char* const pFirmwareRevision, unsigned int buffSizeInBytes );
extern int GetIDWord( char* pIDBuffer, unsigned int byteOffset );
extern void GetMaxLBAFromDCO( void );
extern void GetMaxLBAFromIdentifyDevice( void );
extern void GetMaxLBAFromReadNativeMax( void );
extern void GetModelString( void* pIDData, char* const pModelNum, unsigned int buffSizeInBytes );
extern void GetSerialNumber( void* pIDData, char* const pSerialNum, unsigned int buffSizeInBytes );
extern int GetSmartAttributes( void );
extern void PrintBuffer( void* pBuffer, int numberOfBytes, int printType );
extern void PrintDataBufferHex( int numberOfBytes, int printType );
extern void PrintATACMDGlobalOptions( void );
extern void PrintDriveSecurityState( void );
extern void PrintErrorMessage( void );
extern void PrintFailMessage( void );
extern void PrintFirmwareRevision( void );
extern void PrintModelString( void );
extern void PrintPCIDeviceInfo( void );
extern void PrintSerialNumber( void );
extern void PrintStatusAndErrorRegisters( void );
extern void PrintString( int kPrintType );
extern void ReadDMA( unsigned long gLBA, unsigned long gNumberOfSectors );
extern void ReadSectors( unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gLBA, unsigned long gNumberOfSectors, int kReadMode );
extern void ReadSectorsInCHS( unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gNumberOfSectors );
extern void ReadSectorsInLBA28( unsigned long gLBA, unsigned long gNumberOfSectors );
extern void ReadSectorsInLBA48( unsigned long gLBA, unsigned long gNumberOfSectors );
extern void RemoveHPA( void );
extern void ReadNativeMaxAddress( int kCommandType );
extern unsigned int ScanForStorageDevices( void );
extern void SecureErase( const char* wcPasswordString, int kPasswordType, int kEraseType );
extern void SecuritySetPassword( const char* wcPasswordString, int kPasswordType, int kSecurityLevel );
extern void SecurityUnlockPassword( const char* wcPasswordString, int kPasswordType );
extern void SecurityDisablePassword( const char* wcPasswordString, int kPasswordType );
extern int SelectConnectedATAStorageDevices( void );
extern void SendATACommand( long int* pAtaRegs );
extern int SendNonDataCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned int cylinder, unsigned int head, unsigned int secNum );
extern int SendLBA28DataInCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lba );
extern int SendLBA48DataInCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lbaLow, unsigned long lbaHigh );
extern int SendLBA28DataOutCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lba );
extern int SendLBA48DataOutCommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lbaLow, unsigned long lbaHigh );
extern int SendLBA28DMACommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lba );
extern int SendLBA48DMACommand( int cmd, unsigned int feat, unsigned int secCnt, unsigned long lbaLow, unsigned long lbaHigh );
extern void SetActiveDevice( unsigned int deviceIndex );
extern void SetBasePorts( int kSelectBasePort );
extern void SetHPA( int kCommandType, int kVolatility, unsigned long gLBA );
extern void SetMaxAddress( int kCommandType, int kVolatility, unsigned long gLBA );
extern void SoftwareReset( void );
extern void WriteDMA( unsigned long gLBA, unsigned long gNumberOfSectors );
extern void WriteSectors( unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gLBA, unsigned long gNumberOfSectors, int kWriteMode );
extern void WriteSectorsInCHS( unsigned int kCylinder, unsigned int kHead, unsigned int kSector, unsigned long gNumberOfSectors );
extern void WriteSectorsInLBA28( unsigned long gLBA, unsigned long gNumberOfSectors );
extern void WriteSectorsInLBA48( unsigned long gLBA, unsigned long gNumberOfSectors );
