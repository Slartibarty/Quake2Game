//=============================================================================
// Textures!
//=============================================================================

#include "qbsp.h"

#define	MAX_MAP_TEXTURES	1024

int				nummiptex;
textureref_t	textureref[MAX_MAP_TEXTURES];

//=============================================================================

#if 0

#define FLAGCHECK(flag) if ( Q_strcmp( data, #flag ) == 0 ) return flag;
#define FLAGCHECK2(flag, name) if ( Q_strcmp( data, name ) == 0 ) return flag;

/*
===================
StringToContentFlag

Parse a content flag from a string
===================
*/
static int StringToContentFlag( const char *data )
{
	FLAGCHECK( CONTENTS_SOLID )
	FLAGCHECK( CONTENTS_WINDOW )
	FLAGCHECK( CONTENTS_AUX )
	FLAGCHECK( CONTENTS_LAVA )
	FLAGCHECK( CONTENTS_SLIME )
	FLAGCHECK( CONTENTS_WATER )
	FLAGCHECK( CONTENTS_MIST )
	// last visible
	FLAGCHECK( CONTENTS_AREAPORTAL )
	FLAGCHECK( CONTENTS_PLAYERCLIP )
	FLAGCHECK( CONTENTS_MONSTERCLIP )
	FLAGCHECK( CONTENTS_CURRENT_0 )
	FLAGCHECK( CONTENTS_CURRENT_90 )
	FLAGCHECK( CONTENTS_CURRENT_180 )
	FLAGCHECK( CONTENTS_CURRENT_270 )
	FLAGCHECK( CONTENTS_CURRENT_UP )
	FLAGCHECK( CONTENTS_CURRENT_DOWN )
	FLAGCHECK( CONTENTS_ORIGIN )
	FLAGCHECK( CONTENTS_MONSTER )
	FLAGCHECK( CONTENTS_DEADMONSTER )
	FLAGCHECK( CONTENTS_DETAIL )
	FLAGCHECK( CONTENTS_TRANSLUCENT )
	FLAGCHECK( CONTENTS_LADDER )

	return 0;
}

/*
===================
StringToSurfaceType

Parse a surface type from a string
===================
*/
static int StringToSurfaceType( const char *data )
{
	FLAGCHECK2( SURFTYPE_CONCRETE, "concrete" )
	FLAGCHECK2( SURFTYPE_METAL, "metal" )
	FLAGCHECK2( SURFTYPE_VENT, "vent" )
	FLAGCHECK2( SURFTYPE_DIRT, "dirt" )
	FLAGCHECK2( SURFTYPE_GRASS, "grass" )
	FLAGCHECK2( SURFTYPE_SAND, "sand" )
	FLAGCHECK2( SURFTYPE_ROCK, "rock" )
	FLAGCHECK2( SURFTYPE_WOOD, "wood" )
	FLAGCHECK2( SURFTYPE_TILE, "tile" )
	FLAGCHECK2( SURFTYPE_COMPUTER, "computer" )
	FLAGCHECK2( SURFTYPE_GLASS, "glass" )
	FLAGCHECK2( SURFTYPE_FLESH, "flesh" )

	return 0;
}

/*
===================
StringToSurfaceFlag

Parse a surface flag from a string
===================
*/
static int StringToSurfaceFlag( const char *data )
{
	FLAGCHECK( SURF_LIGHT )
	FLAGCHECK( SURF_SLICK )
	FLAGCHECK( SURF_SKY )
	FLAGCHECK( SURF_WARP )
	FLAGCHECK( SURF_TRANS33 )
	FLAGCHECK( SURF_TRANS66 )
	FLAGCHECK( SURF_FLOWING )
	FLAGCHECK( SURF_NODRAW )
	FLAGCHECK( SURF_HINT )
	FLAGCHECK( SURF_SKIP )

	return 0;
}

#undef FLAGCHECK

/*
===================
ParseSurfaceFlags

1 == contents
2 == surface types
3 == surface flags
===================
*/
static int ParseSurfaceFlags( char *curToken, char *nextToken, int type )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;
	int flags = 0;

	token = curToken;

	// Parse until the end baby
	while ( nextToken )
	{
		if ( type == 1 )
			flags |= StringToContentFlag( token );
		else if ( type == 2 )
			flags |= StringToSurfaceType( token );
		else
			flags |= StringToSurfaceFlag( token );

		token = tokenhack;

		COM_Parse2( &nextToken, &token, sizeof( tokenhack ) );
	}

	return flags;
}

#endif

#define FLAGCHECK2(flag, name) if ( Q_strcmp( data, name ) == 0 ) return flag;

/*
===================
StringToSurfaceType

Parse a surface type from a string
===================
*/
static int StringToSurfaceType( const char *data )
{
	FLAGCHECK2( SURFTYPE_CONCRETE, "concrete" )
	FLAGCHECK2( SURFTYPE_METAL, "metal" )
	FLAGCHECK2( SURFTYPE_VENT, "vent" )
	FLAGCHECK2( SURFTYPE_DIRT, "dirt" )
	FLAGCHECK2( SURFTYPE_GRASS, "grass" )
	FLAGCHECK2( SURFTYPE_SAND, "sand" )
	FLAGCHECK2( SURFTYPE_ROCK, "rock" )
	FLAGCHECK2( SURFTYPE_WOOD, "wood" )
	FLAGCHECK2( SURFTYPE_TILE, "tile" )
	FLAGCHECK2( SURFTYPE_COMPUTER, "computer" )
	FLAGCHECK2( SURFTYPE_GLASS, "glass" )
	FLAGCHECK2( SURFTYPE_FLESH, "flesh" )

	return 0;
}

#undef FLAGCHECK2

/*
===================
ParseMaterial
===================
*/
static void ParseMaterial( char *data, textureref_t *ref )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;

	COM_Parse2( &data, &token, sizeof( tokenhack ) );
	if ( token[0] != '{' )
	{
		// Malformed
		Com_FatalErrorf( "Malformed material %s\n", ref->name );
	}

	COM_Parse2( &data, &token, sizeof( tokenhack ) );

	for ( ; token[0] != '}'; COM_Parse2( &data, &token, sizeof( tokenhack ) ) )
	{
		if ( Q_strcmp( token, "$nextframe" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			Q_strcpy_s( ref->animname, token );
			continue;
		}
		if ( Q_strcmp( token, "$surfacetype" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			ref->flags |= StringToSurfaceType( token );
			continue;
		}
		if ( Q_strcmp( token, "%compilesky" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( atoi( token ) ) {
				ref->flags |= SURF_NODRAW | SURF_SKY;
			}
			continue;
		}
		if ( Q_strcmp( token, "%compilenodraw" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			if ( atoi( token ) ) {
				ref->flags |= SURF_NODRAW;
			}
			continue;
		}
	}

}

/*
===================
FindMiptex
===================
*/
textureref_t *FindMiptex( const char *name )
{
	int			i;
	char		path[MAX_OSPATH];
	char		*data;
	textureref_t	*ref;

	if ( nummiptex == MAX_MAP_TEXTURES )
		Error( "MAX_MAP_TEXTURES" );

	for ( i = 0; i < nummiptex; ++i )
	{
		if ( Q_strcmp( name, textureref[i].name ) == 0 )
			return &textureref[i];
	}

	ref = &textureref[i];

	// Structure is already zeroed

	Q_strcpy_s( ref->name, name );

	Q_sprintf_s( path, "%smaterials/%s.mat", gamedir, name );
	if ( TryLoadFile( path, (void **)&data ) != -1 )
	{
		Q_strcpy_s( ref->name, name );
		ParseMaterial( data, ref );
	}

	++nummiptex;

	if ( ref->animname[0] )
		FindMiptex( ref->animname ); // Recurse

	return ref;
}

//=============================================================================

/*
===================
TextureAxisFromPlane
===================
*/
static const vec3_t baseaxis[18]
{
	{0,0,1}, {1,0,0}, {0,-1,0},			// floor
	{0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
	{1,0,0}, {0,1,0}, {0,0,-1},			// west wall
	{-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
	{0,1,0}, {1,0,0}, {0,0,-1},			// south wall
	{0,-1,0}, {1,0,0}, {0,0,-1}			// north wall
};

static void TextureAxisFromPlane(plane_t *pln, vec3_t xv, vec3_t yv)
{
	int		bestaxis;
	vec_t	dot,best;
	int		i;
	
	best = 0;
	bestaxis = 0;
	
	for (i=0 ; i<6 ; i++)
	{
		dot = DotProduct (pln->normal, baseaxis[i*3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
	VectorCopy (baseaxis[bestaxis*3+1], xv);
	VectorCopy (baseaxis[bestaxis*3+2], yv);
}

//=============================================================================

/*
===================
TexinfoForBrushTexture
===================
*/
int TexinfoForBrushTexture (plane_t *plane, brush_texture_t *bt, vec3_t origin)
{
	vec3_t	vecs[2];
	int		sv, tv;
	vec_t	ang, sinv, cosv;
	vec_t	ns, nt;
	texinfo_t	tx, *tc;
	int		i, j, k;
	brush_texture_t		anim;
	textureref_t		*mt;

	if (!bt->name[0])
		return 0;

	memset (&tx, 0, sizeof(tx));
	strcpy (tx.texture, bt->name);

	if (g_nMapFileVersion < 220)
	{
		TextureAxisFromPlane(plane, vecs[0], vecs[1]);
	}

	if (!bt->scale[0])
		bt->scale[0] = 1;
	if (!bt->scale[1])
		bt->scale[1] = 1;


	if ( g_nMapFileVersion < 220 )
	{
	// rotate axis
		if (bt->rotate == 0)
			{ sinv = 0 ; cosv = 1; }
		else if (bt->rotate == 90)
			{ sinv = 1 ; cosv = 0; }
		else if (bt->rotate == 180)
			{ sinv = 0 ; cosv = -1; }
		else if (bt->rotate == 270)
			{ sinv = -1 ; cosv = 0; }
		else
		{	
			ang = RAD2DEG(bt->rotate);
			sinv = sin(ang);
			cosv = cos(ang);
		}

		if (vecs[0][0])
			sv = 0;
		else if (vecs[0][1])
			sv = 1;
		else
			sv = 2;
				
		if (vecs[1][0])
			tv = 0;
		else if (vecs[1][1])
			tv = 1;
		else
			tv = 2;
					
		for (i=0 ; i<2 ; i++)
		{
			ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
			nt = sinv * vecs[i][sv] +  cosv * vecs[i][tv];
			vecs[i][sv] = ns;
			vecs[i][tv] = nt;
		}

		for (i=0 ; i<2 ; i++)
			for (j=0 ; j<3 ; j++)
				tx.vecs[i][j] = vecs[i][j] / bt->scale[i];

		tx.vecs[0][3] = bt->shift[0] + DotProduct (origin, vecs[0]);
		tx.vecs[1][3] = bt->shift[1] + DotProduct (origin, vecs[1]);
	}
	else
	{
		tx.vecs[0][0] = bt->UAxis[0] / bt->scale[0];
		tx.vecs[0][1] = bt->UAxis[1] / bt->scale[0];
		tx.vecs[0][2] = bt->UAxis[2] / bt->scale[0];

		tx.vecs[1][0] = bt->VAxis[0] / bt->scale[1];
		tx.vecs[1][1] = bt->VAxis[1] / bt->scale[1];
		tx.vecs[1][2] = bt->VAxis[2] / bt->scale[1];

		tx.vecs[0][3] = bt->shift[0];
		tx.vecs[1][3] = bt->shift[1];
	}

	tx.flags = bt->flags;

	//
	// find the texinfo
	//
	tc = texinfo;
	for (i=0 ; i<numtexinfo ; i++, tc++)
	{
		if (tc->flags != tx.flags)
			continue;
		for (j=0 ; j<2 ; j++)
		{
			if (strcmp (tc->texture, tx.texture))
				goto skip;
			for (k=0 ; k<4 ; k++)
			{
				if (tc->vecs[j][k] != tx.vecs[j][k])
					goto skip;
			}
		}
		return i;
skip:;
	}
	*tc = tx;
	numtexinfo++;

	// load the next animation
	mt = FindMiptex (bt->name);
	// SLARTTODO: WTF?? This is a problem
	bt->flags = tc->flags = mt->flags; // This didn't used to be here!!!
	if (mt->animname[0])
	{
		anim = *bt;
		strcpy (anim.name, mt->animname);
		tc->nexttexinfo = TexinfoForBrushTexture (plane, &anim, origin);
	}
	else
		tc->nexttexinfo = -1;


	return i;
}
