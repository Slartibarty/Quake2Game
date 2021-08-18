
#include "modelbuilder_local.h"

// By the time this function is called we know several things:
//  1. The string equates to a valid file that has been opened and parsed
//  2. The string almost certainly has no slash as the last character
void Local_StripFilename( char *str )
{
	char *lastSlash = nullptr;
	for ( char *blah = str; *str; ++str )
	{
		if ( *blah == '/' )
		{
			lastSlash = blah;
		}
	}
	if ( lastSlash )
	{
		*lastSlash = '\0';
	}
}
