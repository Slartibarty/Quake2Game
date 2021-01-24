//=================================================================================================
// A wrapper for the old CDAudio API that hooks into the soundsystem (which now supports OGG Vorbis!)
//=================================================================================================

#include "client.h"

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include "cdaudio.h"

bool CDAudio_Init()
{
	return true;
}

void CDAudio_Shutdown( void )
{
}

void CDAudio_Play( int track, bool looping )
{
	char path[MAX_QPATH];

	Q_sprintf_s( path, "sound/music/Track%02d.ogg", track );

	byte *data;
	int size = FS_LoadFile( path, (void **)&data );
	if ( !data )
	{
		Com_Printf( "Couldn't load music track %s\n", path );
		return;
	}

	int channels, samplerate, samples;
	short *output;
	samples = stb_vorbis_decode_memory( data, size, &channels, &samplerate, &output );
	FS_FreeFile( data );
	if ( samples == -1 )
	{
		Com_Printf( "Failed to decode music track %s\n", path );
		return;
	}

	S_RawSamples( 32768*4, samplerate, sizeof( short ), channels, (byte *)output );
}

void CDAudio_Stop()
{
}

void CDAudio_Update()
{
}

void CDAudio_Activate( bool active )
{
}
