//******************************************************************************
// Header for display.c -- display.h
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
//******************************************************************************

//---------------------------------[DEFINES]------------------------------------

#define SCREEN_SAVED                      ( 1 )
#define SCREEN_NOT_SAVED                  ( 0xFF )
#define COLOR_VID_MEM_ADDR                (char far *)0xB0008000
#define MONOCHROME_VID_MEM_ADDR           (char far *)0xB0000000
#define NUMBER_OF_LINES_PER_SCREEN        ( 25 )   // default, could vary
#define BYTES_PER_VIDEO_MEM_CHAR          ( 2 )
#define DOS_CHARACTERS_PER_LINE           ( 80 )   // default, could vary
#define DOS_BYTES_PER_LINE                ( BYTES_PER_VIDEO_MEM_CHAR * DOS_CHARACTERS_PER_LINE )
#define BYTES_PER_VIDEO_SCREEN            ( NUMBER_OF_LINES_PER_SCREEN * DOS_BYTES_PER_LINE )

#define KEYBOARD_ARROW_KEY_FIRST          ( 224 ) // 0xE0
#define KEYBOARD_FUNCTION_KEY_FIRST       ( 0 )
#define KEYBOARD_ESC                      ( 27 )
#define KEYBOARD_SPACEBAR                 ( 32 )
#define KEYBOARD_BACKSPACE                ( 8 )
#define KEYBOARD_CARRIAGE_RETURN          ( 13 )
#define KEYBOARD_TAB                      ( (int)'\t' )

// 2nd character from keyboard
#define KEYBOARD_RIGHT_ARROW              ( 77 )
#define KEYBOARD_LEFT_ARROW               ( 75 )
#define KEYBOARD_UP_ARROW                 ( 72 )
#define KEYBOARD_DOWN_ARROW               ( 80 )
#define KEYBOARD_PAGE_UP                  ( 73 )
#define KEYBOARD_HOME                     ( 71 )
#define KEYBOARD_END                      ( 79 )
#define KEYBOARD_PAGE_DOWN                ( 81 )
#define KEYBOARD_F1                       ( 59 )
#define KEYBOARD_F2                       ( 60 )
#define KEYBOARD_F3                       ( 61 )
#define KEYBOARD_F4                       ( 62 )
#define KEYBOARD_F5                       ( 63 )
#define KEYBOARD_F6                       ( 64 )
#define KEYBOARD_F7                       ( 65 )
#define KEYBOARD_F8                       ( 66 )
#define KEYBOARD_F9                       ( 67 )
#define KEYBOARD_F10                      ( 68 )

//----------------------------------[ENUMS]-------------------------------------

enum VideoType_t
{
    VIDEO_TYPE_NONE = 0x00,
    VIDEO_TYPE_COLOR = 0x20,
    VIDEO_TYPE_MONOCHROME = 0x30
};

//-----------------------------[Public Functions]-------------------------------

extern void DISPLAY_Initialize( void );
extern int  DISPLAY_ATAErase( void );
extern void DISPLAY_Buffer( const void* const pInBuffer, unsigned long numberOfSectors, int printType );
extern void DISPLAY_ATARegs( void );
extern void DISPLAY_ATACommandHistory( void );
extern int  DISPLAY_GetATACommandParameters( long int* pATARegisters );
extern void DISPLAY_Pause( void );
extern void DISPLAY_RestoreScreen( void );
extern void DISPLAY_SaveScreen( void );
extern int  DISPLAY_InstallClock( void );
extern int  DISPLAY_UninstallClock( void );

