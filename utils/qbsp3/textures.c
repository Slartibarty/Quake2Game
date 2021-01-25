#include "qbsp.h"

#define	MAX_MAP_TEXTURES	1024

int				nummiptex;
textureref_t	textureref[MAX_MAP_TEXTURES];

//=============================================================================

#define	MAX_TOKEN_CHARS		128		// max length of an individual token

void Com_Parse2( char **data_p, char **token_p, int tokenlen )
{
	int		c;
	int		len;
	char	*data;
	char	*token;

	data = *data_p;
	token = *token_p;
	len = 0;

	if ( !data )
	{
		*data_p = NULL;
		token[0] = '\0';
		return;
	}

// skip whitespace
skipwhite:
	while ( ( c = *data ) <= ' ' )
	{
		if ( c == 0 )
		{
			*data_p = NULL;
			token[0] = '\0';
			return;
		}
		data++;
	}

// skip // comments
	if ( c == '/' && data[1] == '/' )
	{
		while ( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

// handle quoted strings specially
	if ( c == '\"' )
	{
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\"' || !c )
			{
				token[len] = 0;
				*data_p = data;
				return;
			}
			if ( len < tokenlen )
			{
				token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if ( len < tokenlen )
		{
			token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > 32 );

	if ( len == tokenlen )
	{
	//	Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	token[len] = 0;

	*data_p = data;
}

#define FLAGCHECK(flag) if ( strcmp( data, #flag ) == 0 ) { return flag; }

int StringToContentFlag( const char *data )
{
	FLAGCHECK( CONTENTS_SOLID );
	FLAGCHECK( CONTENTS_WINDOW );
	FLAGCHECK( CONTENTS_AUX );
	FLAGCHECK( CONTENTS_LAVA );
	FLAGCHECK( CONTENTS_SLIME );
	FLAGCHECK( CONTENTS_WATER );
	FLAGCHECK( CONTENTS_MIST );
	// last visible
	FLAGCHECK( CONTENTS_AREAPORTAL );
	FLAGCHECK( CONTENTS_PLAYERCLIP );
	FLAGCHECK( CONTENTS_MONSTERCLIP );
	FLAGCHECK( CONTENTS_CURRENT_0 );
	FLAGCHECK( CONTENTS_CURRENT_90 );
	FLAGCHECK( CONTENTS_CURRENT_180 );
	FLAGCHECK( CONTENTS_CURRENT_270 );
	FLAGCHECK( CONTENTS_CURRENT_UP );
	FLAGCHECK( CONTENTS_CURRENT_DOWN );
	FLAGCHECK( CONTENTS_ORIGIN );
	FLAGCHECK( CONTENTS_MONSTER );
	FLAGCHECK( CONTENTS_DEADMONSTER );
	FLAGCHECK( CONTENTS_DETAIL );
	FLAGCHECK( CONTENTS_TRANSLUCENT );
	FLAGCHECK( CONTENTS_LADDER );

	return 0;
}

int StringToSurfaceFlag( const char *data )
{
	FLAGCHECK( SURF_LIGHT );
	FLAGCHECK( SURF_SLICK );
	FLAGCHECK( SURF_SKY );
	FLAGCHECK( SURF_WARP );
	FLAGCHECK( SURF_TRANS33 );
	FLAGCHECK( SURF_TRANS66 );
	FLAGCHECK( SURF_FLOWING );
	FLAGCHECK( SURF_NODRAW );
	FLAGCHECK( SURF_HINT );
	FLAGCHECK( SURF_SKIP );

	return 0;
}

#undef FLAGCHECK

int ParseSurfaceFlags( char *data, int type )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;
	int flags = 0;

	Com_Parse2( &data, &token, sizeof( tokenhack ) );

	// Parse until the end baby
	while ( data )
	{
		if ( type == 1 )
			flags |= StringToContentFlag( token );
		else
			flags |= StringToSurfaceFlag( token );

		Com_Parse2( &data, &token, sizeof( tokenhack ) );
	}

	return flags;
}

void ParseMaterial( char *data, textureref_t *ref )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;

	Com_Parse2( &data, &token, sizeof( tokenhack ) );
	if ( token[0] != '{' )
	{
		// Malformed
		Error( "Malformed material %s\n", ref->name );
	}

	Com_Parse2( &data, &token, sizeof( tokenhack ) );

	for ( ; token[0] != '}'; Com_Parse2( &data, &token, sizeof( tokenhack ) ) )
	{
		if ( strcmp( token, "$basetexture" ) == 0 )
		{
			Com_Parse2( &data, &token, sizeof( tokenhack ) );
			strcpy( ref->name, token );
			continue;
		}
		if ( strcmp( token, "$contentflags" ) == 0 )
		{
			Com_Parse2( &data, &token, sizeof( tokenhack ) );
			ref->contents = ParseSurfaceFlags( token, 1 );
			continue;
		}
		if ( strcmp( token, "$surfaceflags" ) == 0 )
		{
			Com_Parse2( &data, &token, sizeof( tokenhack ) );
			ref->flags = ParseSurfaceFlags( token, 0 );
			continue;
		}
		if ( strcmp( token, "$value" ) == 0 )
		{
			Com_Parse2( &data, &token, sizeof( tokenhack ) );
			ref->value = atoi( token );
			continue;
		}
	}

}

textureref_t *FindMiptex( const char *name )
{
	int			i;
	char		path[_MAX_PATH];
	miptex_t	*mt;

	if ( nummiptex == MAX_MAP_TEXTURES )
		Error( "MAX_MAP_TEXTURES" );

	for ( i = 0; i < nummiptex; ++i )
	{
		if ( strcmp( name, textureref[i].name ) == 0 )
			return &textureref[i];
	}

	strcpy( textureref[i].name, name );

	// load the miptex to get the flags and values
	sprintf( path, "%stextures/%s.wal", gamedir, name );
	if ( TryLoadFile( path, (void **)&mt ) != -1 )
	{
		textureref[i].value = LittleLong( mt->value );
		textureref[i].flags = LittleLong( mt->flags );
		textureref[i].contents = LittleLong( mt->contents );
		strcpy( textureref[i].animname, mt->animname );
		free( mt );
	}
	else // No WAL
	{
		char *data;

		sprintf( path, "%smaterials/%s.mat", gamedir, name );
		if ( TryLoadFile( path, (void **)&data ) != -1 )
		{
			strcpy( textureref[i].name, name );
			ParseMaterial( data, &textureref[i] );
		}
	}

	++nummiptex;

	if ( textureref[i].animname[0] )
		FindMiptex( textureref[i].animname ); // Recurse

	return &textureref[i];
}


/*
==================
textureAxisFromPlane
==================
*/
vec3_t	baseaxis[18] =
{
{0,0,1}, {1,0,0}, {0,-1,0},			// floor
{0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
{1,0,0}, {0,1,0}, {0,0,-1},			// west wall
{-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
{0,1,0}, {1,0,0}, {0,0,-1},			// south wall
{0,-1,0}, {1,0,0}, {0,0,-1}			// north wall
};

void TextureAxisFromPlane(plane_t *pln, vec3_t xv, vec3_t yv)
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




int TexinfoForBrushTexture (plane_t *plane, brush_texture_t *bt, vec3_t origin)
{
	vec3_t	vecs[2];
	int		sv, tv;
	vec_t	ang, sinv, cosv;
	vec_t	ns, nt;
	texinfo_t	tx, *tc;
	int		i, j, k;
	float	shift[2];
	brush_texture_t		anim;
	textureref_t		*mt;

	if (!bt->name[0])
		return 0;

	memset (&tx, 0, sizeof(tx));
	strcpy (tx.texture, bt->name);

	TextureAxisFromPlane(plane, vecs[0], vecs[1]);

	shift[0] = DotProduct (origin, vecs[0]);
	shift[1] = DotProduct (origin, vecs[1]);

	if (!bt->scale[0])
		bt->scale[0] = 1;
	if (!bt->scale[1])
		bt->scale[1] = 1;


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
		ang = bt->rotate / 180 * Q_PI;
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

	tx.vecs[0][3] = bt->shift[0] + shift[0];
	tx.vecs[1][3] = bt->shift[1] + shift[1];
	tx.flags = bt->flags;
	tx.value = bt->value;

	//
	// find the texinfo
	//
	tc = texinfo;
	for (i=0 ; i<numtexinfo ; i++, tc++)
	{
		if (tc->flags != tx.flags)
			continue;
		if (tc->value != tx.value)
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
