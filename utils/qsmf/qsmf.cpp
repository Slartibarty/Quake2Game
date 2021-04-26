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

struct options_t
{
	char materialName[MAX_OSPATH];

	char objName[MAX_OSPATH];
	char smfName[MAX_OSPATH];
};

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
	Com_Print( "Usage: qsmf [options] input.obj output.smf\n" );
}

static void PrintHelp()
{
	Com_Print(
		"Help for qsmf:\n"
		"  -material <filename>    : The material this smf should use, defaults to materials/models/default.mat\n"
	);
}

int main( int argc, char **argv )
{
	if ( argc < 3 )
	{
		// need at least input and output
		PrintUsage();
		PrintHelp();
		return EXIT_FAILURE;
	}

	options_t options;

	Q_strcpy_s( options.materialName, "materials/models/default.mat" );

	Q_strcpy_s( options.objName, argv[argc - 2] );
	Str_FixSlashes( options.objName );
	Q_strcpy_s( options.smfName, argv[argc - 1] );
	Str_FixSlashes( options.smfName );

	int argIter;
	for ( argIter = 1; argIter < argc; ++argIter )
	{
		const char *token = argv[argIter];

		if ( Q_stricmp( token, "-material" ) == 0 )
		{
			if ( ++argIter >= argc )
			{
				Com_Printf( "Expected an argument after %s\n", token );
				return EXIT_FAILURE;
			}

			Q_strcpy_s( options.materialName, argv[argIter] );
			Str_FixSlashes( options.materialName );

			strlen_t matExtension = Q_strlen( options.materialName ) - 4;
			if ( Q_stricmp( options.materialName + matExtension, ".mat" ) != 0 )
			{
				Com_Print( "Material name doesn't end with \".mat\"\n" );
				return EXIT_FAILURE;
			}
		}
	}

	FILE *outFile = fopen( options.smfName, "wb" );
	if ( !outFile )
	{
		Com_Printf( "Couldn't open %s\n", options.smfName );
		return EXIT_FAILURE;
	}

	char *buffer;
	size_t fileSize = LoadFile( options.objName, (byte **)&buffer );
	if ( !buffer )
	{
		Com_Printf( "Couldn't open %s\n", options.objName );
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
		.materialName{} // filled below
	};

	Q_strcpy_s( header.materialName, options.materialName );

	fwrite( &header, sizeof( header ), 1, outFile );
	header.offsetVerts = (uint32)ftell( outFile );
	assert( header.offsetVerts = sizeof( header ) );
	fwrite( objReader.GetVertices(), objReader.GetVerticesSize(), 1, outFile );
	header.offsetIndices = (uint32)ftell( outFile );
	fwrite( objReader.GetIndices(), objReader.GetIndicesSize(), 1, outFile );

	fclose( outFile );

	Com_Printf( "Successfully wrote %s\n", options.smfName );
}
