
#include "gl_local.h"

#include "meshbuilder.h"

/*
===================================================================================================

	2D draw functions

	All calls to R_Draw functions are collected in a buffer, then at the end of the frame
	they're uploaded to the GPU and rendered.
	Internal functions are prefixed with Draw_, but public ones are R_Draw, this is to maintain
	consistency with the rest of the public renderer functions.

===================================================================================================
*/

static constexpr auto ConCharsName = "materials/pics/conchars" MAT_EXT;

struct guiDrawCmd_t
{
	material_t *material;
	uint32 offset, count;
};

struct simpleVertex_t
{
	vec3_t pos;
	uint32 rgba;
};

static_assert( sizeof( simpleVertex_t ) == 16 );

static material_t *s_drawChars;

static GUIMeshBuilder s_drawMeshBuilder;

static GLuint s_drawVAO, s_drawVBO, s_drawEBO;
static GLuint s_lineVAO, s_lineVBO;

static std::vector<guiDrawCmd_t> s_drawCmds;
static std::vector<simpleVertex_t> s_lineVertices;
static std::vector<simpleVertex_t> s_polyVertices;

// Used so we can chain consecutive function calls
static guiDrawCmd_t *s_currentCmd;
static uint32 s_lastOffset;

static void Draw_InitGUIStuff()
{
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

static void Draw_InitLineStuff()
{
	glGenVertexArrays( 1, &s_lineVAO );
	glGenBuffers( 1, &s_lineVBO );

	glBindVertexArray( s_lineVAO );
	glBindBuffer( GL_ARRAY_BUFFER, s_lineVBO );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( simpleVertex_t ), (void *)( 0 ) );
	glVertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( simpleVertex_t ), (void *)( 3 * sizeof( GLfloat ) ) );

}

/*
========================
Draw_Init

Grabs the console font and sets up our buffers
========================
*/
void Draw_Init()
{
	// load console characters
	s_drawChars = GL_FindMaterial( ConCharsName, true );
	if ( !s_drawChars->IsOkay() ) {
		// we NEED the console font
		Com_FatalErrorf( "Could not find font: %s\n", ConCharsName );
	}

	Draw_InitGUIStuff();
	Draw_InitLineStuff();
}

/*
========================
Draw_Shutdown
========================
*/
void Draw_Shutdown()
{
	glDeleteBuffers( 1, &s_lineVBO );
	glDeleteVertexArrays( 1, &s_lineVAO );

	glDeleteBuffers( 1, &s_drawEBO );
	glDeleteBuffers( 1, &s_drawVBO );
	glDeleteVertexArrays( 1, &s_drawVAO );
}

/*
========================
Draw_CheckChain

Checks for or creates a draw chain
========================
*/
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

/*
========================
R_RegisterPic

Input is a filename with no decoration
We want to phase this function out eventually
========================
*/
material_t *R_RegisterPic( const char *name )
{
	char fullname[MAX_QPATH];
	Q_sprintf_s( fullname, "materials/pics/%s.mat", name );
	assert( !strstr( fullname, "//" ) ); // Check for double slashes

	return GL_FindMaterial( fullname );
}

/*
========================
R_DrawGetPicSize
========================
*/
void R_DrawGetPicSize( int *w, int *h, const char *pic )
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

/*
========================
R_DrawCharColor
========================
*/
void R_DrawCharColor( int x, int y, int ch, uint32 color )
{
	// bound to 0 - 255
	ch &= 255;

	// temp: bound to 0 - 127
	// all chars after 127 are legacy coloured variations
	ch &= 127;

	if ( ch == 32 ) {
		// space
		return;
	}

	if ( y < -CONCHAR_HEIGHT ) {
		// totally off screen
		return;
	}

	int row = ch >> 4;
	int col = ch & 15;

	constexpr float size = 0.0625f;

	float frow = row * size;
	float fcol = col * size;

	guiRect_t rect;

	// draw nice drop shadows

	rect.v1.Position( x + 1, y + 1 );
	rect.v1.TexCoord( fcol, frow );
	rect.v1.Color( colors::dkGray );

	rect.v2.Position( x + CONCHAR_WIDTH + 1, y + 1 );
	rect.v2.TexCoord( fcol + size, frow );
	rect.v2.Color( colors::dkGray );

	rect.v3.Position( x + CONCHAR_WIDTH + 1, y + CONCHAR_HEIGHT + 1 );
	rect.v3.TexCoord( fcol + size, frow + size );
	rect.v3.Color( colors::dkGray );

	rect.v4.Position( x + 1, y + CONCHAR_HEIGHT + 1 );
	rect.v4.TexCoord( fcol, frow + size );
	rect.v4.Color( colors::dkGray );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( s_drawChars );

	// draw the actual character

	rect.v1.Position( x, y );
	rect.v1.TexCoord( fcol, frow );
	rect.v1.Color( color );

	rect.v2.Position( x + CONCHAR_WIDTH, y );
	rect.v2.TexCoord( fcol + size, frow );
	rect.v2.Color( color );

	rect.v3.Position( x + CONCHAR_WIDTH, y + CONCHAR_HEIGHT );
	rect.v3.TexCoord( fcol + size, frow + size );
	rect.v3.Color( color );

	rect.v4.Position( x, y + CONCHAR_HEIGHT );
	rect.v4.TexCoord( fcol, frow + size );
	rect.v4.Color( color );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( s_drawChars );
}

/*
========================
R_DrawChar

Draws one graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
========================
*/
void R_DrawChar( int x, int y, int ch )
{
	R_DrawCharColor( x, y, ch, colors::defaultText );
}

/*
========================
R_DrawMaterial
========================
*/
static void R_DrawMaterial( int x, int y, int w, int h, material_t *mat )
{
	guiRect_t rect;

	uint32 color = PackColor( 255, 255, 255, mat->alpha );

	rect.v1.Position( x, y );
	rect.v1.TexCoord( 0.0f, 0.0f );
	rect.v1.Color( color );

	rect.v2.Position( x + w, y );
	rect.v2.TexCoord( 1.0f, 0.0f );
	rect.v2.Color( color );

	rect.v3.Position( x + w, y + h );
	rect.v3.TexCoord( 1.0f, 1.0f );
	rect.v3.Color( color );

	rect.v4.Position( x, y + h );
	rect.v4.TexCoord( 0.0f, 1.0f );
	rect.v4.Color( color );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( mat );
}

/*
========================
R_DrawStretchPic
========================
*/
void R_DrawStretchPic( int x, int y, int w, int h, const char *pic )
{
	material_t *mat;

	mat = R_RegisterPic( pic );
	if ( !mat ) {
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	R_DrawMaterial( x, y, w, h, mat );
}

/*
========================
R_DrawPic
========================
*/
void R_DrawPic( int x, int y, const char *pic )
{
	material_t *mat;

	mat = R_RegisterPic( pic );
	if ( !mat ) {
		Com_Printf( "Can't find pic: %s\n", pic );
		return;
	}

	R_DrawMaterial( x, y, mat->image->width, mat->image->height, mat );
}

/*
========================
R_DrawFilled

Fills a box of pixels with a single color
========================
*/
void R_DrawFilled( int x, int y, int w, int h, uint32 color )
{
	guiRect_t rect;

	rect.v1.Position( x, y );
	rect.v1.TexCoord( 0.0f, 0.0f );
	rect.v1.Color( color );

	rect.v2.Position( x + w, y );
	rect.v2.TexCoord( 1.0f, 0.0f );
	rect.v2.Color( color );

	rect.v3.Position( x + w, y + h );
	rect.v3.TexCoord( 1.0f, 1.0f );
	rect.v3.Color( color );

	rect.v4.Position( x, y + h );
	rect.v4.TexCoord( 0.0f, 1.0f );
	rect.v4.Color( color );

	s_drawMeshBuilder.AddElement( rect );

	Draw_CheckChain( whiteMaterial );
}

/*
========================
R_DrawScreenOverlay

Fills the whole screen with floating point color
========================
*/
void R_DrawScreenOverlay( const vec4_t color )
{
	if ( !r_polyblend->GetBool() || color[3] <= 0.0f ) {
		return;
	}

	uint32 newColor = PackColorFromFloats( color[0], color[1], color[2], color[3] );

	R_DrawFilled( 0, 0, tr.refdef.width, tr.refdef.height, newColor );
}

/*
========================
Draw_RenderBatches

Renders all our collected draw calls
========================
*/
void Draw_RenderBatches()
{
	using namespace DirectX;

	// render all lines

	if ( !s_polyVertices.empty() || !s_lineVertices.empty() )
	{
		GL_UseProgram( glProgs.lineProg );

		glUniformMatrix4fv( 2, 1, GL_FALSE, (const GLfloat *)&tr.viewMatrix );
		glUniformMatrix4fv( 3, 1, GL_FALSE, (const GLfloat *)&tr.projMatrix );

		glBindVertexArray( s_lineVAO );
		glBindBuffer( GL_ARRAY_BUFFER, s_lineVBO );
	}

	if ( !s_polyVertices.empty() )
	{
		glBufferData( GL_ARRAY_BUFFER, s_polyVertices.size() * sizeof( simpleVertex_t ), s_polyVertices.data(), GL_STREAM_DRAW );

		glDrawArrays( GL_TRIANGLES, 0, s_polyVertices.size() );

		s_polyVertices.clear();
	}

	if ( !s_lineVertices.empty() )
	{
		glBufferData( GL_ARRAY_BUFFER, s_lineVertices.size() * sizeof( simpleVertex_t ), s_lineVertices.data(), GL_STREAM_DRAW );

		glDrawArrays( GL_LINES, 0, s_lineVertices.size() );

		s_lineVertices.clear();
	}

	// render all gui draw calls

	if ( s_drawMeshBuilder.HasData() )
	{
		// profiling code
	//	double begin = Time_FloatMilliseconds();

		// set up the render state
		DirectX::XMFLOAT4X4A orthoMatrix;
		DirectX::XMStoreFloat4x4A( &orthoMatrix, XMMatrixOrthographicOffCenterRH( 0.0f, tr.refdef.width, tr.refdef.height, 0.0f, -1.0f, 1.0f ) );

		GL_UseProgram( glProgs.guiProg );
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

		GL_ActiveTexture( GL_TEXTURE0 );

		for ( const auto &cmd : s_drawCmds )
		{
			cmd.material->Bind();

			glDrawElements( GL_TRIANGLES, cmd.count, GL_UNSIGNED_SHORT, (void *)( (intptr_t)cmd.offset * sizeof( uint16 ) ) );
		}

		glDisable( GL_PRIMITIVE_RESTART_FIXED_INDEX );

		GL_UseProgram( 0 );

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

//static unsigned s_rawPalette[256];

/*
========================
R_SetRawPalette

Sets the palette used by R_DrawStretchRaw
========================
*/
void R_SetRawPalette( const byte *palette )
{
#if 0
	// we don't use the raw palette for anything but cinematics, so ignore NULL calls
	if ( !palette ) {
		return;
	}

	memcpy( s_rawPalette, palette, sizeof( s_rawPalette ) );
#endif
}

/*
========================
R_DrawStretchRaw

Used for rendering cinematic frames
========================
*/
void R_DrawStretchRaw( int x, int y, int w, int h, int cols, int rows, byte *data )
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
	GL_BindTexture( id );

	// Upload frame
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32 );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glBegin( GL_QUADS );
	glTexCoord( 0.0f, 0.0f );
	glVertex2i( x, y );
	glTexCoord( 1.0f, 0.0f );
	glVertex2i( x + w, y );
	glTexCoord( 1.0f, t );
	glVertex2i( x + w, y + h );
	glTexCoord( 0.0f, t );
	glVertex2i( x, y + h );
	glEnd();

	GL_BindTexture( 0 );

	// Delete frame
	glDeleteTextures( 1, &id );
#endif
}

/*
===================================================================================================

	3D draw functions

===================================================================================================
*/

void R_DrawBounds( const vec3_t mins, const vec3_t maxs )
{
#if 0
	GL_UseProgram( 0 );

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glBegin( GL_LINE_LOOP );
	glVertex3f( mins[0], mins[1], mins[2] );
	glVertex3f( mins[0], maxs[1], mins[2] );
	glVertex3f( maxs[0], maxs[1], mins[2] );
	glVertex3f( maxs[0], mins[1], mins[2] );
	glEnd();
	glBegin( GL_LINE_LOOP );
	glVertex3f( mins[0], mins[1], maxs[2] );
	glVertex3f( mins[0], maxs[1], maxs[2] );
	glVertex3f( maxs[0], maxs[1], maxs[2] );
	glVertex3f( maxs[0], mins[1], maxs[2] );
	glEnd();

	glBegin( GL_LINES );
	glVertex3f( mins[0], mins[1], mins[2] );
	glVertex3f( mins[0], mins[1], maxs[2] );

	glVertex3f( mins[0], maxs[1], mins[2] );
	glVertex3f( mins[0], maxs[1], maxs[2] );

	glVertex3f( maxs[0], mins[1], mins[2] );
	glVertex3f( maxs[0], mins[1], maxs[2] );

	glVertex3f( maxs[0], maxs[1], mins[2] );
	glVertex3f( maxs[0], maxs[1], maxs[2] );
	glEnd();
#endif
}

void R_DrawLine( const vec3_t start, const vec3_t end, uint32 color )
{
	simpleVertex_t &startVertex = s_lineVertices.emplace_back();
	VectorCopy( start, startVertex.pos );
	startVertex.rgba = color;

	simpleVertex_t &endVertex = s_lineVertices.emplace_back();
	VectorCopy( end, endVertex.pos );
	endVertex.rgba = color;
}

void R_DrawPoly( const vec3_t p1, const vec3_t p2, const vec3_t p3, uint32 color )
{
	simpleVertex_t vertex;

	VectorCopy( p1, vertex.pos );
	vertex.rgba = color;

	s_polyVertices.push_back( vertex );

	VectorCopy( p2, vertex.pos );
	vertex.rgba = color;

	s_polyVertices.push_back( vertex );

	VectorCopy( p3, vertex.pos );
	vertex.rgba = color;

	s_polyVertices.push_back( vertex );
}
