//******************************************************************************
// Simple and generic functions header -- tools.h
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
//******************************************************************************

//---------------------------------[DEFINES]------------------------------------

#define TRUE                                    ( 1 )
#define FALSE                                   ( 0 )

#define OFF                                     ( 0 )
#define ON                                      ( 1 )

#define ERROR                                   ( 1 )
#define NO_ERROR                                ( 0 )

//--------------------------[FUNCTION DECLARATIONS]-----------------------------

extern void TOOLS_DumpLine( FILE* pFilePointer );
extern void TOOLS_RemovePadding( char** ppInputStr );
extern void TOOLS_RemoveTrailingSpaces( char* const pInputStr );
extern int TOOLS_StringCompareIgnoreCase( const char* pStr1, const char* pStr2, int numToCompare );
extern void TOOLS_GetTime( char** ppTimeStr );
