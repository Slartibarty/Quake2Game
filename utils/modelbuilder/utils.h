
#pragma once

// Load a file into a buffer
size_t Local_LoadFile( const char *filename, byte **buffer, size_t extradata = 0 );

void Local_StripFilename( char *str );

#ifndef _WIN32
#define _strdup strdup
#endif
