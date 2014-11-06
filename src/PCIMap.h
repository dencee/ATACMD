//******************************************************************************
// PCI bus address map -- PCIMap.h
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
// References:
// -----------
// http://wiki.osdev.org/PCI
//
//******************************************************************************

//---------------------------------[DEFINES]------------------------------------

#define PCI_READ_CONFIGURATION_BYTE                      ( 0xB108 )
#define PCI_READ_CONFIGURATION_WORD                      ( 0xB109 )
#define X86_INTERRUPT_1A                                 ( 0x1A )

#define PCI_MAX_BUS_NUMBER                               ( 256 )
#define PCI_MAX_DEVICE_NUMBER                            ( 32 )
#define PCI_MAX_FUNCTION_NUMBER                          ( 8 )

//#define PCI_BAR0_OFFSET                                  ( 0x10 )
//#define PCI_BAR1_OFFSET                                  ( 0x14 )
//#define PCI_BAR4_OFFSET                                  ( 0x20 )
//#define PCI_INTERRUPT_LINE_OFFSET                        ( 0x3C )

#define PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER           ( 0x01 )
#define PCI_SUBCLASS_CODE_IDE_CONTROLLER                 ( 0x01 )
#define PCI_SUBCLASS_CODE_ATA_CONTROLLER                 ( 0x05 )
#define PCI_SUBCLASS_CODE_SATA_CONTROLLER                ( 0x06 )
#define PCI_SUBCLASS_CODE_UNKNOWN_STORAGE_CONTROLLER     ( 0x80 )

//--------------------------------[STRUCTS]-------------------------------------

// http://wiki.osdev.org/PCI
typedef struct {                    // Offset:
   unsigned short vendorId;         // 0x00
   unsigned short devId;            // 0x02
   unsigned short command;          // 0x04
   unsigned short status;           // 0x06
   unsigned char  revId;            // 0x08
   unsigned char  progIf;           // 0x09
   unsigned char  subClass;         // 0x0A
   unsigned char  classCode;        // 0x0B
   unsigned char  cacheLineSize;    // 0x0C
   unsigned char  latencyTimer;     // 0x0D
   unsigned char  headerType;       // 0x0E
   unsigned char  BIST;             // 0x0F
   unsigned long  BAR0;             // 0x10
   unsigned long  BAR1;             // 0x14
   unsigned long  BAR2;             // 0x18
   unsigned long  BAR3;             // 0x1C
   unsigned long  BAR4;             // 0x20
   unsigned long  BAR5;             // 0x24
   unsigned long  CardBusCISPtr;    // 0x28
   unsigned short SubSysVendorId;   // 0x2C
   unsigned short SubSysId;         // 0x2E
   unsigned long  EROMBAR;          // 0x30
   unsigned char  capPtr;           // 0x34
   unsigned char  Rsvd[7];          // 0x35
   unsigned char  intLine;          // 0x3C
   unsigned char  intPIN;           // 0x3D
   unsigned char  minGrant;         // 0x3E
   unsigned char  maxLatency;       // 0x3F
} PCIRegisters_t ;

