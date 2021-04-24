/*
===================================================================================================

	QSMF
	23/04/2021
	Slartibarty

	Builds an SMF (Static Mesh File) from an OBJ

===================================================================================================
*/

#include "../../core/core.h"
#include "../../common/q_formats.h"

#include "obj_reader.h"

static size_t GetFileSize( FILE *handle )
{
	long oldpos, newpos;

	oldpos = ftell( handle );
	fseek( handle, 0, SEEK_END );
	newpos = ftell( handle );
	fseek( handle, oldpos, SEEK_SET );

	return static_cast<size_t>( newpos );
}

// Load a file into a buffer
static size_t LoadFile( const char *filename, byte **buffer, size_t extradata = 0 )
{
	FILE *handle = fopen( filename, "rb" );
	if ( !handle ) {
		*buffer = nullptr;
		return 0;
	}

	size_t bufsize = GetFileSize( handle ) + extradata;

	*buffer = reinterpret_cast<byte *>( malloc( bufsize ) );
	fread( *buffer, 1, bufsize, handle );
	fclose( handle );

	if ( extradata > 0 )
	{
		memset( *buffer + bufsize - extradata, 0, extradata );
	}

	return bufsize;
}

static void PrintUsage()
{
	Com_Print( "Usage: qsmf input.obj output.smf\n" );
}

int main( int argc, char **argv )
{
	if ( argc != 3 )
	{
		PrintUsage();
		return EXIT_FAILURE;
	}

	const char *filename = argv[1];
	const char *outName = argv[2];

	FILE *outFile = fopen( outName, "wb" );
	if ( !outFile )
	{
		Com_Printf( "Couldn't open %s\n", outName );
		return EXIT_FAILURE;
	}

	char *buffer;
	size_t fileSize = LoadFile( filename, (byte **)&buffer );
	if ( !buffer )
	{
		Com_Printf( "Couldn't open %s\n", filename );
		return EXIT_FAILURE;
	}

	OBJReader objReader( buffer );

	free( buffer );

	fmtSMF::header_t header
	{
		.fourCC = fmtSMF::fourCC,
		.version = fmtSMF::version,
		.numVerts = (uint32)objReader.GetNumVertices(),
		.offsetVerts = sizeof( header ),
		.numIndices = (uint32)objReader.GetNumIndices(),
		.offsetIndices = (uint32)( sizeof( header ) + objReader.GetVerticesSize() ),
		.materialName{ "materials/models/default.mat" } // null
	};

	fwrite( &header, sizeof( header ), 1, outFile );
	header.offsetVerts = (uint32)ftell( outFile );
	assert( header.offsetVerts = sizeof( header ) );
	fwrite( objReader.GetVertices(), objReader.GetVerticesSize(), 1, outFile );
	header.offsetIndices = (uint32)ftell( outFile );
	fwrite( objReader.GetIndices(), objReader.GetIndicesSize(), 1, outFile );

	fclose( outFile );

	Com_Printf( "Successfully wrote %s\n", outName );
}
