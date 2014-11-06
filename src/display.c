//******************************************************************************
// Functions related to read/writing from screen memory -- display.c
//
// by: Daniel Commins (dencee@gmail.com)
//
// References:
// -----------
// http://wiki.osdev.org/Printing_To_Screen
// 
//******************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>      // for toupper()
#include <stdint.h>     // for type uint16_t

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

//---------------------------------[Defines]------------------------------------

#define SCREEN_BUFFER_NOT_INITIALIZED                          ( 0xDC )
#define VIDEO_MEM_BRIGHT_BIT                                   ( 0x08 )
#define VIDEO_MEM_BLINK_BIT                                    ( 0x80 )
#define VIDEO_MEM_COLOR_BLACK                                  ( 0 )
#define VIDEO_MEM_COLOR_BLUE                                   ( 1 )
#define VIDEO_MEM_COLOR_GREEN                                  ( 2 )
#define VIDEO_MEM_COLOR_CYAN                                   ( 3 )
#define VIDEO_MEM_COLOR_RED                                    ( 4 )
#define VIDEO_MEM_COLOR_MAGENTA                                ( 5 )
#define VIDEO_MEM_COLOR_BROWN                                  ( 6 )
#define VIDEO_MEM_COLOR_LGT_GRAY                               ( 7 )

// Note only light colors can be used for foreground. Bit 7 is for blinking/bold for background nibble.
#define VIDEO_MEM_COLOR_LGT_BLUE                               ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_BLUE )
#define VIDEO_MEM_COLOR_LGT_GREEN                              ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_GREEN )
#define VIDEO_MEM_COLOR_LGT_CYAN                               ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_CYAN )
#define VIDEO_MEM_COLOR_LGT_RED                                ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_RED )
#define VIDEO_MEM_COLOR_LGT_MAGENTA                            ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_MAGENTA )
#define VIDEO_MEM_COLOR_YELLOW                                 ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_BROWN )
#define VIDEO_MEM_COLOR_WHITE                                  ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_LGT_GRAY )

#define VIDEO_MEM_WHITE_ON_GREEN                               ( ( VIDEO_MEM_COLOR_GREEN << 4 ) | VIDEO_MEM_COLOR_WHITE )
#define VIDEO_MEM_WHITE_ON_GREEN_BLINK                         ( VIDEO_MEM_BLINK_BIT | VIDEO_MEM_WHITE_ON_GREEN )

// In number of screen lines
#define ATA_INFO_DISPLAY_HEIGHT                                ( 11 )
#define ATA_REG_DISPLAY_WIDTH                                  ( 40 )

//-----------------------------[Global Variables]-------------------------------

enum VideoType_t eVideoType;
char far* upVideoMemoryAddr;
int uScreenSavedFlag = SCREEN_NOT_SAVED;

char wcATARegString[ DOS_CHARACTERS_PER_LINE + 1 ];
char wcSavedScreen[ BYTES_PER_VIDEO_SCREEN ];

//------------------------[LOCAL FUNCTION DECLARATIONS]-------------------------

static uint16_t DetectBIOSAreaHardware( void );
static enum VideoType_t GetBIOSAreaVideoType( void );
static void SetStringInVideoMemory ( unsigned int zeroBasedLineNum, unsigned int xPosition, char* pString );
static void SetBox( unsigned int startLine, unsigned int startColumn, unsigned int numLines, unsigned int numColumns );
static void BlinkText( int blink, unsigned int lineNum, unsigned int columnNum );
static char GetTextFromScreen( unsigned int lineNum, unsigned int columnNum );

//----------------------------[LOCAL FUNCTIONS]---------------------------------

//------------------------------------------------------------------------------
// Description: Get the type of BIOS hardware
//
// Input:  None
//
// Output: Color or monochrome screen (VIDEO_TYPE_COLOR, VIDEO_TYPE_MONOCHROME)
//------------------------------------------------------------------------------
static uint16_t DetectBIOSAreaHardware()
{
    const uint16_t* pBdaDetectedHardware = (const uint16_t*) 0x410;
    return ( *pBdaDetectedHardware );
}

//------------------------------------------------------------------------------
// Description: Enable/disable blinking of 1 pixel on screen.
//
// Input:  None
//
// Output: Color or monochrome screen (VIDEO_TYPE_COLOR, VIDEO_TYPE_MONOCHROME)
//------------------------------------------------------------------------------
static enum VideoType_t GetBIOSAreaVideoType()
{
    return ( (enum VideoType_t) (DetectBIOSAreaHardware() & 0x30) );
}

//------------------------------------------------------------------------------
// Description: Get video memory starting address.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void Display_Initialize()
{
   memset( wcATARegString, 0, sizeof( wcATARegString ) );

   eVideoType = GetBIOSAreaVideoType();

   switch (eVideoType)
   {
      case VIDEO_TYPE_COLOR:
         upVideoMemoryAddr = COLOR_VID_MEM_ADDR;
         break;

      case VIDEO_TYPE_MONOCHROME:
         upVideoMemoryAddr = MONOCHROME_VID_MEM_ADDR;
         break;

      case VIDEO_TYPE_NONE:
      default:
         printf( "ERROR: Unable to detect video card type" );
         upVideoMemoryAddr = NULL;
         break;
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Enable/disable blinking of 1 pixel on screen.
//
// Input:  blink           - BLINK_ON, BLINK_OFF
//         lineNum         - row number (top border is 0)
//         columnNum       - column number (left border is 0)
//
// Output: None
//------------------------------------------------------------------------------
static void BlinkText( int blink, unsigned int lineNum, unsigned int columnNum )
{
   char far* pPixelLocation = NULL;

   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }

   if ( ( lineNum < NUMBER_OF_LINES_PER_SCREEN ) && ( columnNum < DOS_CHARACTERS_PER_LINE ) )
   {
      pPixelLocation = ( upVideoMemoryAddr + ( lineNum * DOS_BYTES_PER_LINE ) + ( columnNum * BYTES_PER_VIDEO_MEM_CHAR ) );

      if ( blink == BLINK_ON )
      {
         *( pPixelLocation + 1 ) = ( *( pPixelLocation + 1 ) | VIDEO_MEM_BLINK_BIT );
      }
      else if ( blink == BLINK_OFF )
      {
         *( pPixelLocation + 1 ) = ( *( pPixelLocation + 1 ) & ( ~VIDEO_MEM_BLINK_BIT & 0xFF ) );
      }
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Capture 1 character from screen at specified location.
//
// Input:  lineNum         - row number (top border is 0)
//         columnNum       - column number (left border is 0)
//
// Output: character at specified screen location
//------------------------------------------------------------------------------
static char GetTextFromScreen( unsigned int lineNum, unsigned int columnNum )
{
   char far* pPixelLocation = NULL;
   char character = 0;

   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return 0; }

   if ( ( lineNum < NUMBER_OF_LINES_PER_SCREEN ) && ( columnNum < DOS_CHARACTERS_PER_LINE ) )
   {
      pPixelLocation = ( upVideoMemoryAddr + ( lineNum * DOS_BYTES_PER_LINE ) + ( columnNum * BYTES_PER_VIDEO_MEM_CHAR ) );
      character = *pPixelLocation;
   }

   return character;
}

//------------------------------------------------------------------------------
// Description: Saves the current content of the screen.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void SaveScreen()
{
   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }

   memcpy( wcSavedScreen, upVideoMemoryAddr, BYTES_PER_VIDEO_SCREEN );
   uScreenSavedFlag = SCREEN_SAVED;
   
   return;
}

//------------------------------------------------------------------------------
// Description: Restores the previous content of the screen.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void RestoreScreen()
{
   if ( uScreenSavedFlag == SCREEN_SAVED )
   {
      if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }

      memcpy( upVideoMemoryAddr, wcSavedScreen, BYTES_PER_VIDEO_SCREEN );
      uScreenSavedFlag = SCREEN_NOT_SAVED;
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Prints a string to the video memory (screen).
//
// Input:  zeroBasedLineNum   - row number (top border is 0)
//         xPosition          - column number (left border is 0)
//         pString            - pointer to string to print
//
// Output: None
//------------------------------------------------------------------------------
static void SetBox( unsigned int startLine, unsigned int startColumn, unsigned int numLines, unsigned int numColumns )
{
   int startBytePosition, currLine, eachLine;
   int i;

   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }
   if ( ( numLines == 0 ) || ( numColumns == 0 ) ) { printf( "ERROR: numLines or numColumns is 0!!!" ); return; };

   startBytePosition = ( startLine * DOS_BYTES_PER_LINE ) + ( 2 * startColumn );
   currLine = startBytePosition;

   // Top left corner
   *( upVideoMemoryAddr + currLine + 0 ) = 218;
   *( upVideoMemoryAddr + currLine + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;

   // Top border
   for ( i = 0; i < ( 2 * numColumns ); i += 2 )
   {
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 0 ) = 196;
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;
   }

   // Top right corner
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 0 ) = 191;
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;

   for ( eachLine = 1; eachLine <= numLines; eachLine++ )
   {
      // Next line
      currLine = ( startBytePosition + ( eachLine * DOS_BYTES_PER_LINE ) );

      // Left side pipe
      *( upVideoMemoryAddr + currLine + 0 ) = 179;
      *( upVideoMemoryAddr + currLine + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;

      // Empty space in box
      for ( i = 0; i < ( 2 * numColumns ); i += 2 )
      {
         *( upVideoMemoryAddr + ( currLine + 2 ) + i + 0 ) = ' ';
         *( upVideoMemoryAddr + ( currLine + 2 ) + i + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;
      }

      // Right side pipe
      *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 0 ) = 179;
      *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;
   }

   // Last column on line for border
   currLine = ( startBytePosition + ( ( numLines + 1 ) * DOS_BYTES_PER_LINE ) );

   // Bottom left corner
   *( upVideoMemoryAddr + currLine + 0 ) = 192;
   *( upVideoMemoryAddr + currLine + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;

   // Bottom border
   for ( i = 0; i < ( 2 * numColumns ); i += 2 )
   {
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 0 ) = 196;
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;
   }

   // Bottom right corner
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 0 ) = 217;
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;

   return;
}

//------------------------------------------------------------------------------
// Description: Prints a string to the video memory (screen).
//
// Input:  zeroBasedLineNum   - row number (top border is 0)
//         xPosition          - column number (left border is 0)
//         pString            - pointer to string to print
//
// Output: None
//------------------------------------------------------------------------------
static void SetStringInVideoMemory ( unsigned int zeroBasedLineNum, unsigned int xPosition, char* pString )
{
   int numChar, vidMemPos, startBytePosition;
   int charPos = 0;

   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }
   if ( pString == NULL ) { printf( "ERROR: pString is NULL!!!" ); return; };

   // TODO: length check the input string -DMC

   numChar = strlen( pString );
   startBytePosition = ( zeroBasedLineNum * DOS_BYTES_PER_LINE ) + ( 2 * xPosition );

   for ( vidMemPos=0; charPos < numChar; vidMemPos+=2, charPos++ )
   {
      *( upVideoMemoryAddr + startBytePosition + vidMemPos + 0 ) = *( pString + charPos );
      *( upVideoMemoryAddr + startBytePosition + vidMemPos + 1 ) = VIDEO_MEM_WHITE_ON_GREEN;
   }

   return;
}

//----------------------------[PUBLIC FUNCTIONS]---------------------------------

//------------------------------------------------------------------------------
// Description: Displays some of the input registers for the last few commands.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void DisplayATACommandHistory()
{
   unsigned int startLine, startColumn, numLines, numColumns;
   unsigned int lastATACommandIndex, cmdEntry, numStoredATACmds, numCmdsToPrint;
   struct ATACommandEntry_t* pNextCmd = NULL;
   struct REG_CMD_INFO* pAtaRegs = NULL;

   startLine   = 0;
   startColumn = ATA_REG_DISPLAY_WIDTH;
   numLines    = ATA_INFO_DISPLAY_HEIGHT;
   numColumns  = ( DOS_CHARACTERS_PER_LINE - ATA_REG_DISPLAY_WIDTH - 2 );  // -2 for each "|" on borders

   // <startx, starty, rows, cols>
   SetBox( startLine, startColumn, numLines, numColumns );

   lastATACommandIndex = GetLastATACommandIndex();
   numStoredATACmds = GetPreviousATACommand( lastATACommandIndex )->entryNumber;

   numCmdsToPrint = ( numStoredATACmds > numLines ) ? numLines : numStoredATACmds;

   for ( cmdEntry = 1; cmdEntry <= numCmdsToPrint; cmdEntry++ )
   {
      pNextCmd = GetPreviousATACommand( ( numStoredATACmds - cmdEntry ) % MAX_STORED_ATA_COMMANDS );

      if ( pNextCmd->entryValid == VALID_ATA_ENTRY )
      {
         pAtaRegs = &( pNextCmd->ataRegs );

         memset( wcATARegString, 0, sizeof( wcATARegString ) );
         sprintf( wcATARegString, "Cmd:%02X|Fr:%02X|Sc:%02X|LBA:%04lX%04lX|E:%02X%02X", pAtaRegs->cmd, pAtaRegs->fr1, pAtaRegs->sc1, ( ( pAtaRegs->lbaLow1 >> 16 ) & 0xFFFF ) , ( pAtaRegs->lbaLow1 & 0xFFFF ), pAtaRegs->as2, pAtaRegs->er2 );
         SetStringInVideoMemory( ( startLine + ( numCmdsToPrint - cmdEntry ) + 1 ), ( startColumn + 1 ), wcATARegString );
      }
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Displays the last ATA input registers values for the last ATA
//              command sent.
//
// Input:  None
//
// Output: None
//------------------------------------------------------------------------------
void DisplayATARegs()
{
   unsigned int line, startLine, startColumn, numLines, numColumns;

   startLine   = 0;
   startColumn = 0;
   numLines    = ATA_INFO_DISPLAY_HEIGHT;
   numColumns  = ( ATA_REG_DISPLAY_WIDTH - 2 );    // -2 for "|" on each width

   // <startx, starty, rows, cols>
   SetBox( startLine, startColumn, numLines, numColumns );

   memset( wcATARegString, 0, sizeof( wcATARegString ) );
   sprintf( wcATARegString, "Registers Before   Registers After" );
   SetStringInVideoMemory( ( startLine + 1 ), ( startColumn + 1 ), wcATARegString );

   for ( line = ( startLine + 2 ); line <= ( startLine + numLines ); line++ )
   {
      // Clear string
      memset( wcATARegString, 0, sizeof( wcATARegString ) );

      switch ( ( line - ( startLine + 2 ) ) )
      {
         case CMD_REG:
            sprintf( wcATARegString, "Command: %02Xh", reg_cmd_info.cmd );
            break;

         case FEAT_REG:
            sprintf( wcATARegString, "Feat...: %02Xh", reg_cmd_info.fr1 );
            break;

         case SECC_REG:
            sprintf( wcATARegString, "SecCnt.: %02Xh", reg_cmd_info.sc1 );
            break;

         case SECN_REG:
            sprintf( wcATARegString, "SecNum.: %02Xh", reg_cmd_info.sn1 );
            break;

         case CYLL_REG:
            sprintf( wcATARegString, "CylLow.: %02Xh", reg_cmd_info.cl1 );
            break;

         case CYLH_REG:
            sprintf( wcATARegString, "CylHi..: %02Xh", reg_cmd_info.ch1 );
            break;

         case DEVH_REG:
            sprintf( wcATARegString, "DevHead: %02Xh", reg_cmd_info.dh1 );
            break;

         case DEVC_REG:
            sprintf( wcATARegString, "DevCtrl: %02Xh", reg_cmd_info.dc1 );
            break;

         case LBA_LOW:
            sprintf( wcATARegString, "LBALo..: %08lXh", ( reg_cmd_info.lbaLow1 ) );
            break;

         case LBA_HIGH:
            sprintf( wcATARegString, "LBAHi..: %08lXh", ( reg_cmd_info.lbaHigh1 ) );
            break;

         default:
            sprintf( wcATARegString, "OUT OF BOUNDS %d", ( line - ( startLine + 2 ) ));
            break;
      }

      // Write the string to video memory
      SetStringInVideoMemory( line, ( startColumn + 1 ), wcATARegString );
   }

// -----------------------------------------------------------------------------
// Second column, registers after sending command
// -----------------------------------------------------------------------------

   for ( line = ( startLine + 2 ); line <= ( startLine + numLines ); line++ )
   {
      // Clear string of remnants
      memset( wcATARegString, 0, sizeof( wcATARegString ) );

      switch ( ( line - ( startLine + 2 ) ) )
      {
         case ALT_STAT_REG:
            sprintf( wcATARegString, "AltStat.: %02Xh", reg_cmd_info.as2 );
            break;

         case ERR_REG:
            sprintf( wcATARegString, "Error...: %02Xh", reg_cmd_info.er2 );
            break;

         case SECC_REG2:
            sprintf( wcATARegString, "SecCnt..: %02Xh", reg_cmd_info.sc2 );
            break;

         case SECN_REG2:
            sprintf( wcATARegString, "SecNum..: %02Xh", reg_cmd_info.sn2 );
            break;

         case CYLL_REG2:
            sprintf( wcATARegString, "CylLow..: %02Xh", reg_cmd_info.cl2 );
            break;
            
         case CYLH_REG2:
            sprintf( wcATARegString, "CylHi...: %02Xh", reg_cmd_info.ch2 );
            break;
            
         case DEVH_REG2:
            sprintf( wcATARegString, "DevHead.: %02Xh", reg_cmd_info.dh2 );
            break;

         case LBA_LOW2:
            sprintf( wcATARegString, "LBALow..: %08lXh", reg_cmd_info.lbaLow2 );
            break;

         case LBA_HIGH2:
            sprintf( wcATARegString, "LBAHi...: %08lXh", reg_cmd_info.lbaHigh2 );
            break;

         case BYTES_XFR:
            sprintf( wcATARegString, "BytesXfr: %08lXh", reg_cmd_info.totalBytesXfer );
            break;

         default:
            sprintf( wcATARegString, "OUT OF BOUNDS %d", ( line - ( startLine + 2 ) ));
            break;
      }

      // Write string to screen
      SetStringInVideoMemory( line, ( startColumn + 20 ), wcATARegString );
   }

   return;
}

//------------------------------------------------------------------------------
// Description: GUI that allows user to input registers on screen, then reads
//              those values from screen when 'S' is pressed.
//
// Input:  pATARegisters   - pointer to an array of long ints where input
//                           register values will be stored
//
// Output: ERROR           - input registers not received successfully
//         NO_ERROR        - input registers received successfully
//------------------------------------------------------------------------------
int GetATACommandParameters( long int* pATARegisters )
{
   unsigned int startLine, startColumn, endColumn, numLines;
   unsigned int column, line;
   char wcRegInput[30];
   int inputChar, returnValue;

   if ( pATARegisters == NULL ) { printf( "ERROR: pATARegisters pointer is NULL!!!" ); return ERROR; }

   returnValue = ERROR;
   numLines    = ( ATA_INFO_DISPLAY_HEIGHT - 1 );     // line for the description is skipped
   startLine   = 2;
   startColumn = strlen( "|Command: " );
   endColumn   = 2;                                   // default is 2 hex chars

   column = startColumn;
   line = startLine;

   // Display the current/last ATA regs
   DisplayATARegs();

   // Highlight the current line for user input
   BlinkText( BLINK_ON, line, column );
   BlinkText( BLINK_ON, line, ( column + 1 ) );

   inputChar = 0;

   // Stay in loop until ESC pressed
   while ( inputChar != KEYBOARD_ESC )
   {
      // Wait for key to be hit
      while ( !kbhit() ) {}

      inputChar = getch();

      // Check valid hex letter/number input
      if ( ( ( inputChar >= 'A' ) && ( inputChar <= 'F' ) ) || ( ( inputChar >= 'a' ) && ( inputChar <= 'f' ) ) || ( ( inputChar >= '0' ) && ( inputChar <= '9' ) ) )
      {
         memset( wcRegInput, 0, sizeof( wcRegInput ) );
         sprintf( wcRegInput, "%c", toupper( inputChar ) );
         SetStringInVideoMemory( line, column, wcRegInput );

         column++;
      }
      else if ( ( inputChar == KEYBOARD_FUNCTION_KEY_FIRST ) || ( inputChar == KEYBOARD_ARROW_KEY_FIRST ) )
      {
         // Check special key pressed (2 chars)

         inputChar = getch();

         switch ( inputChar )
         {
            case KEYBOARD_UP_ARROW:
               for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
               {
                  BlinkText( BLINK_OFF, line, column );
               }

               // Set new line and column at end of user input
               line = ( line == startLine ) ? ( startLine + numLines - 1 ) : ( line - 1 );
               endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

               for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
               {
                  BlinkText( BLINK_ON, line, column );
               }

               // Reset column to beginning at very end
               column = startColumn;
               break;

            case KEYBOARD_DOWN_ARROW:
               for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
               {
                  BlinkText( BLINK_OFF, line, column );
               }

               // Set new line and column at end of user input
               line = ( line >= ( startLine + numLines - 1 ) ) ? startLine : ( line + 1 );
               endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

               for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
               {
                  BlinkText( BLINK_ON, line, column );
               }

               // Reset column to beginning at very end
               column = startColumn;
               break;

            case KEYBOARD_LEFT_ARROW:
            case KEYBOARD_BACKSPACE:
               column = startColumn;
               break;

            case KEYBOARD_RIGHT_ARROW:
//               column = ( startColumn + 1 );
               break;

            default:
               // Do nothing, unrecognized keyboard input
               break;
         }
      }
      else if ( inputChar == 'S' )
      {
         for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
         {
            BlinkText( BLINK_OFF, line, column );
         }

         // Read all the values entered from the screen, line by line
         for ( line = startLine; line < ( startLine + numLines ); line++ )
         {
            // long ints are 32-bits (4 bytes, 8 hex chars)
            endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

            for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
            {
               // TODO: BOUNDARY CHECK THIS LATER! -DMC
               // Lowest byte on screen is on the right-most column and lowest byte should be
               //  at the highest array index, e.g. 0x1234 -> [0]=1, [1]=2, [2]=3, [3]=4
               wcRegInput[ column - startColumn ] = GetTextFromScreen( line, column );
            }

            // Null terminate the string then convert string to long
            wcRegInput[ endColumn ] = '\0';
            
            *( pATARegisters + ( line - startLine ) ) = ( strtol( wcRegInput, NULL, 16 ) );
         }

         // Break and return
         returnValue = NO_ERROR;
         break;
      }

      // Go to next line after 2nd hex value is entered or enter key pressed
      if ( ( inputChar == KEYBOARD_CARRIAGE_RETURN ) || ( column > ( startColumn + endColumn - 1 ) ) )
      {
         for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
         {
            BlinkText( BLINK_OFF, line, column );
         }

         // Set new line and column at end of user input
         line = ( line >= ( startLine + numLines - 1 ) ) ? startLine : ( line + 1 );
         endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

         for ( column = startColumn; column < ( startColumn + endColumn ); column++ )
         {
            BlinkText( BLINK_ON, line, column );
         }

         // Must be done at very end as column is modified above
         column = startColumn;
      }
   }

   return ( returnValue );
}

