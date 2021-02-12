
#include "qrad.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ONLY_TGA
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#include "stb_image.h"

// A binary material
struct materialref_t
{
	texinfo_t	*ltexinfo;
	vec3_t		reflectivity;
	vec3_t		color;
	int			intensity;
};

materialref_t *g_materialrefs;

/*
===================================================================

  TEXTURE LIGHT VALUES

===================================================================
*/

/*
===================
ParseColorVector
===================
*/
void ParseColorVector( char *data, vec3_t color )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;
	int i;

	COM_Parse2( &data, &token, sizeof( tokenhack ) );

	// Parse until the end baby
	for ( i = 0; i < 3 && data; ++i )
	{
		color[i] = atoi( token );

		COM_Parse2( &data, &token, sizeof( tokenhack ) );
	}
}

/*
===================
ParseMaterial
===================
*/
void ParseMaterial( char *data, materialref_t &matref, char *basetexture, strlen_t len, bool &hascolor )
{
	char tokenhack[MAX_TOKEN_CHARS];
	char *token = tokenhack;

	COM_Parse2( &data, &token, sizeof( tokenhack ) );
	if ( token[0] != '{' )
	{
		// Malformed
		Error( "Malformed material %s\n", matref.ltexinfo->texture );
	}

	COM_Parse2( &data, &token, sizeof( tokenhack ) );

	for ( ; token[0] != '}'; COM_Parse2( &data, &token, sizeof( tokenhack ) ) )
	{
		if ( Q_strcmp( token, "$basetexture" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			Q_sprintf_s( basetexture, len, "%s%s", gamedir, token );
			continue;
		}
		if ( Q_strcmp( token, "%rad_color" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			ParseColorVector( token, matref.color );
			hascolor = true;
			continue;
		}
		if ( Q_strcmp( token, "%rad_intensity" ) == 0 )
		{
			COM_Parse2( &data, &token, sizeof( tokenhack ) );
			matref.intensity = atoi( token );
			continue;
		}
	}
}

/*
======================
CalcTextureReflectivity

Loads all materials and fills out g_materialrefs
======================
*/
void CalcTextureReflectivity()
{
	int i, j;
	char basetexture[MAX_OSPATH];
	char materialpath[MAX_OSPATH];
	char *matdata;
	byte *imagedata;
	int width, height;
	bool hascolor = false;	// True if the material has %rad_color set

	int texels;
	int color[3];

	// NEVER FREED!!!
	g_materialrefs = (materialref_t *)malloc( sizeof( materialref_t ) * numtexinfo );

	for ( i = 0; i < numtexinfo; ++i )
	{
		materialref_t &matref = g_materialrefs[i];

		matref.ltexinfo = &texinfo[i];
		// Set up defaults
		VectorSetAll( matref.reflectivity, 0.5f );
		VectorSetAll( matref.color, 1.0f );
		matref.intensity = 0;
		// Intensity of 0 means no light

		// Load the material
		Q_sprintf_s( materialpath, "%smaterials/%s.mat", gamedir, matref.ltexinfo->texture );
		if ( !TryLoadFile( materialpath, (void **)&matdata ) )
		{
			printf( "Couldn't load material %s\n", matref.ltexinfo->texture );
			continue;
		}

		ParseMaterial( matdata, matref, basetexture, sizeof( basetexture ), hascolor );
		free( matdata );

		// No basetexture entry
		if ( !basetexture[0] )
		{
			printf( "%s has no basetexture!\n", matref.ltexinfo->texture );
			continue;
		}

		imagedata = stbi_load( basetexture, &width, &height, NULL, 3 );
		if ( !imagedata )
		{
			printf( "Couldn't load basetexture for %s\n", matref.ltexinfo->texture );
			continue;
		}

		texels = width * height;
		color[0] = color[1] = color[2] = 0;

		for ( j = 0; j < texels; ++j )
		{
			color[0] += imagedata[j + 0];
			color[1] += imagedata[j + 1];
			color[2] += imagedata[j + 2];
		}

		free( imagedata );

		// Convert to 0.0 - 1.0
		matref.reflectivity[0] = (float)( color[0] / texels ) / 255.0f;
		matref.reflectivity[1] = (float)( color[1] / texels ) / 255.0f;
		matref.reflectivity[2] = (float)( color[2] / texels ) / 255.0f;

		// No color and we're lighting the world? Make the colour the reflectivity
		if ( !hascolor && matref.intensity != 0 )
		{
			VectorCopy( matref.reflectivity, matref.color );
		}

#if 0
		// scale the reflectivity up, because the textures are
		// so dim
		scale = ColorNormalize( texture_reflectivity[i], texture_reflectivity[i] );
		if ( scale < 0.5f )
		{
			scale *= 2.0f;
			VectorScale( texture_reflectivity[i], scale, texture_reflectivity[i] );
		}
#endif

		Com_DPrintf( "matref %d (%s) avg rgb [ %f, %f, %f ]\n",
			i, matref.ltexinfo->texture, matref.reflectivity[0],
			matref.reflectivity[1], matref.reflectivity[2] );
	}
}

/*
=======================================================================

MAKE FACES

=======================================================================
*/

/*
=============
WindingFromFace
=============
*/
winding_t	*WindingFromFace (dface_t *f)
{
	int			i;
	int			se;
	dvertex_t	*dv;
	int			v;
	winding_t	*w;

	w = AllocWinding (f->numedges);
	w->numpoints = f->numedges;

	for (i=0 ; i<f->numedges ; i++)
	{
		se = dsurfedges[f->firstedge + i];
		if (se < 0)
			v = dedges[-se].v[1];
		else
			v = dedges[se].v[0];

		dv = &dvertexes[v];
		VectorCopy (dv->point, w->p[i]);
	}

	RemoveColinearPoints (w);

	return w;
}

/*
=============
BaseLightForFace
=============
*/
void BaseLightForFace( dface_t *f, vec3_t color )
{
	//
	// check for light emited by texture
	//
	materialref_t &matref = g_materialrefs[f->texinfo];
	if ( matref.intensity == 0 )
	{
		// No light
		VectorClear( color );
		return;
	}

	VectorScale( matref.color, matref.intensity, color );
}

qboolean IsSky (dface_t *f)
{
	texinfo_t	*tx;

	tx = &texinfo[f->texinfo];
	if (tx->flags & SURF_SKY)
		return true;
	return false;
}

/*
=============
MakePatchForFace
=============
*/
float	totalarea;
void MakePatchForFace (int fn, winding_t *w)
{
	dface_t *f;
	float	area;
	patch_t		*patch;
	dplane_t	*pl;
	int			i;
	vec3_t		color;
	dleaf_t		*leaf;

	f = &dfaces[fn];

	area = WindingArea (w);
	totalarea += area;

	patch = &patches[num_patches];
	if (num_patches == MAX_PATCHES)
		Error ("num_patches == MAX_PATCHES");
	patch->next = face_patches[fn];
	face_patches[fn] = patch;

	patch->winding = w;

	if (f->side)
		patch->plane = &backplanes[f->planenum];
	else
		patch->plane = &dplanes[f->planenum];
	if (face_offset[fn][0] || face_offset[fn][1] || face_offset[fn][2] )
	{	// origin offset faces must create new planes
		if (numplanes + fakeplanes >= MAX_MAP_PLANES)
			Error ("numplanes + fakeplanes >= MAX_MAP_PLANES");
		pl = &dplanes[numplanes + fakeplanes];
		fakeplanes++;

		*pl = *(patch->plane);
		pl->dist += DotProduct (face_offset[fn], pl->normal);
		patch->plane = pl;
	}

	WindingCenter (w, patch->origin);
	VectorAdd (patch->origin, patch->plane->normal, patch->origin);
	leaf = PointInLeaf(patch->origin);
	patch->cluster = leaf->cluster;
	if (patch->cluster == -1)
		qprintf ("patch->cluster == -1\n");

	patch->area = area;
	if (patch->area <= 1)
		patch->area = 1;
	patch->sky = IsSky (f);

	VectorCopy (g_materialrefs[f->texinfo].reflectivity, patch->reflectivity);

	// non-bmodel patches can emit light
	if (fn < dmodels[0].numfaces)
	{
		BaseLightForFace (f, patch->baselight);

		ColorNormalize (patch->reflectivity, color);

		for (i=0 ; i<3 ; i++)
			patch->baselight[i] *= color[i];

		VectorCopy (patch->baselight, patch->totallight);
	}
	num_patches++;
}


entity_t *EntityForModel (int modnum)
{
	int		i;
	char	*s;
	char	name[16];

	Q_sprintf (name, "*%d", modnum);
	// search the entities for one using modnum
	for (i=0 ; i<num_entities ; i++)
	{
		s = ValueForKey (&entities[i], "model");
		if (Q_strcmp (s, name) == 0)
			return &entities[i];
	}

	return &entities[0];
}

/*
=============
MakePatches
=============
*/
void MakePatches (void)
{
	int		i, j, k;
	dface_t	*f;
	int		fn;
	winding_t	*w;
	dmodel_t	*mod;
	vec3_t		origin;
	entity_t	*ent;

	qprintf ("%i faces\n", numfaces);

	for (i=0 ; i<nummodels ; i++)
	{
		mod = &dmodels[i];
		ent = EntityForModel (i);
		// bmodels with origin brushes need to be offset into their
		// in-use position
		GetVectorForKey (ent, "origin", origin);
//VectorCopy (vec3_origin, origin);

		for (j=0 ; j<mod->numfaces ; j++)
		{
			fn = mod->firstface + j;
			face_entity[fn] = ent;
			VectorCopy (origin, face_offset[fn]);
			f = &dfaces[fn];
			w = WindingFromFace (f);
			for (k=0 ; k<w->numpoints ; k++)
			{
				VectorAdd (w->p[k], origin, w->p[k]);
			}
			MakePatchForFace (fn, w);
		}
	}

	qprintf ("%i sqaure feet\n", (int)(totalarea/64));
}

/*
=======================================================================

SUBDIVIDE

=======================================================================
*/

void FinishSplit (patch_t *patch, patch_t *newp)
{
	dleaf_t		*leaf;

	VectorCopy (patch->baselight, newp->baselight);
	VectorCopy (patch->totallight, newp->totallight);
	VectorCopy (patch->reflectivity, newp->reflectivity);
	newp->plane = patch->plane;
	newp->sky = patch->sky;

	patch->area = WindingArea (patch->winding);
	newp->area = WindingArea (newp->winding);

	if (patch->area <= 1)
		patch->area = 1;
	if (newp->area <= 1)
		newp->area = 1;

	WindingCenter (patch->winding, patch->origin);
	VectorAdd (patch->origin, patch->plane->normal, patch->origin);
	leaf = PointInLeaf(patch->origin);
	patch->cluster = leaf->cluster;
	if (patch->cluster == -1)
		qprintf ("patch->cluster == -1\n");

	WindingCenter (newp->winding, newp->origin);
	VectorAdd (newp->origin, newp->plane->normal, newp->origin);
	leaf = PointInLeaf(newp->origin);
	newp->cluster = leaf->cluster;
	if (newp->cluster == -1)
		qprintf ("patch->cluster == -1\n");
}

/*
=============
SubdividePatch

Chops the patch only if its local bounds exceed the max size
=============
*/
void	SubdividePatch (patch_t *patch)
{
	winding_t *w, *o1, *o2;
	vec3_t	mins, maxs, total;
	vec3_t	split;
	vec_t	dist;
	int		i, j;
	vec_t	v;
	patch_t	*newp;

	w = patch->winding;
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
	for (i=0 ; i<w->numpoints ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			v = w->p[i][j];
			if (v < mins[j])
				mins[j] = v;
			if (v > maxs[j])
				maxs[j] = v;
		}
	}
	VectorSubtract (maxs, mins, total);
	for (i=0 ; i<3 ; i++)
		if (total[i] > (subdiv+1) )
			break;
	if (i == 3)
	{
		// no splitting needed
		return;		
	}

	//
	// split the winding
	//
	VectorCopy (vec3_origin, split);
	split[i] = 1;
	dist = (mins[i] + maxs[i])*0.5f;
	ClipWindingEpsilon (w, split, dist, ON_EPSILON, &o1, &o2);

	//
	// create a new patch
	//
	if (num_patches == MAX_PATCHES)
		Error ("MAX_PATCHES");
	newp = &patches[num_patches];
	num_patches++;

	newp->next = patch->next;
	patch->next = newp;

	patch->winding = o1;
	newp->winding = o2;

	FinishSplit (patch, newp);

	SubdividePatch (patch);
	SubdividePatch (newp);
}


/*
=============
DicePatch

Chops the patch by a global grid
=============
*/
void	DicePatch (patch_t *patch)
{
	winding_t *w, *o1, *o2;
	vec3_t	mins, maxs;
	vec3_t	split;
	vec_t	dist;
	int		i;
	patch_t	*newp;

	w = patch->winding;
	WindingBounds (w, mins, maxs);
	for (i=0 ; i<3 ; i++)
		if (floor((mins[i]+1)/subdiv) < floor((maxs[i]-1)/subdiv))
			break;
	if (i == 3)
	{
		// no splitting needed
		return;		
	}

	//
	// split the winding
	//
	VectorCopy (vec3_origin, split);
	split[i] = 1;
	dist = subdiv*(1+floor((mins[i]+1)/subdiv));
	ClipWindingEpsilon (w, split, dist, ON_EPSILON, &o1, &o2);

	//
	// create a new patch
	//
	if (num_patches == MAX_PATCHES)
		Error ("MAX_PATCHES");
	newp = &patches[num_patches];
	num_patches++;

	newp->next = patch->next;
	patch->next = newp;

	patch->winding = o1;
	newp->winding = o2;

	FinishSplit (patch, newp);

	DicePatch (patch);
	DicePatch (newp);
}


/*
=============
SubdividePatches
=============
*/
void SubdividePatches (void)
{
	int		i, num;

	if (subdiv < 1)
		return;

	num = num_patches;	// because the list will grow
	for (i=0 ; i<num ; i++)
	{
//		SubdividePatch (&patches[i]);
		DicePatch (&patches[i]);
	}
	qprintf ("%i patches after subdivision\n", num_patches);
}

//=====================================================================
