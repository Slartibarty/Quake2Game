//-------------------------------------------------------------------------------------------------
// Functions related to drawing 2D imagery
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"

static constexpr auto ConChars_Name = "materials/pics/conchars" MAT_EXT;

material_t *draw_chars;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_InitLocal( void )
{
	// load console characters
	draw_chars = GL_FindMaterial( ConChars_Name );
	if ( !draw_chars->IsOkay() ) {
		// This is super aggressive, but it easily warns about bad game folders
		Com_Printf( "Could not get console font: %s", ConChars_Name );
	}
}

//-------------------------------------------------------------------------------------------------
// Draws one 8*8 graphics character with 0 being transparent.
// It can be clipped to the top of the screen to allow the console to be
// smoothly scrolled off.
//-------------------------------------------------------------------------------------------------
void Draw_Char( int x, int y, int num )
{
	int row, col;
	float frow, fcol, size;

	num &= 255;

	if ( ( num & 127 ) == 32 )
		return;		// space

	if ( y <= -8 )
		return;		// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	draw_chars->Bind();

	glBegin( GL_QUADS );
	glTexCoord2f( fcol, frow );
	glVertex2i( x, y );
	glTexCoord2f( fcol + size, frow );
	glVertex2i( x + 8, y );
	glTexCoord2f( fcol + size, frow + size );
	glVertex2i( x + 8, y + 8 );
	glTexCoord2f( fcol, frow + size );
	glVertex2i( x, y + 8 );
	glEnd();
}

//-------------------------------------------------------------------------------------------------
// Input is a filename with no decoration
// We want to phase this function out eventually
//-------------------------------------------------------------------------------------------------
material_t *Draw_FindPic( const char *name )
{
	char fullname[MAX_QPATH];
	Q_sprintf_s( fullname, "materials/pics/%s.mat", name );
	assert( !strstr( fullname, "//" ) ); // Check for double slashes

	return GL_FindMaterial( fullname );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_GetPicSize( int *w, int *h, const char *pic )
{
	material_t *mat;

	mat = Draw_FindPic( pic );
	if ( !mat )
	{
		*w = *h = -1;
		return;
	}
	*w = mat->image->width;
	*h = mat->image->height;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_StretchPic( int x, int y, int w, int h, const char *pic )
{
	material_t *mat;
	image_t *img;

	mat = Draw_FindPic( pic );
	if ( !mat )
	{
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	img = mat->image;

	mat->Bind();

	glBegin( GL_QUADS );
	glTexCoord2f( img->sl, img->tl );
	glVertex2i( x, y );
	glTexCoord2f( img->sh, img->tl );
	glVertex2i( x + w, y );
	glTexCoord2f( img->sh, img->th );
	glVertex2i( x + w, y + h );
	glTexCoord2f( img->sl, img->th );
	glVertex2i( x, y + h );
	glEnd();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_Pic( int x, int y, const char *pic )
{
	material_t *mat;
	image_t *img;

	mat = Draw_FindPic( pic );
	if ( !mat )
	{
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	img = mat->image;

	mat->Bind();

	glBegin( GL_QUADS );
	glTexCoord2f( img->sl, img->tl );
	glVertex2i( x, y );
	glTexCoord2f( img->sh, img->tl );
	glVertex2i( x + img->width, y );
	glTexCoord2f( img->sh, img->th );
	glVertex2i( x + img->width, y + img->height );
	glTexCoord2f( img->sl, img->th );
	glVertex2i( x, y + img->height );
	glEnd();
}

//-------------------------------------------------------------------------------------------------
// This repeats a 64*64 tile graphic to fill the screen around a sized down
// refresh window.
//-------------------------------------------------------------------------------------------------
void Draw_TileClear( int x, int y, int w, int h, const char *pic )
{
	material_t *mat;
	image_t *img;

	mat = Draw_FindPic( pic );
	if ( !mat )
	{
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	img = mat->image;

	mat->Bind();

	glBegin( GL_QUADS );
	glTexCoord2f( x / 64.0f, y / 64.0f );
	glVertex2i( x, y );
	glTexCoord2f( ( x + w ) / 64.0f, y / 64.0f );
	glVertex2i( x + w, y );
	glTexCoord2f( ( x + w ) / 64.0f, ( y + h ) / 64.0f );
	glVertex2i( x + w, y + h );
	glTexCoord2f( x / 64.0f, ( y + h ) / 64.0f );
	glVertex2i( x, y + h );
	glEnd();
}

//-------------------------------------------------------------------------------------------------
// Fills a box of pixels with a single color
//-------------------------------------------------------------------------------------------------
void Draw_Fill( int x, int y, int w, int h, int c )
{
	union
	{
		uint	c;
		byte	v[4];
	} color;

	if ( c > 255 )
		Com_Error( ERR_FATAL, "Draw_Fill: bad color" );

	glDisable( GL_TEXTURE_2D );

	color.c = d_8to24table[c];
	glColor3f( color.v[0] / 255.0f,
		color.v[1] / 255.0f,
		color.v[2] / 255.0f );

	glBegin( GL_QUADS );

	glVertex2i( x, y );
	glVertex2i( x + w, y );
	glVertex2i( x + w, y + h );
	glVertex2i( x, y + h );

	glEnd();
	glColor3f( 1.0f, 1.0f, 1.0f );
	glEnable( GL_TEXTURE_2D );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_FadeScreen( void )
{
	glEnable( GL_BLEND );
	glDisable( GL_TEXTURE_2D );
	glColor4f( 0.0f, 0.0f, 0.0f, 0.8f );
	glBegin( GL_QUADS );

	glVertex2i( 0, 0 );
	glVertex2i( r_newrefdef.width, 0 );
	glVertex2i( r_newrefdef.width, r_newrefdef.height );
	glVertex2i( 0, r_newrefdef.height );

	glEnd();
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
}

//-------------------------------------------------------------------------------------------------

extern unsigned	r_rawpalette[256];

//-------------------------------------------------------------------------------------------------
// Used for rendering cinematic frames
//-------------------------------------------------------------------------------------------------
void Draw_StretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data )
{
	uint		image32[256 * 256];
	int			i, j, trows;
	byte		*source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	if ( rows <= 256 )
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows / 256.0f;
		trows = 256;
	}
	t = rows * hscale / 256;

	{
		uint *dest;

		for ( i = 0; i < trows; i++ )
		{
			row = (int)( i * hscale );
			if ( row > rows )
				break;
			source = data + cols * row;
			dest = &image32[i * 256];
			fracstep = cols * 0x10000 / 256;
			frac = fracstep >> 1;
			for ( j = 0; j < 256; j++ )
			{
				dest[j] = r_rawpalette[source[frac >> 16]];
				frac += fracstep;
			}
		}
	}

	GLuint id;

	glGenTextures( 1, &id );
	GL_Bind( id );

	// Upload frame
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32 );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glBegin( GL_QUADS );
	glTexCoord2f( 0.0f, 0.0f );
	glVertex2i( x, y );
	glTexCoord2f( 1.0f, 0.0f );
	glVertex2i( x + w, y );
	glTexCoord2f( 1.0f, t );
	glVertex2i( x + w, y + h );
	glTexCoord2f( 0.0f, t );
	glVertex2i( x, y + h );
	glEnd();

	GL_Bind( 0 );

	// Delete frame
	glDeleteTextures( 1, &id );
}
