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

extern void DumpLine( FILE* pFilePointer );
extern void RemovePadding( char** ppInputStr );
extern void RemoveTrailingSpaces( char* const pInputStr );
extern int StringCompareIgnoreCase( const char* pStr1, const char* pStr2, int numToCompare );
