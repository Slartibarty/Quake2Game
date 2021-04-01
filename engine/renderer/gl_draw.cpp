//-------------------------------------------------------------------------------------------------
// Functions related to drawing 2D imagery
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"

#include "meshbuilder.h"

static constexpr auto ConChars_Name = "materials/pics/conchars" MAT_EXT;

struct guiDrawCmd_t
{
	material_t *material;
	uint32 offset, count;
};

material_t *draw_chars;

static qGUIMeshBuilder s_drawMeshBuilder;

static GLuint s_drawVAO;
static GLuint s_drawVBO;
static GLuint s_drawEBO;

std::vector<guiDrawCmd_t> s_drawCmds;

// Used so we can chain consecutive function calls
static guiDrawCmd_t	*s_currentCmd;
static uint32		s_lastOffset;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_Init()
{
	// load console characters
	draw_chars = GL_FindMaterial( ConChars_Name, true );
	if ( !draw_chars->IsOkay() ) {
		// we NEED the console font
		Com_FatalErrorf( "Could not get console font: %s", ConChars_Name );
	}

	// reserve 64 draw commands
	s_drawCmds.reserve( 64 );

	glGenVertexArrays( 1, &s_drawVAO );
	glGenBuffers( 1, &s_drawVBO );
	glGenBuffers( 1, &s_drawEBO );

	glBindVertexArray( s_drawVAO );
	glBindBuffer( GL_ARRAY_BUFFER, s_drawVBO );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_drawEBO );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );

	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( guiVertex_t ), (void *)( 0 ) );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( guiVertex_t ), (void *)( 2 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( guiVertex_t ), (void *)( 4 * sizeof( GLfloat ) ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_Shutdown()
{
	glDeleteBuffers( 1, &s_drawEBO );
	glDeleteBuffers( 1, &s_drawVBO );
	glDeleteVertexArrays( 1, &s_drawVAO );
}

//-------------------------------------------------------------------------------------------------
// Checks for or creates a draw chain
//-------------------------------------------------------------------------------------------------
static void Draw_CheckChain( material_t *material )
{
	if ( !s_currentCmd ) {
		// this is our first run for this frame
		guiDrawCmd_t &cmd = s_drawCmds.emplace_back();

		cmd.material = material;
		cmd.offset = 0;
		cmd.count = 8;

		assert( s_drawMeshBuilder.GetIndexArrayCount() == 8 );

		s_lastOffset = 8;

		s_currentCmd = &cmd;
	}
	else if ( s_currentCmd->material == material ) {
		// use the existing chain
		guiDrawCmd_t &cmd = *s_currentCmd;

		cmd.count += 8; // eight more indices
		s_lastOffset += 8;
	}
	else {
		// start a new chain
		guiDrawCmd_t &cmd = s_drawCmds.emplace_back();

		cmd.material = material;
		cmd.offset = s_lastOffset;
		cmd.count = 8;

		s_lastOffset += 8;

		s_currentCmd = &cmd;
	}
}

//-------------------------------------------------------------------------------------------------
// Draws one graphics character with 0 being transparent.
// It can be clipped to the top of the screen to allow the console to be
// smoothly scrolled off.
//-------------------------------------------------------------------------------------------------
void Draw_Char( int x, int y, int ch )
{
#if 1
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ( ch & 127 ) == 32 ) {
		// space
		return;
	}

	if ( y < -CONCHAR_HEIGHT ) {
		// totally off screen
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	guiRect_t rect;

	rect.v1.Position2f( x, y );
	rect.v1.TexCoord2f( fcol, frow );
	rect.v1.Color1f( 255 );

	rect.v2.Position2f( x + CONCHAR_WIDTH, y );
	rect.v2.TexCoord2f( fcol + size, frow );
	rect.v2.Color1f( 255 );

	rect.v3.Position2f( x + CONCHAR_WIDTH, y + CONCHAR_HEIGHT );
	rect.v3.TexCoord2f( fcol + size, frow + size );
	rect.v3.Color1f( 255 );

	rect.v4.Position2f( x, y + CONCHAR_HEIGHT );
	rect.v4.TexCoord2f( fcol, frow + size );
	rect.v4.Color1f( 255 );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( draw_chars );
#endif
}

//-------------------------------------------------------------------------------------------------
// Input is a filename with no decoration
// We want to phase this function out eventually
//-------------------------------------------------------------------------------------------------
material_t *R_RegisterPic( const char *name )
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

	mat = R_RegisterPic( pic );
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
static void Draw_Generic( int x, int y, int w, int h, material_t *mat )
{
	guiRect_t rect;

	int alpha = static_cast<int>( mat->alpha * 255.0f );

	rect.v1.Position2f( x, y );
	rect.v1.TexCoord2f( 0, 0 );
	rect.v1.Color2f( 255, alpha );

	rect.v2.Position2f( x + w, y );
	rect.v2.TexCoord2f( 1, 0 );
	rect.v2.Color2f( 255, alpha );

	rect.v3.Position2f( x + w, y + h );
	rect.v3.TexCoord2f( 1, 1 );
	rect.v3.Color2f( 255, alpha );

	rect.v4.Position2f( x, y + h );
	rect.v4.TexCoord2f( 0, 1 );
	rect.v4.Color2f( 255, alpha );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( mat );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_StretchPic( int x, int y, int w, int h, const char *pic )
{
	material_t *mat;

	mat = R_RegisterPic( pic );
	if ( !mat ) {
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	Draw_Generic( x, y, w, h, mat );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_Pic( int x, int y, const char *pic )
{
	material_t *mat;

	mat = R_RegisterPic( pic );
	if ( !mat ) {
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	Draw_Generic( x, y, mat->image->width, mat->image->height, mat );
}

//-------------------------------------------------------------------------------------------------
// This repeats a 64*64 tile graphic to fill the screen around a sized down
// refresh window.
//-------------------------------------------------------------------------------------------------
void Draw_TileClear( int x, int y, int w, int h, const char *pic )
{
#if 1
	material_t *mat;

	mat = R_RegisterPic( pic );
	if ( !mat )
	{
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	guiRect_t rect;

	float alpha = mat->alpha;

	rect.v1.Position2f( x, y );
	rect.v1.TexCoord2f( x / 64.0f, y / 64.0f );
	rect.v1.Color2f( 1.0f, alpha );

	rect.v2.Position2f( x + w, y );
	rect.v2.TexCoord2f( ( x + w ) / 64.0f, y / 64.0f );
	rect.v2.Color2f( 1.0f, alpha );

	rect.v3.Position2f( x + w, y + h );
	rect.v3.TexCoord2f( ( x + w ) / 64.0f, ( y + h ) / 64.0f );
	rect.v3.Color2f( 1.0f, alpha );

	rect.v4.Position2f( x, y + h );
	rect.v4.TexCoord2f( x / 64.0f, ( y + h ) / 64.0f );
	rect.v4.Color2f( 1.0f, alpha );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( mat );
#endif
}

//-------------------------------------------------------------------------------------------------
// Fills a box of pixels with a single color
//-------------------------------------------------------------------------------------------------
void R_DrawFilled( float x, float y, float w, float h, qColor color )
{
	guiRect_t rect;

	rect.v1.Position2f( x, y );
	rect.v1.TexCoord2f( 0.0f, 0.0f );
	rect.v1.Color( color );

	rect.v2.Position2f( x + w, y );
	rect.v2.TexCoord2f( 1.0f, 0.0f );
	rect.v2.Color( color );

	rect.v3.Position2f( x + w, y + h );
	rect.v3.TexCoord2f( 1.0f, 1.0f );
	rect.v3.Color( color );

	rect.v4.Position2f( x, y + h );
	rect.v4.TexCoord2f( 0.0f, 1.0f );
	rect.v4.Color( color );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( whiteMaterial );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_PolyBlend( const vec4_t color )
{
#if 1
	if ( !r_polyblend->value || color[3] <= 0.0f )
		return;

	guiRect_t rect;

	rect.v1.Position2f( 0.0f, 0.0f );
	rect.v1.TexCoord2f( 0.0f, 0.0f );
	rect.v1.Color4fv( color );

	rect.v2.Position2f( r_newrefdef.width, 0.0f );
	rect.v2.TexCoord2f( 1.0f, 0.0f );
	rect.v2.Color4fv( color );

	rect.v3.Position2f( r_newrefdef.width, r_newrefdef.height );
	rect.v3.TexCoord2f( 1.0f, 1.0f );
	rect.v3.Color4fv( color );

	rect.v4.Position2f( 0.0f, r_newrefdef.height );
	rect.v4.TexCoord2f( 0.0f, 1.0f );
	rect.v4.Color4fv( color );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( whiteMaterial );
#endif
}

//-------------------------------------------------------------------------------------------------
// Batching
//-------------------------------------------------------------------------------------------------
void Draw_RenderBatches()
{
	// render all gui draw calls

	if ( s_drawMeshBuilder.HasData() ) {

		// profiling code
	//	double begin = Time_FloatMilliseconds();

		// set up the render state
		DirectX::XMMATRIX orthoMatrix = DirectX::XMMatrixOrthographicOffCenterRH( 0.0f, vid.width, vid.height, 0.0f, -1.0f, 1.0f );

		glUseProgram( glProgs.guiProg );
		glUniformMatrix4fv( 3, 1, GL_FALSE, (float *)&orthoMatrix );
		glUniform1i( 4, 0 );

		glBindVertexArray( s_drawVAO );
		glBindBuffer( GL_ARRAY_BUFFER, s_drawVBO );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, s_drawEBO );

		glBufferData( GL_ARRAY_BUFFER, s_drawMeshBuilder.GetVertexArraySize(), s_drawMeshBuilder.GetVertexArray(), GL_STREAM_DRAW );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, s_drawMeshBuilder.GetIndexArraySize(), s_drawMeshBuilder.GetIndexArray(), GL_STREAM_DRAW );

		// these will be re-enabled next frame
		glDisable( GL_DEPTH_TEST );
	//	glDisable( GL_CULL_FACE );
	//	glDisable( GL_BLEND );

		glEnable( GL_BLEND );
		glEnable( GL_PRIMITIVE_RESTART_FIXED_INDEX );

		for ( const auto &cmd : s_drawCmds )
		{
			cmd.material->Bind();

			glDrawElements( GL_TRIANGLES, cmd.count, GL_UNSIGNED_SHORT, (void *)( (intptr_t)cmd.offset * sizeof( uint16 ) ) );
		}

		glDisable( GL_PRIMITIVE_RESTART_FIXED_INDEX );

		glUseProgram( 0 );

		glBindVertexArray( 0 );

		s_drawMeshBuilder.Reset();
		s_drawCmds.clear();
		s_currentCmd = nullptr;
		s_lastOffset = 0;

		// profiling code
	//	double end = Time_FloatMilliseconds();
	//	Com_Printf( "Draw_RenderBatches: %.6f milliseconds\n", end - begin );
	}
}

//-------------------------------------------------------------------------------------------------

static unsigned s_rawPalette[256];

/*
========================
R_SetRawPalette

Sets the palette used by Draw_StretchRaw
========================
*/
void R_SetRawPalette( const unsigned char *palette )
{
	// We don't use the raw palette for anything but cinematics, so ignore NULL calls
	if ( !palette )
		return;

	int i;
	byte *rp = (byte *)s_rawPalette;

	for ( i = 0; i < 256; i++ )
	{
		rp[i*4+0] = palette[i*3+0];
		rp[i*4+1] = palette[i*3+1];
		rp[i*4+2] = palette[i*3+2];
		rp[i*4+3] = 0xff;
	}

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT );
	glClearColor( DEFAULT_CLEARCOLOR );
}

//-------------------------------------------------------------------------------------------------
// Used for rendering cinematic frames
//-------------------------------------------------------------------------------------------------
void Draw_StretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data )
{
#if 0
	static uint	image32[256 * 256];
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
				dest[j] = s_rawPalette[source[frac >> 16]];
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
#endif
}
