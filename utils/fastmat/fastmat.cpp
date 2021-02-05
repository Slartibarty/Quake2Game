//
// The slart-o-matic
//

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include <filesystem>

static constexpr auto MatFormat = "{\n\t$basetexture %s\n}\n";

namespace filesys = std::filesystem;

// Param 1 = old copyright
// Param 2 = new copyright
// Param 3 = folder to recursively search
//
int wmain( int argc, wchar_t** argv )
{
	if ( argc != 2 ) {
		wprintf(
			L"Error: Too few arguments!\n"
			L"Usage: fastmat <folder>\n"
		);
		return 1;
	}
	
	// A std path for our root folder
	const filesys::path pthRoot( argv[1] );

	// For every sub-object
	for ( const auto& entry : filesys::recursive_directory_iterator( pthRoot ) )
	{
		// If we're a file
		if ( entry.is_regular_file() )
		{
			// If we end with .cpp or .h
			if ( entry.path().extension().compare( L".png" ) == 0 ) //||
			//	 entry.path().extension().compare( L".tga" ) == 0 )
			{
				wprintf( L"Working on %s\n", entry.path().c_str() );

				auto newname = entry.path();

				newname.replace_extension(".mat");

				FILE* handle = _wfopen( newname.c_str(), L"wb" );

				fprintf(handle, MatFormat, entry.path().string().c_str());

				fclose( handle );
			}
		}
	}

	wprintf( L"Done!\n" );

	return 0;
}
