//******************************************************************************
// Header for display.c -- display.h
//
// by: Daniel Commins (dencee@gmail.com)
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
#define KEYBOARD_BACKSPACE                ( 8 )
#define KEYBOARD_CARRIAGE_RETURN          ( 13 )

// 2nd character from keyboard
#define KEYBOARD_RIGHT_ARROW              ( 77 )
#define KEYBOARD_LEFT_ARROW               ( 75 )
#define KEYBOARD_UP_ARROW                 ( 72 )
#define KEYBOARD_DOWN_ARROW               ( 80 )

//
#define BLINK_ON                          ( 1 )
#define BLINK_OFF                         ( 0 )

//----------------------------------[ENUMS]-------------------------------------

enum VideoType_t
{
    VIDEO_TYPE_NONE = 0x00,
    VIDEO_TYPE_COLOR = 0x20,
    VIDEO_TYPE_MONOCHROME = 0x30
};

//-----------------------------[Public Functions]-------------------------------

extern void Display_Initialize( void );
extern void DisplayATARegs( void );
extern void DisplayATACommandHistory( void );
extern int GetATACommandParameters( long int* pATARegisters );
extern void RestoreScreen( void );
extern void SaveScreen( void );

