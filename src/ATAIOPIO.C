//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOPIO.C
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
// This module contains inline assembler code so you'll
// also need Borland's Assembler.
//
// This C source contains the low level I/O port IN/OUT functions.
//********************************************************************

#include <dos.h>

#include "ataio.h"

//*************************************************************
//
// Host adapter base addresses.
//
//*************************************************************

unsigned int pio_base_addr1 = 0x1f0;
unsigned int pio_base_addr2 = 0x3f0;

unsigned int pio_memory_seg = 0;
int pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;

unsigned int pio_bmide_base_addr = 0;

unsigned int pio_reg_addrs[10];

unsigned char pio_last_write[10];
unsigned char pio_last_read[10];

int pio_xfer_width = 16;

extern void PreserveAXDX(void);
#pragma aux PreserveAXDX = \
   "push ax"               \
   "push dx"               ;

extern unsigned char AsmInpB(int regAddr);
#pragma aux AsmInpB = \
   "in    al,dx"      \
   value [al]         \
   parm  [dx]         ;

extern unsigned int AsmInpW(int regAddr);
#pragma aux AsmInpW = \
   "in    ax,dx"      \
   value [ax]         \
   parm  [dx]         ;

extern void AsmOutpB(int regAddr, unsigned char data);
#pragma aux AsmOutpB = \
   "out   dx,al"       \
   parm [dx] [ax]      ;

extern void AsmOutpW(int regAddr, unsigned int data);
#pragma aux AsmOutpW = \
   "out   dx,ax"       \
   parm [dx] [ax]      ;

extern void RestoreDXAX(void);
#pragma aux RestoreDXAX = \
   "pop dx"               \
   "pop ax"               \
   modify [ax dx]         ;

extern void PreserveAXCXDXDIES(void);
#pragma aux PreserveAXCXDXDIES = \
   "push ax"                     \
   "push cx"                     \
   "push dx"                     \
   "push di"                     \
   "push es"                     ;

extern void ReadBlockPIOB(unsigned int bufSeg, unsigned int bufOff, unsigned int bCnt, unsigned int dataRegAddr);
#pragma aux ReadBlockPIOB = \
   "mov   es,ax"            \
   "cld"                    \
   "rep   insb"             \
   parm [ax] [di] [cx] [dx] \
   modify [cx di es]        ;

extern void ReadBlockPIOW(unsigned int bufSeg, unsigned int bufOff, unsigned int wCnt, unsigned int dataRegAddr);
#pragma aux ReadBlockPIOW = \
   "mov   es,ax"            \
   "cld"                    \
   "rep   insw"             \
   parm [ax] [di] [cx] [dx] \
   modify [cx di es]        ;

extern void ReadBlockPIOD(unsigned int bufSeg, unsigned int bufOff, unsigned int dwCnt, unsigned int dataRegAddr);
#pragma aux ReadBlockPIOD = \
   "mov   es,ax"            \
   "cld"                    \
   "rep   insd"             \
   parm [ax] [di] [cx] [dx] \
   modify [cx di es]        ;

extern void RestoreESDIDXCXAX(void);
#pragma aux RestoreESDIDXCXAX = \
   "pop es"                     \
   "pop di"                     \
   "pop dx"                     \
   "pop cx"                     \
   "pop ax"                     \
   modify [ax cx dx di es]      ;

extern void PreserveAXCXDXSIDS(void);
#pragma aux PreserveAXCXDXSIDS = \
   "push ax"                     \
   "push cx"                     \
   "push dx"                     \
   "push si"                     \
   "push ds"                     ;

extern void WriteBlockPIOB(unsigned int bufSeg, unsigned int bufOff, unsigned int bCnt, unsigned int dataRegAddr);
#pragma aux WriteBlockPIOB = \
   "mov   ds,ax"             \
   "cld"                     \
   "rep   outsb"             \
   parm [ax] [si] [cx] [dx]  \
   modify [cx si ds]         ;

extern void WriteBlockPIOW(unsigned int bufSeg, unsigned int bufOff, unsigned int wCnt, unsigned int dataRegAddr);
#pragma aux WriteBlockPIOW = \
   "mov   ds,ax"             \
   "cld"                     \
   "rep   outsw"             \
   parm [ax] [si] [cx] [dx]  \
   modify [cx si ds]         ;

extern void WriteBlockPIOD(unsigned int bufSeg, unsigned int bufOff, unsigned int dwCnt, unsigned int dataRegAddr);
#pragma aux WriteBlockPIOD = \
   "mov   ds,ax"             \
   "cld"                     \
   "rep   outsd"             \
   parm [ax] [si] [cx] [dx]  \
   modify [cx si ds]         ;

extern void RestoreDSSIDXCXAX(void);
#pragma aux RestoreDSSIDXCXAX = \
   "pop ds"                     \
   "pop si"                     \
   "pop dx"                     \
   "pop cx"                     \
   "pop ax"                     \
   modify [ax cx dx si ds]      ;

//*************************************************************
//
// Set the host adapter i/o base addresses.
//
//*************************************************************

void pio_set_iobase_addr( unsigned int base1,
                          unsigned int base2,
                          unsigned int base3 )
{

   pio_base_addr1 = base1;
   pio_base_addr2 = base2;
   pio_bmide_base_addr = base3;
   pio_memory_seg = 0;
   pio_reg_addrs[ CB_DATA ] = pio_base_addr1 + 0;  // 0
   pio_reg_addrs[ CB_FR   ] = pio_base_addr1 + 1;  // 1
   pio_reg_addrs[ CB_SC   ] = pio_base_addr1 + 2;  // 2
   pio_reg_addrs[ CB_SN   ] = pio_base_addr1 + 3;  // 3
   pio_reg_addrs[ CB_CL   ] = pio_base_addr1 + 4;  // 4
   pio_reg_addrs[ CB_CH   ] = pio_base_addr1 + 5;  // 5
   pio_reg_addrs[ CB_DH   ] = pio_base_addr1 + 6;  // 6
   pio_reg_addrs[ CB_CMD  ] = pio_base_addr1 + 7;  // 7
   pio_reg_addrs[ CB_DC   ] = pio_base_addr2 + 6;  // 8
   pio_reg_addrs[ CB_DA   ] = pio_base_addr2 + 7;  // 9
}

//*************************************************************
//
// Set the host adapter memory base addresses.
//
//*************************************************************

void pio_set_memory_addr( unsigned int seg )

{

   pio_base_addr1 = 0;
   pio_base_addr2 = 8;
   pio_bmide_base_addr = 0;
   pio_memory_seg = seg;
   pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;
   pio_reg_addrs[ CB_DATA ] = pio_base_addr1 + 0;  // 0
   pio_reg_addrs[ CB_FR   ] = pio_base_addr1 + 1;  // 1
   pio_reg_addrs[ CB_SC   ] = pio_base_addr1 + 2;  // 2
   pio_reg_addrs[ CB_SN   ] = pio_base_addr1 + 3;  // 3
   pio_reg_addrs[ CB_CL   ] = pio_base_addr1 + 4;  // 4
   pio_reg_addrs[ CB_CH   ] = pio_base_addr1 + 5;  // 5
   pio_reg_addrs[ CB_DH   ] = pio_base_addr1 + 6;  // 6
   pio_reg_addrs[ CB_CMD  ] = pio_base_addr1 + 7;  // 7
   pio_reg_addrs[ CB_DC   ] = pio_base_addr2 + 6;  // 8
   pio_reg_addrs[ CB_DA   ] = pio_base_addr2 + 7;  // 9
}

//*************************************************************
//
// These functions do basic IN/OUT of byte and word values:
//
//    pio_inbyte()
//    pio_outbyte()
//    pio_inword()
//    pio_outword()
//
//*************************************************************

unsigned char pio_inbyte( unsigned int addr )

{
   unsigned int regAddr;
   unsigned char uc;
   unsigned char far * ucp;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      ucp = (unsigned char far *) MK_FP( pio_memory_seg, regAddr );
      uc = * ucp;
   }
   else
   {
      // uc = (unsigned char) inportb( regAddr );

      // READ THIS: If you get a compile error on the following
      // statement you are trying to use BASM (the assembler
      // built into Borland C). BASM can not assemble 386
      // instructions. You must use Borland TASM as is shown
      // in the EXAMPLE1.MAK or EXAMPLE2.MAK "make files".

      #ifdef    __WATCOMC__

         PreserveAXDX();
         uc = AsmInpB( regAddr );
         RestoreDXAX();

      #else

         asm   .386

         asm   push  ax
         asm   push  dx

         asm   mov   dx,regAddr

         asm   in    al,dx
         asm   mov   uc,al

         asm   pop   dx
         asm   pop   ax

      #endif

   }
   pio_last_read[ addr ] = uc;
   trc_llt( addr, uc, TRC_LLT_INB );
   return uc;
}

//*************************************************************

void pio_outbyte( unsigned int addr, unsigned char data )

{
   unsigned int regAddr;
   unsigned char far * ucp;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      ucp = (unsigned char far *) MK_FP( pio_memory_seg, regAddr );
      * ucp = data;
   }
   else
   {
      // outportb( regAddr, data );

      #ifdef    __WATCOMC__

         PreserveAXDX();
         AsmOutpB( regAddr, data );
         RestoreDXAX();

      #else

         asm   .386

         asm   push  ax
         asm   push  dx

         asm   mov   dx,regAddr
         asm   mov   al,data

         asm   out   dx,al

         asm   pop   dx
         asm   pop   ax

      #endif

   }
   pio_last_write[ addr ] = data;
   trc_llt( addr, data, TRC_LLT_OUTB );
}

//*************************************************************

unsigned int pio_inword( unsigned int addr )

{
   unsigned int regAddr;
   unsigned int ui;
   unsigned int far * uip;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      uip = (unsigned int far *) MK_FP( pio_memory_seg, regAddr );
      ui = * uip;
   }
   else
   {
      // ui = inport( regAddr );

      #ifdef    __WATCOMC__

         PreserveAXDX();
         ui = AsmInpW( regAddr );
         RestoreDXAX();

      #else

         asm   .386

         asm   push  ax
         asm   push  dx

         asm   mov   dx,regAddr

         asm   in    ax,dx
         asm   mov   ui,ax

         asm   pop   dx
         asm   pop   ax

      #endif

   }
   trc_llt( addr, 0, TRC_LLT_INW );
   return ui;
}

//*************************************************************

void pio_outword( unsigned int addr, unsigned int data )

{
   unsigned int regAddr;
   unsigned int far * uip;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      uip = (unsigned int far *) MK_FP( pio_memory_seg, regAddr );
      * uip = data;
   }
   else
   {
      // outport( regAddr, data );

      #ifdef    __WATCOMC__

         PreserveAXDX();
         AsmOutpW( regAddr, data );
         RestoreDXAX();

      #else

         asm   .386

         asm   push  ax
         asm   push  dx

         asm   mov   dx,regAddr
         asm   mov   ax,data

         asm   out   dx,ax

         asm   pop   dx
         asm   pop   ax

      #endif

   }
   trc_llt( addr, 0, TRC_LLT_OUTW );
}

//*************************************************************
//
// These functions are normally used to transfer DRQ blocks:
//
// pio_drq_block_in()
// pio_drq_block_out()
//
//*************************************************************

// Note: pio_drq_block_in() is the primary way perform PIO
// Data In transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_drq_block_in( unsigned int addrDataReg,
                       unsigned int bufSeg, unsigned int bufOff,
                       long wordCnt )

{
   long bCnt;
   int memDtOpt;
   unsigned int randVal;
   unsigned int dataRegAddr;
   unsigned int far * uip1;
   unsigned int far * uip2;
   unsigned char far * ucp1;
   unsigned char far * ucp2;
   unsigned long bufAddr;

   // NOTE: wordCnt is the size of a DRQ block
   // in words. The maximum value of wordCnt is normally:
   // a) For ATA, 16384 words or 32768 bytes (64 sectors,
   //    only with READ/WRITE MULTIPLE commands),
   // b) For ATAPI, 32768 words or 65536 bytes
   //    (actually 65535 bytes plus a pad byte).

   // normalize bufSeg:bufOff

   bufAddr = bufSeg;
   bufAddr = bufAddr << 4;
   bufAddr = bufAddr + bufOff;

   if ( pio_memory_seg )
   {

      // PCMCIA Memory mode data transfer.

      // set Data reg address per pio_memory_dt_opt
      dataRegAddr = 0x0000;
      memDtOpt = pio_memory_dt_opt;
      if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
      {
         randVal = * (unsigned int *) MK_FP( 0x40, 0x6c );
         memDtOpt = randVal % 3;
      }
      if ( memDtOpt == PIO_MEMORY_DT_OPT8 )
         dataRegAddr = 0x0008;
      if ( memDtOpt == PIO_MEMORY_DT_OPTB )
      {
         dataRegAddr = 0x0400;
         if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
            dataRegAddr = dataRegAddr | ( randVal & 0x03fe );
      }

      if ( pio_xfer_width == 8 )
      {
         // PCMCIA Memory mode 8-bit
         bCnt = wordCnt * 2L;
         ucp1 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
         for ( ; bCnt > 0; bCnt -- )
         {
            bufSeg = (unsigned int) ( bufAddr >> 4 );
            bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
            ucp2 = (unsigned char far *) MK_FP( bufSeg, bufOff );
            * ucp2 = * ucp1;
            bufAddr += 1;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr += 1;
               dataRegAddr = ( dataRegAddr & 0x03ff ) | 0x0400;
               ucp1 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_INSB );
      }
      else
      {
         // PCMCIA Memory mode 16-bit
         uip1 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
         for ( ; wordCnt > 0; wordCnt -- )
         {
            bufSeg = (unsigned int) ( bufAddr >> 4 );
            bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
            uip2 = (unsigned int far *) MK_FP( bufSeg, bufOff );
            * uip2 = * uip1;
            bufAddr += 2;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr += 2;
               dataRegAddr = ( dataRegAddr & 0x03fe ) | 0x0400;
               uip1 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_INSW );
      }
   }
   else
   {
      int pxw;
      long wc;

      // adjust pio_xfer_width - don't use DWORD if wordCnt is odd.

      pxw = pio_xfer_width;
      if ( ( pxw == 32 ) && ( wordCnt & 0x00000001L ) )
         pxw = 16;

      // Data transfer using INS instruction.
      // Break the transfer into chunks of 32768 or fewer bytes.

      while ( wordCnt > 0 )
      {
         bufSeg = (unsigned int) ( bufAddr >> 4 );
         bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
         if ( wordCnt > 16384L )
            wc = 16384;
         else
            wc = wordCnt;
         if ( pxw == 8 )
         {
            // do REP INS
            pio_rep_inbyte( addrDataReg, bufSeg, bufOff, wc * 2L );
         }
         else
         if ( pxw == 32 )
         {
            // do REP INSD
            pio_rep_indword( addrDataReg, bufSeg, bufOff, wc / 2L );
         }
         else
         {
            // do REP INSW
            pio_rep_inword( addrDataReg, bufSeg, bufOff, wc );
         }
         bufAddr = bufAddr + ( wc * 2 );
         wordCnt = wordCnt - wc;
      }
   }

   return;
}

//*************************************************************

// Note: pio_drq_block_out() is the primary way perform PIO
// Data Out transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_drq_block_out( unsigned int addrDataReg,
                        unsigned int bufSeg, unsigned int bufOff,
                        long wordCnt )

{
   long bCnt;
   int memDtOpt;
   unsigned int randVal;
   unsigned int dataRegAddr;
   unsigned int far * uip1;
   unsigned int far * uip2;
   unsigned char far * ucp1;
   unsigned char far * ucp2;
   unsigned long bufAddr;

   // NOTE: wordCnt is the size of a DRQ block
   // in words. The maximum value of wordCnt is normally:
   // a) For ATA, 16384 words or 32768 bytes (64 sectors,
   //    only with READ/WRITE MULTIPLE commands),
   // b) For ATAPI, 32768 words or 65536 bytes
   //    (actually 65535 bytes plus a pad byte).

   // normalize bufSeg:bufOff

   bufAddr = bufSeg;
   bufAddr = bufAddr << 4;
   bufAddr = bufAddr + bufOff;

   if ( pio_memory_seg )
   {

      // PCMCIA Memory mode data transfer.

      // set Data reg address per pio_memory_dt_opt
      dataRegAddr = 0x0000;
      memDtOpt = pio_memory_dt_opt;
      if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
      {
         randVal = * (unsigned int *) MK_FP( 0x40, 0x6c );
         memDtOpt = randVal % 3;
      }
      if ( memDtOpt == PIO_MEMORY_DT_OPT8 )
         dataRegAddr = 0x0008;
      if ( memDtOpt == PIO_MEMORY_DT_OPTB )
      {
         dataRegAddr = 0x0400;
         if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
            dataRegAddr = dataRegAddr | ( randVal & 0x03fe );
      }

      if ( pio_xfer_width == 8 )
      {
         // PCMCIA Memory mode 8-bit
         bCnt = wordCnt * 2L;
         ucp2 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
         for ( ; bCnt > 0; bCnt -- )
         {
            bufSeg = (unsigned int) ( bufAddr >> 4 );
            bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
            ucp1 = (unsigned char far *) MK_FP( bufSeg, bufOff );
            * ucp2 = * ucp1;
            bufAddr += 1;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr += 1;
               dataRegAddr = ( dataRegAddr & 0x03ff ) | 0x0400;
               ucp2 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_OUTSB );
      }
      else
      {
         // PCMCIA Memory mode 16-bit
         uip2 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
         for ( ; wordCnt > 0; wordCnt -- )
         {
            bufSeg = (unsigned int) ( bufAddr >> 4 );
            bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
            uip1 = (unsigned int far *) MK_FP( bufSeg, bufOff );
            * uip2 = * uip1;
            bufAddr += 2;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr = dataRegAddr + 2;
               dataRegAddr = ( dataRegAddr & 0x03fe ) | 0x0400;
               uip2 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_OUTSW );
      }
   }
   else
   {
      int pxw;
      long wc;

      // adjust pio_xfer_width - don't use DWORD if wordCnt is odd.

      pxw = pio_xfer_width;
      if ( ( pxw == 32 ) && ( wordCnt & 0x00000001L ) )
         pxw = 16;

      // Data transfer using OUTS instruction.
      // Break the transfer into chunks of 32768 or fewer bytes.

      while ( wordCnt > 0 )
      {
         bufOff = (unsigned int) ( bufAddr & 0x0000000fL );
         bufSeg = (unsigned int) ( bufAddr >> 4 );
         if ( wordCnt > 16384L )
            wc = 16384;
         else
            wc = wordCnt;
         if ( pxw == 8 )
         {
            // do REP OUTS
            pio_rep_outbyte( addrDataReg, bufSeg, bufOff, wc * 2L );
         }
         else
         if ( pxw == 32 )
         {
            // do REP OUTSD
            pio_rep_outdword( addrDataReg, bufSeg, bufOff, wc / 2L );
         }
         else
         {
            // do REP OUTSW
            pio_rep_outword( addrDataReg, bufSeg, bufOff, wc );
         }
         bufAddr = bufAddr + ( wc * 2 );
         wordCnt = wordCnt - wc;
      }
   }

   return;
}

//*************************************************************
//
// These functions do REP INS/OUTS data transfers
// (PIO data transfers in I/O mode):
//
// pio_rep_inbyte()
// pio_rep_outbyte()
// pio_rep_inword()
// pio_rep_outword()
// pio_rep_indword()
// pio_rep_outdword()
//
// These functions can be called directly but usually they
// are called by the pio_drq_block_in() and pio_drq_block_out()
// functions to perform I/O mode transfers. See the
// pio_xfer_width variable!
//
//*************************************************************

void pio_rep_inbyte( unsigned int addrDataReg,
                     unsigned int bufSeg, unsigned int bufOff,
                     long byteCnt )

{
   unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
   unsigned int bCnt = (unsigned int) byteCnt;

   // Warning: Avoid calling this function with
   // byteCnt > 32768 (transfers 32768 bytes).
   // bufSeg and bufOff should be normalized such
   // that bufOff is a value between 0 and 15 (0xf).

   #ifdef    __WATCOMC__

      PreserveAXCXDXDIES();
      ReadBlockPIOB( bufSeg, bufOff, bCnt, dataRegAddr );
      RestoreESDIDXCXAX();

   #else

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  di
      asm   push  es

      asm   mov   ax,bufSeg
      asm   mov   es,ax
      asm   mov   di,bufOff

      asm   mov   cx,bCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   insb

      asm   pop   es
      asm   pop   di
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

   #endif

   trc_llt( addrDataReg, 0, TRC_LLT_INSB );
}

//*************************************************************

void pio_rep_outbyte( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      long byteCnt )

{
   unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
   unsigned int bCnt = (unsigned int) byteCnt;

   // Warning: Avoid calling this function with
   // byteCnt > 32768 (transfers 32768 bytes).
   // bufSeg and bufOff should be normalized such
   // that bufOff is a value between 0 and 15 (0xf).

   #ifdef    __WATCOMC__

      PreserveAXCXDXSIDS();
      WriteBlockPIOB( bufSeg, bufOff, bCnt, dataRegAddr );
      RestoreDSSIDXCXAX();

   #else

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  si
      asm   push  ds

      asm   mov   ax,bufSeg
      asm   mov   ds,ax
      asm   mov   si,bufOff

      asm   mov   cx,bCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   outsb

      asm   pop   ds
      asm   pop   si
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

   #endif

   trc_llt( addrDataReg, 0, TRC_LLT_OUTSB );
}

//*************************************************************

void pio_rep_inword( unsigned int addrDataReg,
                     unsigned int bufSeg, unsigned int bufOff,
                     long wordCnt )

{
   unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
   unsigned int wCnt = (unsigned int) wordCnt;

   // Warning: Avoid calling this function with
   // wordCnt > 16384 (transfers 32768 bytes).
   // bufSeg and bufOff should be normalized such
   // that bufOff is a value between 0 and 15 (0xf).

   #ifdef    __WATCOMC__

      PreserveAXCXDXDIES();
      ReadBlockPIOW( bufSeg, bufOff, wCnt, dataRegAddr );
      RestoreESDIDXCXAX();

   #else

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  di
      asm   push  es

      asm   mov   ax,bufSeg
      asm   mov   es,ax
      asm   mov   di,bufOff

      asm   mov   cx,wCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   insw

      asm   pop   es
      asm   pop   di
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

   #endif

   trc_llt( addrDataReg, 0, TRC_LLT_INSW );
}

//*************************************************************

void pio_rep_outword( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      long wordCnt )

{
   unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
   unsigned int wCnt = (unsigned int) wordCnt;

   // Warning: Avoid calling this function with
   // wordCnt > 16384 (transfers 32768 bytes).
   // bufSeg and bufOff should be normalized such
   // that bufOff is a value between 0 and 15 (0xf).

   #ifdef    __WATCOMC__

      PreserveAXCXDXSIDS();
      WriteBlockPIOW( bufSeg, bufOff, wCnt, dataRegAddr );
      RestoreDSSIDXCXAX();

   #else

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  si
      asm   push  ds

      asm   mov   ax,bufSeg
      asm   mov   ds,ax
      asm   mov   si,bufOff

      asm   mov   cx,wCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   outsw

      asm   pop   ds
      asm   pop   si
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

   #endif

   trc_llt( addrDataReg, 0, TRC_LLT_OUTSW );
}

//*************************************************************

void pio_rep_indword( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      long dwordCnt )

{
   unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
   unsigned int dwCnt = (unsigned int) dwordCnt;

   // Warning: Avoid calling this function with
   // dwordCnt > 8192 (transfers 32768 bytes).
   // bufSeg and bufOff should be normalized such
   // that bufOff is a value between 0 and 15 (0xf).

   #ifdef    __WATCOMC__

      PreserveAXCXDXDIES();
      ReadBlockPIOD( bufSeg, bufOff, dwCnt, dataRegAddr );
      RestoreESDIDXCXAX();

   #else

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  di
      asm   push  es

      asm   mov   ax,bufSeg
      asm   mov   es,ax
      asm   mov   di,bufOff

      asm   mov   cx,dwCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   insd

      asm   pop   es
      asm   pop   di
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

   #endif

   trc_llt( addrDataReg, 0, TRC_LLT_INSD );
}

//*************************************************************

void pio_rep_outdword( unsigned int addrDataReg,
                       unsigned int bufSeg, unsigned int bufOff,
                       long dwordCnt )

{
   unsigned int dataRegAddr = pio_reg_addrs[ addrDataReg ];
   unsigned int dwCnt = (unsigned int) dwordCnt;

   // Warning: Avoid calling this function with
   // dwordCnt > 8192 (transfers 32768 bytes).
   // bufSeg and bufOff should be normalized such
   // that bufOff is a value between 0 and 15 (0xf).

   #ifdef    __WATCOMC__

      PreserveAXCXDXSIDS();
      WriteBlockPIOD( bufSeg, bufOff, dwCnt, dataRegAddr );
      RestoreDSSIDXCXAX();

   #else

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  si
      asm   push  ds

      asm   mov   ax,bufSeg
      asm   mov   ds,ax
      asm   mov   si,bufOff

      asm   mov   cx,dwCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   outsd

      asm   pop   ds
      asm   pop   si
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

   #endif

   trc_llt( addrDataReg, 0, TRC_LLT_OUTSD );
}

// end ataiopio.c
