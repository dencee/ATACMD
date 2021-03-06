//******************************************************************************
// Simple and generic functions -- tools.C
//
// by: Daniel Commins (danielcommins@atacmd.com)
//
// File contains simple and generic functions I use in other files, but feel do
// not belong in those files.
//
//******************************************************************************

#include <stdio.h>
#include <string.h>
#include <ctype.h>      // for tolower()
#include <time.h>       // for clock() functions

#ifndef   __TOOLS_H__
#include   "tools.h"
#define   __TOOLS_H__
#endif // __TOOLS_H__

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Description: Used to consume any newline characters after functions such
//              as scanf();
//
// Input:  pFilePointer   - input stream, usually stdin
// Output: None
//------------------------------------------------------------------------------
void TOOLS_DumpLine( FILE* pFilePointer )
{
   int ch;

   // Break when EOF or newline is reached
   while ( ( ( ch = fgetc( pFilePointer ) ) != EOF ) && ( ch != '\n' ) ) {}

   return;
}

//------------------------------------------------------------------------------
// Description: Remove leading whitespace and trailing whitespace from
//              specified string. Directly modifies the input string.
//
// Input:  pInputStr    - pointer to pointer of string
// Output: None
//------------------------------------------------------------------------------
void TOOLS_RemovePadding( char** ppInputStr ) // TODO: think about making this char** const ppInputStr -DMC
{
   char* pStartPtr;
   char* pEndPtr;

   if ( ppInputStr == NULL ) {
      return;
   }

   // Remove trailing spaces
   for ( pEndPtr = ( *ppInputStr + strlen( *ppInputStr ) - 1 ); *pEndPtr == ' '; pEndPtr-- ) {}
   *( pEndPtr + 1 ) = '\0';

   // Remove leading spaces
   for ( pStartPtr = *ppInputStr; *pStartPtr == ' '; pStartPtr++ ) {}

   // Move the start of the input string to first char that's not a ' '
   *ppInputStr = pStartPtr;

   return;
}

//------------------------------------------------------------------------------
// Description: Remove trailing whitespace from specified string. Does not
//              modify the input string directly.
//
// Input:  pInputStr    - pointer to string
// Output: None
//------------------------------------------------------------------------------
void TOOLS_RemoveTrailingSpaces( char* const pInputStr )
{
   char* pEndPtr;

   if ( pInputStr == NULL ) {
      return;
   }

   // Remove trailing spaces
   for ( pEndPtr = ( pInputStr + strlen( pInputStr ) - 1 ); *pEndPtr == ' '; pEndPtr-- ) {}
   *( pEndPtr + 1 ) = '\0';

   return;
}

//------------------------------------------------------------------------------
// Description: Compare 2 strings, ignoring case.
//
// Input:  pStr1        - first string
//         pStr2        - second string
//         numToCompare - number of bytes to compare
// Output: NO_ERROR
//------------------------------------------------------------------------------
int TOOLS_StringCompareIgnoreCase( const char* pStr1, const char* pStr2, int numToCompare )
{
   int eachChar, result;

   result = ERROR;

   for ( eachChar = 0; eachChar < numToCompare; eachChar++, pStr1++, pStr2++ ) {
      result = ( tolower( *pStr1 ) - tolower( *pStr2 ) );

      // If char differs or end of string reached
      if ( ( result != 0 ) || ( *pStr1 == NULL ) ) {
         break;
      }
   }

   return ( result );
}

//------------------------------------------------------------------------------
// Description: Place current time string in specified pointer. Time format is
//              hh:mm:ss. This function is not re-entrant! DO NOT use in an
//              interrupt handler routine!
//
// Input:  ppTimerStr         - �ddress of pointer to hold time string
// Output: None
//------------------------------------------------------------------------------
void TOOLS_GetTime( char** ppTimeStr )
{
   time_t rawTime;
   static struct tm* pTimeInfo;     // must be static since it holds the actual time string!
   
   if ( ppTimeStr == NULL ) { printf( "ERROR: Input time string is NULL!" ); return; }
   
   time( &rawTime );
   pTimeInfo = localtime( &rawTime );
   
   // Date format is: Www Mmm dd hh:mm:ss yyyy   
   *ppTimeStr = ( asctime( pTimeInfo ) + strlen( "Www Mmm dd " ) );
   
   // remove trailing newline         
   *( *ppTimeStr + strlen( "hh:mm:ss" ) ) = '\0';
   
   return;
}
