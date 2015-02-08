//******************************************************************************
// Functions related to read/writing from screen memory -- display.c
//
// by: Daniel Commins (danielcommins@atacmd.com)
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
#include <dos.h>        // for getvect() and setvect()

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

#define SCREEN_BUFFER_NOT_INITIALIZED        ( 0xDC )
#define VIDEO_MEM_BRIGHT_BIT                 ( 0x08 )
#define VIDEO_MEM_BLINK_BIT                  ( 0x80 )
#define VIDEO_MEM_COLOR_BLACK                ( 0 )
#define VIDEO_MEM_COLOR_BLUE                 ( 1 )
#define VIDEO_MEM_COLOR_GREEN                ( 2 )
#define VIDEO_MEM_COLOR_CYAN                 ( 3 )
#define VIDEO_MEM_COLOR_RED                  ( 4 )
#define VIDEO_MEM_COLOR_MAGENTA              ( 5 )
#define VIDEO_MEM_COLOR_BROWN                ( 6 )
#define VIDEO_MEM_COLOR_LGT_GRAY             ( 7 )

// Note only light colors can be used for foreground. Bit 7 is for blinking/bold for background nibble.
#define VIDEO_MEM_COLOR_LGT_BLUE             ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_BLUE )
#define VIDEO_MEM_COLOR_LGT_GREEN            ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_GREEN )
#define VIDEO_MEM_COLOR_LGT_CYAN             ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_CYAN )
#define VIDEO_MEM_COLOR_LGT_RED              ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_RED )
#define VIDEO_MEM_COLOR_LGT_MAGENTA          ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_MAGENTA )
#define VIDEO_MEM_COLOR_YELLOW               ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_BROWN )
#define VIDEO_MEM_COLOR_WHITE                ( VIDEO_MEM_BRIGHT_BIT | VIDEO_MEM_COLOR_LGT_GRAY )

#define VIDEO_MEM_WHITE_ON_GREEN             ( ( VIDEO_MEM_COLOR_GREEN << 4 ) | VIDEO_MEM_COLOR_WHITE )
#define VIDEO_MEM_WHITE_ON_GREEN_BLINK       ( VIDEO_MEM_BLINK_BIT | VIDEO_MEM_WHITE_ON_GREEN )
#define VIDEO_MEM_DEFAULT_COLOR              ( VIDEO_MEM_WHITE_ON_GREEN )
#define VIDEO_MEM_WHITE_ON_BLACK             ( ( VIDEO_MEM_COLOR_BLACK << 4 ) | VIDEO_MEM_COLOR_WHITE )
#define VIDEO_MEM_RED_ON_BLACK               ( ( VIDEO_MEM_COLOR_BLACK << 4 ) | VIDEO_MEM_COLOR_RED )
#define VIDEO_MEM_RED_ON_BLACK_BLINK         ( VIDEO_MEM_BLINK_BIT | VIDEO_MEM_RED_ON_BLACK )

// In number of screen lines
#define ATA_INFO_DISPLAY_HEIGHT              ( 11 )
#define ATA_REG_DISPLAY_WIDTH                ( 40 )

#define BLINK_ON                             ( 1 )
#define BLINK_OFF                            ( 0 )

// For DISPLAY_Buffer()
#define BUFFER_START_ROW                     ( 2 )
#define BUFFER_START_COL                     ( 10 )

// Clock location
#define CLOCK_INTERRUPT_VECTOR               ( 0x1C )
#define CLOCK_ROW                            ( 0 )
#define CLOCK_COLUMN                         ( DOS_CHARACTERS_PER_LINE - 8 ) // Upper right corner of screen

// clock format hh:mm:ss
#define CLOCK_HOURS_TENS_OFFSET              ( ( ( CLOCK_ROW * DOS_CHARACTERS_PER_LINE ) + CLOCK_COLUMN + 0 ) * BYTES_PER_VIDEO_MEM_CHAR )
#define CLOCK_HOURS_ONES_OFFSET              ( ( ( CLOCK_ROW * DOS_CHARACTERS_PER_LINE ) + CLOCK_COLUMN + 1 ) * BYTES_PER_VIDEO_MEM_CHAR )
#define CLOCK_MIN_TENS_OFFSET                ( ( ( CLOCK_ROW * DOS_CHARACTERS_PER_LINE ) + CLOCK_COLUMN + 3 ) * BYTES_PER_VIDEO_MEM_CHAR )
#define CLOCK_MIN_ONES_OFFSET                ( ( ( CLOCK_ROW * DOS_CHARACTERS_PER_LINE ) + CLOCK_COLUMN + 4 ) * BYTES_PER_VIDEO_MEM_CHAR )
#define CLOCK_SEC_TENS_OFFSET                ( ( ( CLOCK_ROW * DOS_CHARACTERS_PER_LINE ) + CLOCK_COLUMN + 6 ) * BYTES_PER_VIDEO_MEM_CHAR )
#define CLOCK_SEC_ONES_OFFSET                ( ( ( CLOCK_ROW * DOS_CHARACTERS_PER_LINE ) + CLOCK_COLUMN + 7 ) * BYTES_PER_VIDEO_MEM_CHAR )

//-----------------------------[Global Variables]-------------------------------

static enum VideoType_t eVideoType;
static char far* upVideoMemoryAddr;
static int uScreenSavedFlag = SCREEN_NOT_SAVED;

static char wcATARegString[ DOS_CHARACTERS_PER_LINE + 1 ];
static char wcSavedScreen[ BYTES_PER_VIDEO_SCREEN ];

static int uClockIRQInstalled;
static char uFractionTick, uTicks, uTickSecLimit;

//------------------------[LOCAL FUNCTION DECLARATIONS]-------------------------

static uint16_t DetectBIOSAreaHardware( void );
static enum VideoType_t GetBIOSAreaVideoType( void );
static void SetStringInVideoMemory ( unsigned int zeroBasedLineNum, unsigned int xPosition, const char* const pString, char color );
static void SetStringInVideoMemoryDefault( unsigned int zeroBasedLineNum, unsigned int xPosition, const char* const pString );
static void SetBox( unsigned int startLine, unsigned int startColumn, unsigned int numLines, unsigned int numColumns, char borderColor, char fillColor );
static void SetBoxDefault( unsigned int startLine, unsigned int startColumn, unsigned int numLines, unsigned int numColumns );
static void BlinkText( int blink, unsigned int lineNum, unsigned int columnNum );
static char GetTextFromScreen( unsigned int lineNum, unsigned int columnNum );
static void interrupt (* pSavedVector)( void );
static void interrupt ClockInterruptHandler( void );

//----------------------------[LOCAL FUNCTIONS]---------------------------------

//------------------------------------------------------------------------------
// Description: Get the type of BIOS hardware
//
// Input:  None
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
// Output: Color or monochrome screen (VIDEO_TYPE_COLOR, VIDEO_TYPE_MONOCHROME)
//------------------------------------------------------------------------------
static enum VideoType_t GetBIOSAreaVideoType()
{
    return ( (enum VideoType_t) (DetectBIOSAreaHardware() & 0x30) );
}

//------------------------------------------------------------------------------
// Description: Enable/disable blinking of 1 pixel on screen.
//
// Input:  blink           - BLINK_ON, BLINK_OFF
//         lineNum         - row number (top border is 0)
//         columnNum       - column number (left border is 0)
// Output: None
//------------------------------------------------------------------------------
static void BlinkText( int blink, unsigned int lineNum, unsigned int columnNum )
{
   char far* pPixelLocation = NULL;

   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }

   if ( ( lineNum < NUMBER_OF_LINES_PER_SCREEN ) && ( columnNum < DOS_CHARACTERS_PER_LINE ) )
   {
      pPixelLocation = ( upVideoMemoryAddr + ( lineNum * DOS_BYTES_PER_LINE ) + ( columnNum * BYTES_PER_VIDEO_MEM_CHAR ) );

      if ( blink == BLINK_ON ) {
         *( pPixelLocation + 1 ) = ( *( pPixelLocation + 1 ) | VIDEO_MEM_BLINK_BIT );
      } else if ( blink == BLINK_OFF ) {
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
// Description: Displays a filled in box with border.
//
// Input:  startLine          - starting y-position
//         startColumn        - starting x-postion
//         numLines           - height of box
//         numColumns         - width of box
//         borderColor        - color of box border
//         fillColor          - color inside box
// Output: None
//------------------------------------------------------------------------------
static void SetBox( unsigned int startLine, unsigned int startColumn, unsigned int numLines, unsigned int numColumns, char borderColor, char fillColor )
{
   int startBytePosition, currLine, eachLine;
   int i;

   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }
   if ( ( numLines == 0 ) || ( numColumns == 0 ) ) { printf( "ERROR: numLines or numColumns is 0!!!" ); return; };

   startBytePosition = ( startLine * DOS_BYTES_PER_LINE ) + ( 2 * startColumn );
   currLine = startBytePosition;

   // Top left corner
   *( upVideoMemoryAddr + currLine + 0 ) = 218;
   *( upVideoMemoryAddr + currLine + 1 ) = borderColor;

   // Top border
   for ( i = 0; i < ( 2 * numColumns ); i += 2 ) {
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 0 ) = 196;
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 1 ) = borderColor;
   }

   // Top right corner
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 0 ) = 191;
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 1 ) = borderColor;

   for ( eachLine = 1; eachLine <= numLines; eachLine++ ) {
      // Next line
      currLine = ( startBytePosition + ( eachLine * DOS_BYTES_PER_LINE ) );

      // Left side pipe
      *( upVideoMemoryAddr + currLine + 0 ) = 179;
      *( upVideoMemoryAddr + currLine + 1 ) = borderColor;

      // Empty space in box
      for ( i = 0; i < ( 2 * numColumns ); i += 2 ) {
         *( upVideoMemoryAddr + ( currLine + 2 ) + i + 0 ) = ' ';
         *( upVideoMemoryAddr + ( currLine + 2 ) + i + 1 ) = fillColor;
      }

      // Right side pipe
      *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 0 ) = 179;
      *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 1 ) = borderColor;
   }

   // Last column on line for border
   currLine = ( startBytePosition + ( ( numLines + 1 ) * DOS_BYTES_PER_LINE ) );

   // Bottom left corner
   *( upVideoMemoryAddr + currLine + 0 ) = 192;
   *( upVideoMemoryAddr + currLine + 1 ) = borderColor;

   // Bottom border
   for ( i = 0; i < ( 2 * numColumns ); i += 2 ) {
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 0 ) = 196;
      *( upVideoMemoryAddr + ( currLine + 2 ) + i + 1 ) = borderColor;
   }

   // Bottom right corner
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 0 ) = 217;
   *( upVideoMemoryAddr + ( currLine + 2 ) + ( 2 * numColumns ) + 1 ) = borderColor;

   return;
}

//------------------------------------------------------------------------------
// Description: Displays a filled in box with border and default color.
//
// Input:  startLine          - starting y-position
//         startColumn        - starting x-postion
//         numLines           - height of box
//         numColumns         - width of box
// Output: None
//------------------------------------------------------------------------------
static void SetBoxDefault( unsigned int startLine, unsigned int startColumn, unsigned int numLines, unsigned int numColumns )
{
   SetBox( startLine, startColumn, numLines, numColumns, VIDEO_MEM_DEFAULT_COLOR, VIDEO_MEM_DEFAULT_COLOR );
}

//------------------------------------------------------------------------------
// Description: Prints a string to the video memory (screen).
//
// Input:  zeroBasedLineNum   - row number (top border is 0)
//         xPosition          - column number (left border is 0)
//         pString            - pointer to string to print
//         color              - color of text
// Output: None
//------------------------------------------------------------------------------
static void SetStringInVideoMemory( unsigned int zeroBasedLineNum, unsigned int xPosition, const char* const pString, char color )
{
   int numChar, vidMemPos, startBytePosition;
   int charPos = 0;

   if ( upVideoMemoryAddr == NULL ) { printf( "ERROR: video memory address pointer is NULL!!!" ); return; }
   if ( pString == NULL ) { printf( "ERROR: pString is NULL!!!" ); return; };

   // TODO: length check the input string -DMC

   numChar = strlen( pString );
   startBytePosition = ( zeroBasedLineNum * DOS_BYTES_PER_LINE ) + ( 2 * xPosition );

   for ( vidMemPos=0; charPos < numChar; vidMemPos+=2, charPos++ ) {
      *( upVideoMemoryAddr + startBytePosition + vidMemPos + 0 ) = *( pString + charPos );
      *( upVideoMemoryAddr + startBytePosition + vidMemPos + 1 ) = color;
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Prints a string of default color to the video memory (screen).
//
// Input:  zeroBasedLineNum   - row number (top border is 0)
//         xPosition          - column number (left border is 0)
//         pString            - pointer to string to print
// Output: None
//------------------------------------------------------------------------------
static void SetStringInVideoMemoryDefault( unsigned int zeroBasedLineNum, unsigned int xPosition, const char* const pString )
{
   SetStringInVideoMemory( zeroBasedLineNum, xPosition, pString, VIDEO_MEM_DEFAULT_COLOR );
}

//------------------------------------------------------------------------------
// Description: Interrupt handler that updates the current time displayed on
//              screen. Current time must already be displayed on sceen,
//              i.e. DISPLAY_InstallClock() must have been called.
//
// Input:  zeroBasedLineNum   - row number (top border is 0)
//         xPosition          - column number (left border is 0)
//         pString            - pointer to string to print
// Output: None
//------------------------------------------------------------------------------
static void interrupt ClockInterruptHandler()
{
   if ( upVideoMemoryAddr == NULL ) { return; }

   ++uTicks;

   if ( uTicks > uTickSecLimit ) {
      uTicks = 0;

      ++uFractionTick;

      // INT fires off every 18.2 seconds so every 5 entries account for extra second
      if ( uFractionTick == 5 ) {
         uFractionTick = 0;
         uTickSecLimit = 18;
      } else {
         uTickSecLimit = 17;
      }

      // Update second's ones place
      *( upVideoMemoryAddr + CLOCK_SEC_ONES_OFFSET ) = ( *( upVideoMemoryAddr + CLOCK_SEC_ONES_OFFSET ) + 1 );

      if ( *( upVideoMemoryAddr + CLOCK_SEC_ONES_OFFSET ) > '9' ) {
         *( upVideoMemoryAddr + CLOCK_SEC_ONES_OFFSET ) = '0';

         // Update second's tens place
         *( upVideoMemoryAddr + CLOCK_SEC_TENS_OFFSET ) = ( *( upVideoMemoryAddr + CLOCK_SEC_TENS_OFFSET ) + 1 );

         if ( *( upVideoMemoryAddr + CLOCK_SEC_TENS_OFFSET ) > '5') {
            *( upVideoMemoryAddr + CLOCK_SEC_TENS_OFFSET ) = '0';

            // Update minute's ones place
            *( upVideoMemoryAddr + CLOCK_MIN_ONES_OFFSET ) = ( *( upVideoMemoryAddr + CLOCK_MIN_ONES_OFFSET ) + 1 );

            if ( *( upVideoMemoryAddr + CLOCK_MIN_ONES_OFFSET ) > '9' ) {
               *( upVideoMemoryAddr + CLOCK_MIN_ONES_OFFSET ) = '0';

               // Update minute's tens place
               *( upVideoMemoryAddr + CLOCK_MIN_TENS_OFFSET ) = ( *( upVideoMemoryAddr + CLOCK_MIN_TENS_OFFSET ) + 1 );

               if ( *( upVideoMemoryAddr + CLOCK_MIN_TENS_OFFSET ) > '5' ) {
                  *( upVideoMemoryAddr + CLOCK_MIN_TENS_OFFSET ) = '0';

                  // Update hour's ones place
                  *( upVideoMemoryAddr + CLOCK_HOURS_ONES_OFFSET ) = ( *( upVideoMemoryAddr + CLOCK_HOURS_ONES_OFFSET ) + 1 );

                  if ( *( upVideoMemoryAddr + CLOCK_HOURS_ONES_OFFSET ) > '9' ) {
                     *( upVideoMemoryAddr + CLOCK_HOURS_ONES_OFFSET ) = '0';

                     *( upVideoMemoryAddr + CLOCK_HOURS_TENS_OFFSET ) = ( *( upVideoMemoryAddr + CLOCK_HOURS_TENS_OFFSET ) + 1 );

                  } else if ( ( *( upVideoMemoryAddr + CLOCK_HOURS_TENS_OFFSET ) == '2' ) && ( *( upVideoMemoryAddr + CLOCK_HOURS_ONES_OFFSET ) == '4' ) ) {
                     *( upVideoMemoryAddr + CLOCK_HOURS_ONES_OFFSET ) = '0';
                     *( upVideoMemoryAddr + CLOCK_HOURS_TENS_OFFSET ) = '0';
                  }
               }
            }
         }
      }
   }
}

//----------------------------[PUBLIC FUNCTIONS]---------------------------------

//------------------------------------------------------------------------------
// Description: Get video memory starting address.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void DISPLAY_Initialize()
{
   memset( wcATARegString, 0, sizeof( wcATARegString ) );
   uClockIRQInstalled = OFF;

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
// Description: Save existing clock ISR routine and install ours.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
int DISPLAY_InstallClock()
{
   char* pTimeStr;

   // Error if already enabled
   if ( uClockIRQInstalled == ON ) {
      return ( ERROR );
   }

   // Display the current time
   TOOLS_GetTime( &pTimeStr );
   SetStringInVideoMemoryDefault( CLOCK_ROW, CLOCK_COLUMN, pTimeStr );

   // Disable interrupts.
   // Save the original interrupt vector.
   // Install the clock interrupt handler.
   // Enable interrupts.

   _DISABLE();
   pSavedVector = _GETVECT( CLOCK_INTERRUPT_VECTOR );
   _SETVECT( CLOCK_INTERRUPT_VECTOR, ClockInterruptHandler );
   _ENABLE();

   // Set global install flag at very end
   uClockIRQInstalled = ON;

   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Restore the saved ISR routine for the clock interrupt.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
int DISPLAY_UninstallClock()
{
   // Error if not enabled
   if ( uClockIRQInstalled == OFF ) {
      return ( ERROR );
   }

   // Disable interrupts.
   // Restore the system's interrupt vector.
   // Enable interrupts.

   _DISABLE();
   _SETVECT( CLOCK_INTERRUPT_VECTOR, pSavedVector );
   _ENABLE();

   // Clear global install flag at very end
   uClockIRQInstalled = OFF;

   return ( NO_ERROR );
}

//------------------------------------------------------------------------------
// Description: Wait for user input.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void DISPLAY_Pause()
{
   int ch;

   // Clear any queued up keys
   while ( kbhit() ) {
      ch = getch();

      if ( ( ch == KEYBOARD_FUNCTION_KEY_FIRST ) || ( ch == KEYBOARD_ARROW_KEY_FIRST ) )
         getch();
   }

   // Pause until key hit
   printf( "Press any key to continue...\n" );
   fflush( stdout );

   // Wait for a key to be pressed
   while ( !kbhit() ) {}

   ch = getch();

   if ( ( ch == KEYBOARD_ARROW_KEY_FIRST ) || ( ch == KEYBOARD_FUNCTION_KEY_FIRST ) ) {
      // Get 2nd input char
      getch();
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Saves the current content of the screen.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void DISPLAY_SaveScreen()
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
// Output: None
//------------------------------------------------------------------------------
void DISPLAY_RestoreScreen()
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
// Description: Displays the number of specified bytes from the specified
//              buffer in bytes or 16-bit words.
//
// Input:  pInBuffer       - pointer to buffer with data
//         numberOfBytes   - number of bytes to print
//         printType       - PRINT_BYTE (XX XX XX), PRINT_WORD (XXXX XXXX XXXX)
// Output: None
//------------------------------------------------------------------------------
void DISPLAY_Buffer( const void* const pInBuffer, unsigned long numberOfSectors, int printType )
{
   char* pBuffer = NULL;
   char wPageNum[ 20 + 1 ];
   int inputChar, eachSect, curByte, curWord, nextNib;

   // Check for valid parameters
   if ( pInBuffer == NULL ) { printf( "ERROR: pInBuffer is NULL!!!" ); return; }
   if ( numberOfSectors == 0 ) { printf( "ERROR: number of sectors entered = %d", numberOfSectors ); return; }

   eachSect = 0;
   curByte = 0;
   curWord = 0;
   nextNib = 0;
   pBuffer = (char *)pInBuffer;

   // Save current screen data
   DISPLAY_SaveScreen();

   while ( 1 )
   {
      // Full screen box
      SetBoxDefault( 0, 0, ( NUMBER_OF_LINES_PER_SCREEN - 2 ), ( DOS_CHARACTERS_PER_LINE - 2 ) );

      switch ( printType )
      {
         case PRINT_BYTE:
         {
            int byte, row, col;
            int bytesPerRow;
            unsigned long byteOffset;

            bytesPerRow = 23;
            row = 0;
            col = 0;

            while ( 1 )
            {
               sprintf( wPageNum, "%3d/%3d", ( eachSect + 1 ), numberOfSectors );
               row = 0;
               col = ( ( DOS_CHARACTERS_PER_LINE / 2 ) - ( strlen( wPageNum ) / 2 ) );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               row++;

               col = 1;
               sprintf( wPageNum, "offset  " );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               col += strlen( "offset  " );
               for ( byte = 0; byte < bytesPerRow; byte++ ) {
                  sprintf( wPageNum, " %2X", byte );
                  SetStringInVideoMemoryDefault( row, col, wPageNum );
                  col += 3;
               }
               row++;

               col = 1;
               byteOffset = ( eachSect * 512 );
               sprintf( wPageNum, "%08lX ", byteOffset );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               col += 9;
               sprintf( wPageNum, "%02X", *( pBuffer + byteOffset + 0 ) );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               col += 2;

               for ( byte = 1; byte < 512; byte++ )
               {
                  // 2x + ( x - 1 ) <= 80; x <= 27
                  if ( ( byte % bytesPerRow ) == 0 ) {
                     row++;
                     col = 1;
                     sprintf( wPageNum, "%08X ", ( ( eachSect * 512 ) + byte ) );
                     SetStringInVideoMemoryDefault( row, col, wPageNum );
                     col += 9;
                  } else {
                     col++;
                  }

                  byteOffset = ( ( eachSect * 512 ) + byte );
                  sprintf( wPageNum, "%02X", *( pBuffer + byteOffset ) );
                  SetStringInVideoMemoryDefault( row, col, wPageNum );
                  col += 2;
               }

               // Blink on the current selected byte
               row = ( BUFFER_START_ROW + ( curByte / bytesPerRow ) );
               col = ( BUFFER_START_COL + ( ( curByte % bytesPerRow ) * 3 ) );
               BlinkText( BLINK_ON, row, ( col + 0 ) );
               BlinkText( BLINK_ON, row, ( col + 1 ) );

               // Wait for key to be hit
               while ( !kbhit() ) {}

               inputChar = getch();

               // Break on escape
               if ( ( inputChar == KEYBOARD_ESC ) || ( inputChar == KEYBOARD_TAB ) ) {
                  break;
               } else if ( ( inputChar == KEYBOARD_ARROW_KEY_FIRST ) || ( inputChar == KEYBOARD_FUNCTION_KEY_FIRST ) ) {

                  // Get 2nd input char
                  inputChar = getch();
                  nextNib = 0;

                  if ( inputChar == KEYBOARD_PAGE_UP ) {
                     if ( eachSect == 0 ) {
                        eachSect = ( numberOfSectors - 1 );
                     } else {
                        eachSect--;
                     }
                  } else if ( inputChar == KEYBOARD_PAGE_DOWN ) {
                     if ( eachSect == ( numberOfSectors - 1 ) ) {
                        eachSect = 0;
                     } else {
                        eachSect++;
                     }
                  } else if ( inputChar == KEYBOARD_UP_ARROW ) {
                     if ( curByte >= bytesPerRow ) {
                        curByte -= bytesPerRow;
                     }
                  } else if ( inputChar == KEYBOARD_DOWN_ARROW ) {
                     if ( curByte < ( 512 - bytesPerRow ) ) {
                        curByte += bytesPerRow;
                     }
                  } else if ( inputChar == KEYBOARD_LEFT_ARROW ) {
                     if ( ( curByte % bytesPerRow ) != 0 ) {
                        curByte--;
                     }
                  } else if ( inputChar == KEYBOARD_RIGHT_ARROW ) {
                     if ( ( ( curByte % bytesPerRow ) != ( bytesPerRow - 1 ) ) && ( curWord != 511 ) ) {
                        curByte++;
                     }
                  }
               } else if ( ( ( inputChar >= 'A' ) && ( inputChar <= 'F' ) ) || ( ( inputChar >= 'a' ) && ( inputChar <= 'f' ) ) || ( ( inputChar >= '0' ) && ( inputChar <= '9' ) ) ) {
                  int value;
                  int byteOfs = ( ( eachSect * 512 ) + curByte );

                  if ( ( inputChar >= '0' ) && ( inputChar <= '9' ) ) {
                     value = (int)( (char)inputChar - '0' );
                  } else {
                     value = ( 10 + (int)( (char)toupper( inputChar ) - 'A' ) );
                  }

                  // Fills in HOB to LOB
                  if ( nextNib == 0 ) {
                     *( pBuffer + byteOfs ) = ( ( value << 4 ) | ( 0x0F & *( pBuffer + byteOfs ) ) );
                  } else if ( nextNib == 1 ) {
                     *( pBuffer + byteOfs ) = ( value | ( 0xF0 & *( pBuffer + byteOfs ) ) );
                  }

                  // Increment & wrap around
                  nextNib = (++nextNib % 2 );
               }
            }
            break;
         }

         case PRINT_WORD:
         {
            int word, row, col;
            int wordsPerRow;
            unsigned long byteOffset;

            wordsPerRow = 14;
            row = 0;
            col = 0;

            while ( 1 )
            {
               sprintf( wPageNum, "%3d/%3d", ( eachSect + 1 ), numberOfSectors );
               row = 0;
               col = ( ( DOS_CHARACTERS_PER_LINE / 2 ) - ( strlen( wPageNum ) / 2 ) );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               row++;

               col = 1;
               sprintf( wPageNum, "offset  " );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               col += strlen( "offset  " );
               for ( word = 0; word < wordsPerRow; word++ ) {
                  sprintf( wPageNum, " %4X", word );
                  SetStringInVideoMemoryDefault( row, col, wPageNum );
                  col += 5;
               }
               row++;

               col = 1;
               byteOffset = ( eachSect * 512 );
               sprintf( wPageNum, "%08lX ", byteOffset );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               col += 9;
               sprintf( wPageNum, "%02X%02X", *( pBuffer + byteOffset + 1 ), *( pBuffer + byteOffset + 0 ) );
               SetStringInVideoMemoryDefault( row, col, wPageNum );
               col += 4;

               for ( word = 1; word < ( 512 / 2 ); word++ )
               {
                  // 4x + ( x - 1 ) <= 80; x <= 16
                  if ( ( word % wordsPerRow ) == 0 ) {
                     row++;
                     col = 1;
                     sprintf( wPageNum, "%08X ", ( ( eachSect * 512 ) + word ) );
                     SetStringInVideoMemoryDefault( row, col, wPageNum );
                     col += 9;
                  } else {
                     col++;
                  }

                  byteOffset = ( ( eachSect * 512 ) + ( word * 2 ) );
                  sprintf( wPageNum, "%02X%02X", *( pBuffer + byteOffset + 1 ), *( pBuffer + byteOffset + 0 ) );
                  SetStringInVideoMemoryDefault( row, col, wPageNum );
                  col += 4;
               }

               // Blink on the current selected word
               row = ( BUFFER_START_ROW + ( curWord / wordsPerRow ) );
               col = ( BUFFER_START_COL + ( ( curWord % wordsPerRow ) * 5 ) );
               BlinkText( BLINK_ON, row, ( col + 0 ) );
               BlinkText( BLINK_ON, row, ( col + 1 ) );
               BlinkText( BLINK_ON, row, ( col + 2 ) );
               BlinkText( BLINK_ON, row, ( col + 3 ) );

               // Wait for key to be hit
               while ( !kbhit() ) {}

               inputChar = getch();

               // Break on escape
               if ( ( inputChar == KEYBOARD_ESC ) || ( inputChar == KEYBOARD_TAB ) ) {
                  break;
               } else if ( ( inputChar == KEYBOARD_ARROW_KEY_FIRST ) || ( inputChar == KEYBOARD_FUNCTION_KEY_FIRST ) ) {

                  // Get 2nd input char
                  inputChar = getch();
                  nextNib = 0;

                  if ( inputChar == KEYBOARD_PAGE_UP ) {
                     if ( eachSect == 0 ) {
                        eachSect = ( numberOfSectors - 1 );
                     } else {
                        eachSect--;
                     }
                  } else if ( inputChar == KEYBOARD_PAGE_DOWN ) {
                     if ( eachSect == ( numberOfSectors - 1 ) ) {
                        eachSect = 0;
                     } else {
                        eachSect++;
                     }
                  } else if ( inputChar == KEYBOARD_UP_ARROW ) {
                     if ( curWord >= wordsPerRow ) {
                        curWord -= wordsPerRow;
                     }
                  } else if ( inputChar == KEYBOARD_DOWN_ARROW ) {
                     if ( curWord < ( ( 512 / 2 ) - wordsPerRow ) ) {
                        curWord += wordsPerRow;
                     }
                  } else if ( inputChar == KEYBOARD_LEFT_ARROW ) {
                     if ( ( curWord % wordsPerRow ) != 0 ) {
                        curWord--;
                     }
                  } else if ( inputChar == KEYBOARD_RIGHT_ARROW ) {
                     if ( ( ( curWord % wordsPerRow ) != ( wordsPerRow - 1 ) ) && ( curWord != 255 ) ) {
                        curWord++;
                     }
                  }
               } else if ( ( ( inputChar >= 'A' ) && ( inputChar <= 'F' ) ) || ( ( inputChar >= 'a' ) && ( inputChar <= 'f' ) ) || ( ( inputChar >= '0' ) && ( inputChar <= '9' ) ) ) {
                  int value;
                  int byteOfs = ( ( eachSect * 512 ) + ( curWord * 2 ) );

                  if ( ( inputChar >= '0' ) && ( inputChar <= '9' ) ) {
                     value = (int)( (char)inputChar - '0' );
                  } else {
                     value = ( 10 + (int)( (char)toupper( inputChar ) - 'A' ) );
                  }

                  // Fills in HOB to LOB
                  if ( nextNib == 0 ) {
                     *( pBuffer + byteOfs + 1 ) = ( ( value << 4 ) | ( 0x0F & *( pBuffer + byteOfs + 1 ) ) );
                  } else if ( nextNib == 1 ) {
                     *( pBuffer + byteOfs + 1 ) = ( value | ( 0xF0 & *( pBuffer + byteOfs + 1 ) ) );
                  } else if ( nextNib == 2 ) {
                     *( pBuffer + byteOfs + 0 ) = ( ( value << 4 ) | ( 0x0F & *( pBuffer + byteOfs + 0 ) ) );
                  } else if ( nextNib == 3 ) {
                     *( pBuffer + byteOfs + 0 ) = ( value | ( 0xF0 & *( pBuffer + byteOfs + 0 ) ) );
                  }

                  // Increment & wrap around
                  nextNib = (++nextNib % 4 );
               }
            }
            break;
         }
         default:
            printf( "ERROR: Invalid print parameter!!!\n" );
            break;
      }

      // Switch the print types
      if ( inputChar == KEYBOARD_TAB ) {
         if ( printType == PRINT_BYTE ) {
            printType = PRINT_WORD;
            curWord = ( curByte / 2 );
            nextNib = 0;
         } else {
            printType = PRINT_BYTE;
            curByte = ( curWord * 2 );
            nextNib = 0;
         }
      }

      if ( inputChar == KEYBOARD_ESC ) {
         break;
      }
   }

   DISPLAY_RestoreScreen();

   return;
}

//------------------------------------------------------------------------------
// Description: Displays some of the input registers for the last few commands.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void DISPLAY_ATACommandHistory()
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
   SetBoxDefault( startLine, startColumn, numLines, numColumns );

   lastATACommandIndex = ATAIOREG_GetLastATACommandIndex();
   numStoredATACmds = ATAIOREG_GetPreviousATACommand( lastATACommandIndex )->entryNumber;

   numCmdsToPrint = ( numStoredATACmds > numLines ) ? numLines : numStoredATACmds;

   for ( cmdEntry = 1; cmdEntry <= numCmdsToPrint; cmdEntry++ )
   {
      pNextCmd = ATAIOREG_GetPreviousATACommand( ( numStoredATACmds - cmdEntry ) % MAX_STORED_ATA_COMMANDS );

      if ( pNextCmd->entryValid == VALID_ATA_ENTRY )
      {
         pAtaRegs = &( pNextCmd->ataRegs );

         memset( wcATARegString, 0, sizeof( wcATARegString ) );
         sprintf( wcATARegString, "Cmd:%02X|Fr:%02X|Sc:%02X|LBA:%04lX%04lX|E:%02X%02X", pAtaRegs->cmd, pAtaRegs->fr1, pAtaRegs->sc1, ( ( pAtaRegs->lbaLow1 >> 16 ) & 0xFFFF ) , ( pAtaRegs->lbaLow1 & 0xFFFF ), pAtaRegs->as2, pAtaRegs->er2 );
         SetStringInVideoMemoryDefault( ( startLine + ( numCmdsToPrint - cmdEntry ) + 1 ), ( startColumn + 1 ), wcATARegString );
      }
   }

   return;
}

//------------------------------------------------------------------------------
// Description: Displays the last ATA input registers values for the last ATA
//              command sent.
//
// Input:  None
// Output: None
//------------------------------------------------------------------------------
void DISPLAY_ATARegs()
{
   unsigned int line, startLine, startColumn, numLines, numColumns;

   startLine   = 0;
   startColumn = 0;
   numLines    = ATA_INFO_DISPLAY_HEIGHT;
   numColumns  = ( ATA_REG_DISPLAY_WIDTH - 2 );    // -2 for "|" on each width

   // <startx, starty, rows, cols>
   SetBoxDefault( startLine, startColumn, numLines, numColumns );

   memset( wcATARegString, 0, sizeof( wcATARegString ) );
   sprintf( wcATARegString, "Registers Before   Registers After" );
   SetStringInVideoMemoryDefault( ( startLine + 1 ), ( startColumn + 1 ), wcATARegString );

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
      SetStringInVideoMemoryDefault( line, ( startColumn + 1 ), wcATARegString );
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
//            sprintf( wcATARegString, "EC/DRQs.: %04X%04Xh", ( reg_cmd_info.ec & 0xFFFF ), ( reg_cmd_info.drqPackets & 0xFFFF ) );
            break;

         case BYTES_XFR:
            sprintf( wcATARegString, "BytesXfr: %08lXh", reg_cmd_info.totalBytesXfer );
            break;

         default:
            sprintf( wcATARegString, "OUT OF BOUNDS %d", ( line - ( startLine + 2 ) ));
            break;
      }

      // Write string to screen
      SetStringInVideoMemoryDefault( line, ( startColumn + 20 ), wcATARegString );
   }

   return;
}

//------------------------------------------------------------------------------
// Description: GUI that allows user to input registers on screen, then reads
//              those values from screen when 'S' is pressed.
//
// Input:  pATARegisters   - pointer to an array of long ints where input
//                           register values will be stored
// Output: ERROR           - input registers not received successfully
//         NO_ERROR        - input registers received successfully
//------------------------------------------------------------------------------
int DISPLAY_GetATACommandParameters( long int* pATARegisters )
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
   DISPLAY_ATARegs();

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
         SetStringInVideoMemoryDefault( line, column, wcRegInput );

         column++;
      }
      else if ( ( inputChar == KEYBOARD_FUNCTION_KEY_FIRST ) || ( inputChar == KEYBOARD_ARROW_KEY_FIRST ) )
      {
         // Check special key pressed (2 chars)
         inputChar = getch();

         switch ( inputChar )
         {
            case KEYBOARD_UP_ARROW:
               for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
                  BlinkText( BLINK_OFF, line, column );
               }

               // Set new line and column at end of user input
               line = ( line == startLine ) ? ( startLine + numLines - 1 ) : ( line - 1 );
               endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

               for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
                  BlinkText( BLINK_ON, line, column );
               }

               // Reset column to beginning at very end
               column = startColumn;
               break;

            case KEYBOARD_DOWN_ARROW:
               for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
                  BlinkText( BLINK_OFF, line, column );
               }

               // Set new line and column at end of user input
               line = ( line >= ( startLine + numLines - 1 ) ) ? startLine : ( line + 1 );
               endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

               for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
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

//            case KEYBOARD_F4:
               // Display command codes
//               break;

            default:
               // Do nothing, unrecognized keyboard input
               break;
         }
      }
      else if ( inputChar == 'S' )
      {
         for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
            BlinkText( BLINK_OFF, line, column );
         }

         // Read all the values entered from the screen, line by line
         for ( line = startLine; line < ( startLine + numLines ); line++ )
         {
            // long ints are 32-bits (4 bytes, 8 hex chars)
            endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

            for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
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
         for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
            BlinkText( BLINK_OFF, line, column );
         }

         // Set new line and column at end of user input
         line = ( line >= ( startLine + numLines - 1 ) ) ? startLine : ( line + 1 );
         endColumn = ( ( line == ( startLine + LBA_LOW ) ) || ( line == startLine + LBA_HIGH ) ) ? 8 : 2;

         for ( column = startColumn; column < ( startColumn + endColumn ); column++ ) {
            BlinkText( BLINK_ON, line, column );
         }

         // Must be done at very end as column is modified above
         column = startColumn;
      }
   }

   return ( returnValue );
}

//------------------------------------------------------------------------------
// Description: Intro page for ATAErase.c that shows the available drives for
//              erasing.
//
// Input:  None
// Output: ON     - exit program
//         OFF    - continue program
//------------------------------------------------------------------------------
int DISPLAY_ATAErase()
{
   int eachDevice, numDevices, row, col;
   int exitProgram, eraseableDevices;

   eraseableDevices = 0;
   exitProgram = ON;

   // Full screen box
   SetBox( 0, 0, ( NUMBER_OF_LINES_PER_SCREEN - 2 ), ( DOS_CHARACTERS_PER_LINE - 2 ), VIDEO_MEM_WHITE_ON_BLACK, VIDEO_MEM_WHITE_ON_BLACK );

   // Title
   SetStringInVideoMemory( 1, 1, "ATA Erase v1.0", VIDEO_MEM_WHITE_ON_BLACK );
   SetStringInVideoMemory( 2, 1, "------------------------------------------------------------------------------", VIDEO_MEM_WHITE_ON_BLACK );

   // Left and right 3-way intersections
   *( upVideoMemoryAddr + ( 2 * DOS_BYTES_PER_LINE ) + 0 ) = 195;          // left
   *( upVideoMemoryAddr + ( 2 * DOS_BYTES_PER_LINE ) + 1 ) = VIDEO_MEM_WHITE_ON_BLACK;
   *( upVideoMemoryAddr + ( 3 * DOS_BYTES_PER_LINE ) - 2 + 0 ) = 180;      // right
   *( upVideoMemoryAddr + ( 3 * DOS_BYTES_PER_LINE ) - 2 + 1 ) = VIDEO_MEM_WHITE_ON_BLACK;

   // Find drives
   numDevices = ScanForStorageDevices();

   if ( numDevices == 0 ) {
      SetStringInVideoMemory( ( NUMBER_OF_LINES_PER_SCREEN / 2 ), ( ( DOS_CHARACTERS_PER_LINE - 8 ) / 2 ), "No Devices Found!", VIDEO_MEM_RED_ON_BLACK_BLINK );

      // Wait for key to be hit
      while ( !kbhit() ) {}

      return ( ON );
   }

   // Display found drives
   SetStringInVideoMemory( 3, 1, "Dev# | Base | bus/dev/fun | Eraseable | Model# [Serial#]", VIDEO_MEM_WHITE_ON_BLACK );
   SetStringInVideoMemory( 4, 1, "-----+------+-------------+-----------+------------------------------", VIDEO_MEM_WHITE_ON_BLACK );
   row = 4;

   for ( eachDevice = 0; eachDevice < numDevices; eachDevice++ )
   {
      struct StorageDevice_t* pDeviceInfo = GetDeviceInfo( eachDevice );

      if ( ( pDeviceInfo != NULL ) && ( pDeviceInfo->valid == VALID_DEVICE_ENTRY ) )
      {
         char wcLineChars[ DOS_CHARACTERS_PER_LINE ];
         int secSupport;

         col = 1;   // column just after the border of the box
         row++;

         // Setup device I/O ports for ID command. DOES NOT send data to the device.
         SetActiveDevice( eachDevice );

         // Print device model string
         memset( wcLineChars, 0, sizeof( wcLineChars ) );
         sprintf( wcLineChars, "  %2d |", ( eachDevice + 1 ) );
         SetStringInVideoMemory( row, col, wcLineChars, VIDEO_MEM_WHITE_ON_BLACK );
         col += strlen( wcLineChars );

         memset( wcLineChars, 0, sizeof( wcLineChars ) );
         sprintf( wcLineChars, " %04Xh|", pDeviceInfo->cmdBase );
         SetStringInVideoMemory( row, col, wcLineChars, VIDEO_MEM_WHITE_ON_BLACK );
         col += strlen( wcLineChars );

         if ( ( pDeviceInfo->busNum == IGNORE_VALUE ) && ( pDeviceInfo->devNum == IGNORE_VALUE ) && ( pDeviceInfo->funNum == IGNORE_VALUE ) )
         {
            SetStringInVideoMemory( row, 19, "   --/--/--  |", VIDEO_MEM_WHITE_ON_BLACK );
            col += 18;
         }
         else
         {
            memset( wcLineChars, 0, sizeof( wcLineChars ) );
            sprintf( wcLineChars, "  %02Xh/%02Xh/%01dh |", pDeviceInfo->busNum, pDeviceInfo->devNum, pDeviceInfo->funNum );
            SetStringInVideoMemory( row, col, wcLineChars, VIDEO_MEM_WHITE_ON_BLACK );
            col += strlen( wcLineChars );
         }

         IdentifyDevice();

         // Eraseable? i.e. security feature set support?
         memset( wcLineChars, 0, sizeof( wcLineChars ) );
         secSupport = ( GetIDWord( upBufferPtr, ( 128 * 2 ) ) & 0x1 );
         if ( secSupport != 0 ){
            eraseableDevices++;
            sprintf( wcLineChars, "    Yes    |" );
         } else {
            sprintf( wcLineChars, "    No     |" );
         }
         SetStringInVideoMemory( row, col, wcLineChars, VIDEO_MEM_WHITE_ON_BLACK );
         col += strlen( wcLineChars );

         // Model #
         memset( wcLineChars, 0, sizeof( wcLineChars ) );
         GetModelString( upBufferPtr, wcLineChars, sizeof( wcLineChars ) );
         SetStringInVideoMemory( row, col, " ", VIDEO_MEM_WHITE_ON_BLACK );
         col++;
         SetStringInVideoMemory( row, col, wcLineChars, VIDEO_MEM_WHITE_ON_BLACK );
         col += strlen( wcLineChars );
         SetStringInVideoMemory( row, col, " ", VIDEO_MEM_WHITE_ON_BLACK );
         col++;

         // Serial #
         memset( wcLineChars, 0, sizeof( wcLineChars ) );
         GetSerialNumber( upBufferPtr, wcLineChars, sizeof( wcLineChars ) );
         SetStringInVideoMemory( row, col, "[", VIDEO_MEM_WHITE_ON_BLACK );
         col++;
         SetStringInVideoMemory( row, col, wcLineChars, VIDEO_MEM_WHITE_ON_BLACK );
         col += strlen( wcLineChars );
         SetStringInVideoMemory( row, col, "]", VIDEO_MEM_WHITE_ON_BLACK );
      }
   }

   if ( eraseableDevices == 0 ) {
      // No drives can be erased!
   
      SetStringInVideoMemory( ( NUMBER_OF_LINES_PER_SCREEN / 2 ), ( DOS_CHARACTERS_PER_LINE / 4 ), "No devices found that support Security Erase!", VIDEO_MEM_RED_ON_BLACK_BLINK );

      // Wait for key to be hit
      while ( !kbhit() ) {}
      
      // Exit
      return ( ON );
   } else {
      int inputChar;
      char wcLineChars[ DOS_CHARACTERS_PER_LINE ];
      char wcInputStr[3];
      
      row += 2;
      col = 1;
      memset( wcLineChars, 0, sizeof( wcLineChars ) );
      sprintf( wcLineChars, "Erase all %d drives (Y/N)?", eraseableDevices );
      SetStringInVideoMemory( row, col, wcLineChars, VIDEO_MEM_WHITE_ON_BLACK );
      col += 26;
      
      inputChar = getch();

      memset( wcInputStr, 0, sizeof( wcInputStr ) );
      sprintf( wcInputStr, "%c", (char)inputChar );
      SetStringInVideoMemory( row, col, wcInputStr, VIDEO_MEM_WHITE_ON_BLACK );

      // All input besides 'y' will enable exit program flag
      if ( ( inputChar == 'Y' ) || ( inputChar == 'y' ) ) {
         exitProgram = OFF;
      }
      else if ( ( inputChar == KEYBOARD_FUNCTION_KEY_FIRST ) || ( inputChar == KEYBOARD_ARROW_KEY_FIRST ) ) {
         // Grab implicit second input
         getch();
      }
   }
   
   return ( exitProgram );
}
