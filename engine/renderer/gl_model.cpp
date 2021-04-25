// models.c -- model loading and caching

#include "gl_local.h"

model_t			*loadmodel;
static int		modfilelen;

void Mod_LoadBrushModel( model_t *mod, void *buffer );
void Mod_LoadAliasModel( model_t *mod, void *buffer );
void Mod_LoadStudioModel( model_t *mod, void *buffer );
void Mod_LoadSMFModel( model_t *mod, void *buffer );
void Mod_LoadSpriteModel( model_t *mod, void *buffer );

static byte	mod_novis[MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	512
static model_t	mod_known[MAX_MOD_KNOWN];
static int		mod_numknown;

// the inline * models from the current map are kept seperate
static model_t	mod_inline[MAX_MOD_KNOWN];

/*
========================
Mod_PointInLeaf
========================
*/
mleaf_t *Mod_PointInLeaf( vec3_t p, model_t *model )
{
	mnode_t *node;
	float		d;
	cplane_t *plane;

	if ( !model || !model->nodes ) {
		Com_Error( "Mod_PointInLeaf: bad model" );
	}

	node = model->nodes;
	while ( 1 )
	{
		if ( node->contents != -1 ) {
			return (mleaf_t *)node;
		}
		plane = node->plane;
		d = DotProduct( p, plane->normal ) - plane->dist;
		if ( d > 0 ) {
			node = node->children[0];
		} else {
			node = node->children[1];
		}
	}

	return NULL;	// never reached
}

/*
========================
Mod_DecompressVis
========================
*/
static byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->vis->numclusters+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
========================
Mod_ClusterPVS
========================
*/
byte *Mod_ClusterPVS( int cluster, model_t *model )
{
	if ( cluster == -1 || !model->vis ) {
		return mod_novis;
	}
	return Mod_DecompressVis( (byte *)model->vis + model->vis->bitofs[cluster][DVIS_PVS], model );
}

//=================================================================================================

/*
========================
Mod_Modellist_f
========================
*/
void Mod_Modellist_f()
{
	int		i;
	model_t *mod;
	int		total;

	total = 0;
	Com_DPrintf( "Loaded models:\n" );
	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[0] ) {
			continue;
		}
		Com_Printf( "%8i : %s\n", mod->extradatasize, mod->name );
		total += mod->extradatasize;
	}
	Com_Printf( "Total resident: %i\n", total );
}

/*
========================
Mod_Init
========================
*/
void Mod_Init()
{
	memset( mod_novis, 0xff, sizeof( mod_novis ) );
}

/*
========================
Mod_ForName

Loads in a model for the given name
========================
*/
model_t *Mod_ForName( const char *name, qboolean crash )
{
	model_t *mod;
	unsigned *buf;
	int		i;

	if ( !name[0] ) {
		Com_Error( "Mod_ForName: NULL name" );
	}

	//
	// inline models are grabbed only from worldmodel
	//
	if ( name[0] == '*' )
	{
		i = atoi( name + 1 );
		if ( i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels ) {
			Com_Error( "bad inline model number" );
		}
		return &mod_inline[i];
	}

	//
	// search the currently loaded models
	//
	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[0] ) {
			continue;
		}
		if ( !Q_strcmp( mod->name, name ) ) {
			return mod;
		}
	}

	//
	// find a free model slot spot
	//
	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[0] ) {
			// free spot
			break;
		}
	}
	if ( i == mod_numknown )
	{
		if ( mod_numknown == MAX_MOD_KNOWN ) {
			Com_Error( "mod_numknown == MAX_MOD_KNOWN" );
		}
		mod_numknown++;
	}

	Q_strcpy_s( mod->name, name );

	//
	// load the file
	//
	modfilelen = FS_LoadFile( mod->name, (void **)&buf );
	if ( !buf )
	{
		if ( crash ) {
			Com_Errorf( "Mod_NumForName: %s not found", mod->name );
		}
		memset( mod->name, 0, sizeof( mod->name ) );
		return NULL;
	}

	loadmodel = mod;

	//
	// fill it in
	//

	// call the apropriate loader

	switch ( LittleLong( *(unsigned *)buf ) )
	{
	case fmtSMF::fourCC:
		loadmodel->extradata = Hunk_Begin( 0x1000000 );
		Mod_LoadSMFModel( mod, buf );
		break;

	case IDALIASHEADER:
		loadmodel->extradata = Hunk_Begin( 0x200000 );
		Mod_LoadAliasModel( mod, buf );
		break;

	case IDSTUDIOHEADER:
		loadmodel->extradata = Hunk_Begin( 0x400000 );
		Mod_LoadStudioModel( mod, buf );
		break;

	case IDSPRITEHEADER:
		loadmodel->extradata = Hunk_Begin( 0x10000 );
		Mod_LoadSpriteModel( mod, buf );
		break;

	case IDBSPHEADER:
		loadmodel->extradata = Hunk_Begin( 0x1000000 );
		Mod_LoadBrushModel( mod, buf );
		break;

	default:
		Com_Errorf( "Mod_NumForName: unknown fileid for %s", mod->name );
		break;
	}

	loadmodel->extradatasize = Hunk_End();

	FS_FreeFile( buf );

	return mod;
}

/*
===================================================================================================

	Brush models

===================================================================================================
*/

static byte *mod_base;

/*
========================
Mod_LoadLighting
========================
*/
static void Mod_LoadLighting (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = (byte*)Hunk_Alloc ( l->filelen);	
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}

/*
========================
Mod_LoadVisibility
========================
*/
static void Mod_LoadVisibility (lump_t *l)
{
	int		i;

	if (!l->filelen)
	{
		loadmodel->vis = NULL;
		return;
	}
	loadmodel->vis = (dvis_t*)Hunk_Alloc ( l->filelen);	
	memcpy (loadmodel->vis, mod_base + l->fileofs, l->filelen);

	loadmodel->vis->numclusters = LittleLong (loadmodel->vis->numclusters);
	for (i=0 ; i<loadmodel->vis->numclusters ; i++)
	{
		loadmodel->vis->bitofs[i][0] = LittleLong (loadmodel->vis->bitofs[i][0]);
		loadmodel->vis->bitofs[i][1] = LittleLong (loadmodel->vis->bitofs[i][1]);
	}
}

/*
========================
Mod_LoadVertexes
========================
*/
static void Mod_LoadVertexes (lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (dvertex_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mvertex_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
========================
RadiusFromBounds
========================
*/
static float RadiusFromBounds( vec3_t mins, vec3_t maxs )
{
	int		i;
	vec3_t	corner;

	for ( i = 0; i < 3; i++ )
	{
		corner[i] = fabs( mins[i] ) > fabs( maxs[i] ) ? fabs( mins[i] ) : fabs( maxs[i] );
	}

	return VectorLength( corner );
}

/*
========================
Mod_LoadSubmodels
========================
*/
static void Mod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	mmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mmodel_t*)Hunk_Alloc ( count*sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->radius = RadiusFromBounds (out->mins, out->maxs);
		out->headnode = LittleLong (in->headnode);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

/*
========================
Mod_LoadEdges
========================
*/
static void Mod_LoadEdges (lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (dedge_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (medge_t*)Hunk_Alloc ( (count + 1) * sizeof(*out));	

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
========================
Mod_LoadTexinfo
========================
*/
static void Mod_LoadTexinfo (lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out, *step;
	int 	i, j, count;
	char	name[MAX_QPATH];
	int		next;

	in = (texinfo_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);

		out->flags = LittleLong (in->flags);
		next = LittleLong (in->nexttexinfo);
		if (next > 0)
			out->next = loadmodel->texinfo + next;
		else
		    out->next = NULL;

		Q_sprintf_s( name, "materials/%s.mat", in->texture );
		out->material = GL_FindMaterial( name );
		if ( out->material == mat_notexture )
		{
			Com_Printf( "Couldn't load %s\n", name );
		}
	}

	// count animation frames
	for (i=0 ; i<count ; i++)
	{
		out = &loadmodel->texinfo[i];
		out->numframes = 1;
		for (step = out->next ; step && step != out ; step=step->next)
			out->numframes++;
	}
}

/*
========================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
========================
*/
static void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 99999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] + 
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = (int)floor(mins[i]/16);
		bmaxs[i] = (int)ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256 */ )
//			Com_Errorf ("Bad surface extents");
	}
}

void GL_BuildPolygonFromSurface(msurface_t *fa);
void GL_CreateSurfaceLightmap (msurface_t *surf);
void GL_EndBuildingLightmaps (void);
void GL_BeginBuildingLightmaps (model_t *m);

/*
========================
Mod_LoadFaces
========================
*/
static void Mod_LoadFaces (lump_t *l)
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;
	int			ti;

	in = (dface_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	currentmodel = loadmodel;

	GL_BeginBuildingLightmaps (loadmodel);

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;
		out->polys = NULL;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;			

		out->plane = loadmodel->planes + planenum;

		ti = LittleShort (in->texinfo);
		if (ti < 0 || ti >= loadmodel->numtexinfo)
			Com_Errorf ("MOD_LoadBmodel: bad texinfo number");
		out->texinfo = loadmodel->texinfo + ti;

		CalcSurfaceExtents (out);
				
	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;
		
	// set the drawing flags
		
		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface (out);	// cut up polygon for warps
		}

		// create lightmaps and polygons
		if ( !(out->texinfo->flags & (SURFMASK_UNLIT) ) )
			GL_CreateSurfaceLightmap (out);

		if (! (out->texinfo->flags & SURF_WARP) ) 
			GL_BuildPolygonFromSurface(out);

	}

	GL_EndBuildingLightmaps ();
}

/*
========================
Mod_SetParent
========================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
========================
Mod_LoadNodes
========================
*/
static void Mod_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = (dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mnode_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
========================
Mod_LoadLeafs
========================
*/
static void Mod_LoadLeafs (lump_t *l)
{
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;
//	glpoly_t	*poly;

	in = (dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mleaf_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstleafface);
		out->nummarksurfaces = LittleShort(in->numleaffaces);
	}	
}

/*
========================
Mod_LoadMarksurfaces
========================
*/
static void Mod_LoadMarksurfaces (lump_t *l)
{	
	int		i, j, count;
	short		*in;
	msurface_t **out;
	
	in = (short*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t**)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleShort(in[i]);
		if (j < 0 ||  j >= loadmodel->numsurfaces)
			Com_Errorf ("Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
========================
Mod_LoadSurfedges
========================
*/
static void Mod_LoadSurfedges (lump_t *l)
{	
	int		i, count;
	int		*in, *out;
	
	in = (int*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= MAX_MAP_SURFEDGES)
		Com_Errorf ("MOD_LoadBmodel: bad surfedges count in %s: %i",
		loadmodel->name, count);

	out = (int*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}

/*
========================
Mod_LoadPlanes
========================
*/
static void Mod_LoadPlanes (lump_t *l)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Errorf ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t*)Hunk_Alloc ( count*2*sizeof(*out));	
	
	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
========================
Mod_LoadBrushModel
========================
*/
void Mod_LoadBrushModel (model_t *mod, void *buffer)
{
	int			i;
	dheader_t	*header;
	mmodel_t 	*bm;
	
	loadmodel->type = mod_brush;
	if (loadmodel != mod_known)
		Com_Errorf ("Loaded a brush model after the world");

	header = (dheader_t *)buffer;

	i = LittleLong (header->version);
	if (i != BSPVERSION)
		Com_Errorf ("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	
	Mod_LoadVertexes (&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges (&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadLighting (&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces (&header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs (&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes (&header->lumps[LUMP_NODES]);
	Mod_LoadSubmodels (&header->lumps[LUMP_MODELS]);
	mod->numframes = 2;		// regular and alternate animation
	
//
// set up the submodels
//
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		model_t	*starmod;

		bm = &mod->submodels[i];
		starmod = &mod_inline[i];

		*starmod = *loadmodel;
		
		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;
		if (starmod->firstnode >= loadmodel->numnodes)
			Com_Errorf ("Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;
	
		if (i == 0)
			*loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}
}

/*
===================================================================================================

	Alias models

===================================================================================================
*/

/*
========================
Mod_LoadAliasModel
========================
*/
void Mod_LoadAliasModel (model_t *mod, void *buffer)
{
	int					i, j;
	dmdl_t				*pinmodel, *pheader;
	dstvert_t			*pinst, *poutst;
	dtriangle_t			*pintri, *pouttri;
	daliasframe_t		*pinframe, *poutframe;
	int					*pincmd, *poutcmd;
	int					version;

	pinmodel = (dmdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Com_Errorf ("%s has wrong version number (%i should be %i)",
				 mod->name, version, ALIAS_VERSION);

	pheader = (dmdl_t*)Hunk_Alloc (LittleLong(pinmodel->ofs_end));
	
	// byte swap the header fields and sanity check
	for (i=0 ; i<sizeof(dmdl_t)/4 ; i++)
		((int *)pheader)[i] = LittleLong (((int *)buffer)[i]);

	if (pheader->num_xyz <= 0)
		Com_Errorf ("model %s has no vertices", mod->name);

	if (pheader->num_xyz > MAX_VERTS)
		Com_Errorf ("model %s has too many vertices", mod->name);

	if (pheader->num_st <= 0)
		Com_Errorf ("model %s has no st vertices", mod->name);

	if (pheader->num_tris <= 0)
		Com_Errorf ("model %s has no triangles", mod->name);

	if (pheader->num_frames <= 0)
		Com_Errorf ("model %s has no frames", mod->name);

//
// load base s and t vertices (not used in gl version)
//
	pinst = (dstvert_t *) ((byte *)pinmodel + pheader->ofs_st);
	poutst = (dstvert_t *) ((byte *)pheader + pheader->ofs_st);

	for (i=0 ; i<pheader->num_st ; i++)
	{
		poutst[i].s = LittleShort (pinst[i].s);
		poutst[i].t = LittleShort (pinst[i].t);
	}

//
// load triangle lists
//
	pintri = (dtriangle_t *) ((byte *)pinmodel + pheader->ofs_tris);
	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

	for (i=0 ; i<pheader->num_tris ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort (pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort (pintri[i].index_st[j]);
		}
	}

//
// load the frames
//
	for (i=0 ; i<pheader->num_frames ; i++)
	{
		pinframe = (daliasframe_t *) ((byte *)pinmodel 
			+ pheader->ofs_frames + i * pheader->framesize);
		poutframe = (daliasframe_t *) ((byte *)pheader 
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy (poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j=0 ; j<3 ; j++)
		{
			poutframe->scale[j] = LittleFloat (pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat (pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		memcpy (poutframe->verts, pinframe->verts, 
			pheader->num_xyz*sizeof(dtrivertx_t));

	}

	mod->type = mod_alias;

	//
	// load the glcmds
	//
	pincmd = (int *) ((byte *)pinmodel + pheader->ofs_glcmds);
	poutcmd = (int *) ((byte *)pheader + pheader->ofs_glcmds);
	for (i=0 ; i<pheader->num_glcmds ; i++)
		poutcmd[i] = LittleLong (pincmd[i]);


	// register all skins
	memcpy ((char *)pheader + pheader->ofs_skins, (char *)pinmodel + pheader->ofs_skins,
		pheader->num_skins*MAX_SKINNAME);
	for (i=0 ; i<pheader->num_skins ; i++)
	{
		mod->skins[i] = GL_FindMaterial( (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME );
	}

	mod->mins[0] = -32;
	mod->mins[1] = -32;
	mod->mins[2] = -32;
	mod->maxs[0] = 32;
	mod->maxs[1] = 32;
	mod->maxs[2] = 32;
}

/*
===================================================================================================

	Studio models

===================================================================================================
*/

/*
========================
Mod_LoadStudioModel
========================
*/
void Mod_LoadStudioModel( model_t *mod, void *buffer )
{
	int					i;
	studiohdr_t			*phdr;
	mstudiotexture_t	*ptexture;

	phdr = (studiohdr_t *)Hunk_Alloc( modfilelen );
	memcpy( phdr, buffer, modfilelen ); // Don't bother with swapping

	assert( phdr->numtextures <= MAX_MD2SKINS );

	if ( phdr->textureindex != 0 )
	{
		ptexture = (mstudiotexture_t *)( (byte *)phdr + phdr->textureindex );

		for ( i = 0; i < phdr->numtextures; ++i )
		{
			// strcpy( name, mod->name );
			// strcpy( name, ptexture[i].name );
			mod->skins[i] = mat_notexture;
			ptexture[i].index = mod->skins[i]->image->texnum;
		}
	}

	mod->type = mod_studio;

	VectorCopy( phdr->min, mod->mins );
	VectorCopy( phdr->max, mod->maxs );
}

/*
===================================================================================================

	SMF models

===================================================================================================
*/

void Mod_LoadSMFModel( model_t *pModel, void *pBuffer )
{
	fmtSMF::header_t *header = (fmtSMF::header_t *)pBuffer;

	if ( header->version != fmtSMF::version )
	{
		Com_Errorf( "%s has wrong version number (%i should be %i)", pModel->name, header->version, fmtSMF::version );
	}

	mSMF_t *memSMF = (mSMF_t *)Hunk_Alloc( sizeof( mSMF_t ) );

	memSMF->material = GL_FindMaterial( "models/devtest/computer/computer.mat" /*header->materialName*/ );

	memSMF->numIndices = header->numIndices;

	glGenVertexArrays( 1, &memSMF->vao );
	glGenBuffers( 1, &memSMF->vbo );
	glGenBuffers( 1, &memSMF->ebo );

	glBindVertexArray( memSMF->vao );
	glBindBuffer( GL_ARRAY_BUFFER, memSMF->vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, memSMF->ebo );

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glEnableVertexAttribArray( 2 );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( fmtSMF::vertex_t ), (void *)( 0 ) );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( fmtSMF::vertex_t ), (void *)( 3 * sizeof( GLfloat ) ) );
	glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof( fmtSMF::vertex_t ), (void *)( 5 * sizeof( GLfloat ) ) );

	const byte *vertexData = (byte *)pBuffer + header->offsetVerts;
	const byte *indexData = (byte *)pBuffer + header->offsetIndices;

	glBufferData( GL_ARRAY_BUFFER, header->numVerts * sizeof( fmtSMF::vertex_t ), vertexData, GL_STATIC_DRAW );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, header->numIndices * sizeof( uint16 ), indexData, GL_STATIC_DRAW );

	pModel->type = mod_smf;

	pModel->mins[0] = -32;
	pModel->mins[1] = -32;
	pModel->mins[2] = -32;
	pModel->maxs[0] = 32;
	pModel->maxs[1] = 32;
	pModel->maxs[2] = 32;
}

/*
===================================================================================================

	Sprite models

===================================================================================================
*/

/*
========================
Mod_LoadSpriteModel
========================
*/
void Mod_LoadSpriteModel (model_t *mod, void *buffer)
{
	dsprite_t	*sprin, *sprout;
	int			i;

	sprin = (dsprite_t *)buffer;
	sprout = (dsprite_t*)Hunk_Alloc (modfilelen);

	sprout->ident = LittleLong (sprin->ident);
	sprout->version = LittleLong (sprin->version);
	sprout->numframes = LittleLong (sprin->numframes);

	if (sprout->version != SPRITE_VERSION)
		Com_Errorf ("%s has wrong version number (%i should be %i)",
				 mod->name, sprout->version, SPRITE_VERSION);

	if (sprout->numframes > MAX_MD2SKINS)
		Com_Errorf ("%s has too many frames (%i > %i)",
				 mod->name, sprout->numframes, MAX_MD2SKINS);

	// byte swap everything
	for (i=0 ; i<sprout->numframes ; i++)
	{
		sprout->frames[i].width = LittleLong (sprin->frames[i].width);
		sprout->frames[i].height = LittleLong (sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong (sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong (sprin->frames[i].origin_y);
		memcpy (sprout->frames[i].name, sprin->frames[i].name, MAX_SKINNAME);
		mod->skins[i] = GL_FindMaterial (sprout->frames[i].name);
	}

	mod->type = mod_sprite;
}

//=================================================================================================

/*
========================
Mod_Free
========================
*/
void Mod_Free( model_t *pModel )
{
	Hunk_Free( pModel->extradata );
	memset( pModel, 0, sizeof( *pModel ) );
}

/*
========================
Mod_FreeAll
========================
*/
void Mod_FreeAll()
{
	int i;

	for ( i = 0; i < mod_numknown; i++ )
	{
		if ( mod_known[i].extradatasize )
		{
			Mod_Free( &mod_known[i] );
		}
	}
}

//=================================================================================================

/*
========================
R_BeginRegistration

Specifies the model that will be used as the world
========================
*/
void R_BeginRegistration( const char *model )
{
	char fullname[MAX_QPATH];
	static cvar_t *flushmap;

	tr.registrationSequence++;
	r_oldviewcluster = -1;		// force markleafs

	Q_sprintf_s( fullname, "maps/%s.bsp", model );

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	if ( !flushmap ) {
		flushmap = Cvar_Get( "flushmap", "0", 0 );
	}
	if ( Q_strcmp( mod_known[0].name, fullname ) || flushmap->value ) {
		Mod_Free( &mod_known[0] );
	}
	r_worldmodel = Mod_ForName( fullname, true );

	r_viewcluster = -1;
}

/*
========================
R_RegisterModel
========================
*/
model_t *R_RegisterModel( const char *name )
{
	int					i;
	model_t *			pModel;
	dsprite_t *			pSpriteHeader;
	dmdl_t *			pAliasHeader;
	mSMF_t *			pMemSMF;

	pModel = Mod_ForName( name, false );
	if ( pModel )
	{
		pModel->registration_sequence = tr.registrationSequence;

		switch ( pModel->type )
		{
		case mod_smf:
			pMemSMF = (mSMF_t *)pModel->extradata;
			// don't register if we're the missing material
			if ( !pMemSMF->material->IsMissing() )
			{
				pMemSMF->material->Register();
			}
			break;

		case mod_alias:
			pAliasHeader = (dmdl_t *)pModel->extradata;
			for ( i = 0; i < pAliasHeader->num_skins; i++ )
			{
				pModel->skins[i] = GL_FindMaterial( (char *)pAliasHeader + pAliasHeader->ofs_skins + i * MAX_SKINNAME );
			}
//PGM
			pModel->numframes = pAliasHeader->num_frames;
//PGM
			break;

		case mod_sprite:
			pSpriteHeader = (dsprite_t *)pModel->extradata;
			for ( i = 0; i < pSpriteHeader->numframes; i++ )
			{
				pModel->skins[i] = GL_FindMaterial( pSpriteHeader->frames[i].name );
			}
			break;

		case mod_brush:
			for ( i = 0; i < pModel->numtexinfo; i++ )
			{
				pModel->texinfo[i].material->registration_sequence = tr.registrationSequence;
			}
			break;

		default:
			// get rid of this
			Com_Errorf( "R_RegisterModel: mod_bad for %s\n", pModel->name );
		}
	}

	return pModel;
}

/*
========================
R_EndRegistration
========================
*/
void R_EndRegistration()
{
	int i;
	model_t *mod;

	for ( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if ( !mod->name[0] ) {
			continue;
		}
		if ( mod->registration_sequence != tr.registrationSequence ) {
			// don't need this model
			Mod_Free( mod );
		}
	}

	GL_FreeUnusedMaterials();
}
