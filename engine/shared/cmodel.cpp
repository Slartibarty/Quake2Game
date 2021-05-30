//=================================================================================================
// Model loading
//=================================================================================================

#include "engine.h"

#include <vector>
#include <string>

#include "cmodel.h"

struct cnode_t
{
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
};

struct cbrushside_t
{
	cplane_t	*plane;
	csurface_t	*surface;
};

struct cleaf_t
{
	int			contents;
	int			cluster;
	int			area;
	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
};

struct cbrush_t
{
	int			contents;
	int			numsides;
	int			firstbrushside;
	int			checkcount;		// to avoid repeated testings
};

struct carea_t
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;
};

template< typename T >
struct cmArray_t
{
private:

	T *data;
	int count;		// The amount of Ts in use
	int reserved;	// The amount of Ts we have reserved

public:

	// Accessors

	// Return a reference to an element in this array
	T &Data( int index )
	{
		assert( index < reserved );	// Kinda sucks, we need to do this for the secret box hull crap
		return data[index];
	}

	// Return the base of this array
	T *Base()
	{
		return data;
	}

	// The amount of elements in this array
	int Count()
	{
		return count;
	}

	int SizeInBytes()
	{
		return count * sizeof( T );
	}

	// Set all used elements in this array to 0
	void Clear()
	{
		memset( data, 0, SizeInBytes() );
	}

	// Management

	// Make clients forget about the data in here
	void Forget()
	{
		count = 0;
	}

	void PrepForNewData( int newcount, int extra = 0 )
	{
		count = newcount;
		if ( newcount > reserved )
		{
			reserved = newcount + extra;
			// SlartTodo: Is it more efficient to realloc here? We don't care about our data at this point
			if ( data )
			{
				Mem_Free( data );
			}
			data = (T *)Mem_Alloc( reserved * sizeof( T ) );
		}
		// Don't bother clearing memory, it will be written over by clients
	}

	void Free()
	{
		if ( data )
		{
			Mem_Free( data );
			data = nullptr;
			reserved = 0;
			count = 0;
		}
	}
};

//
// Struct that stores data about a particular map
// Really, this could be replaced with a class
// But classes in C++ are a pain to maintain
//
struct cmMapData_t
{
	char		name[MAX_QPATH];	// Exists soley to prevent re-loading of the same map

	cmArray_t<cbrushside_t>		brushsides;
	cmArray_t<csurface_t>		surfaces;
	cmArray_t<cplane_t>			planes;				// +6 extra for box hull
	cmArray_t<cnode_t>			nodes;				// +6 extra for box hull
	cmArray_t<cleaf_t>			leafs;				// Will always have size of 1, allows leaf funcs to be called without a map
	int							emptyleaf, solidleaf;
	cmArray_t<uint16>			leafbrushes;
	cmArray_t<cmodel_t>			cmodels;
	cmArray_t<cbrush_t>			brushes;
	cmArray_t<byte>				vis;
	cmArray_t<char>				entitystring;
	cmArray_t<carea_t>			areas;
	cmArray_t<dareaportal_t>	areaportals;

	int			numclusters = 1;

	int			floodvalid;
	int			checkcount;

	cmArray_t<bool>		portalopen;

	// Reset everything that matters
	void Reset()
	{
		name[0] = 0;

		brushsides.Forget();
		surfaces.Forget();
		planes.Forget();
		nodes.Forget();
		leafs.Forget();
		leafbrushes.Forget();
		cmodels.Forget();
		brushes.Forget();
		vis.Forget();
		entitystring.Forget();
		areas.Forget();
		areaportals.Forget();

		checkcount = 0;

		portalopen.Forget();
	}

	// Clear all allocated memory
	void Free()
	{
		brushsides.Free();
		surfaces.Free();
		planes.Free();
		nodes.Free();
		leafs.Free();
		leafbrushes.Free();
		cmodels.Free();
		brushes.Free();
		vis.Free();
		entitystring.Free();
		areas.Free();
		areaportals.Free();

		portalopen.Free();
	}

	// Create a fake map for servers running cinematics to call through to
	// Slart: Isn't this really just a hack? There has to be a better way lol
	void CreateCinematicMap()
	{
		leafs.PrepForNewData( 1 );
		leafs.Clear();
		numclusters = 1;
		areas.PrepForNewData( 1 );
		areas.Clear();

		cmodels.PrepForNewData( 1 );
		cmodels.Clear();
	}
};

cmMapData_t cm;
csurface_t	s_nullsurface;


cvar_t	*cm_noareas;


// Counters
int		c_pointcontents;
int		c_traces, c_brush_traces;


void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);


/*
===============================================================================

							MAP LOADING

===============================================================================
*/

inline void VectorCopy_ByteSwapLE( const vec3_t in, vec3_t out )
{
	out[0] = LittleFloat( in[0] );
	out[1] = LittleFloat( in[1] );
	out[2] = LittleFloat( in[2] );
}

/*
=================
CMod_LoadSubmodels
=================
*/
static void CMod_LoadSubmodels( byte *cmod_base, lump_t *l )
{
	dmodel_t *in = (dmodel_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no models" );
	}

	cm.cmodels.PrepForNewData( count );
	cmodel_t *out = cm.cmodels.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			// Spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat( in->mins[j] ) - 1.0f;
			out->maxs[j] = LittleFloat( in->maxs[j] ) + 1.0f;
			out->origin[j] = LittleFloat( in->origin[j] );
		}
		out->headnode = LittleLong( in->headnode );
	}
}

/*
=================
CMod_LoadSurfaces
=================
*/
static void CMod_LoadSurfaces( byte *cmod_base, lump_t *l )
{
	texinfo_t *in = (texinfo_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no surfaces" );
	}

	cm.surfaces.PrepForNewData( count );
	csurface_t *out = cm.surfaces.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		strcpy( out->name, in->texture );
		out->flags = LittleLong( in->flags );
	}
}

/*
=================
CMod_LoadNodes
=================
*/
static void CMod_LoadNodes( byte *cmod_base, lump_t *l )
{
	dnode_t *in = (dnode_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no nodes" );
	}

	cm.nodes.PrepForNewData( count, 6 );
	cnode_t *out = cm.nodes.Base();

	for ( int i = 0; i < count; i++, out++, in++ )
	{
		out->plane = &cm.planes.Data( LittleLong( in->planenum ) );
		for ( int j = 0; j < 2; j++ )
		{
			out->children[j] = LittleLong( in->children[j] );
		}
	}
}

/*
=================
CMod_LoadBrushes
=================
*/
static void CMod_LoadBrushes( byte *cmod_base, lump_t *l )
{
	dbrush_t *in = (dbrush_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no brushes" );
	}

	cm.brushes.PrepForNewData( count, 1 );
	cbrush_t *out = cm.brushes.Base();

	for ( int i = 0; i < count; i++, out++, in++ )
	{
		out->firstbrushside = LittleLong( in->firstside );
		out->numsides = LittleLong( in->numsides );
		out->contents = LittleLong( in->contents );
	}
}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs( byte *cmod_base, lump_t *l )
{
	dleaf_t *in = (dleaf_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no leafs" );
	}

	cm.leafs.PrepForNewData( count, 1 );
	cm.numclusters = 0;
	cleaf_t *out = cm.leafs.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		out->contents = LittleLong( in->contents );
		out->cluster = LittleShort( in->cluster );
		out->area = LittleShort( in->area );
		// SlartTodo: ushort > short uTruncation
		out->firstleafbrush = LittleShort( in->firstleafbrush );
		out->numleafbrushes = LittleShort( in->numleafbrushes );

		if ( out->cluster >= cm.numclusters )
			cm.numclusters = out->cluster + 1;
	}

	if ( cm.leafs.Data( 0 ).contents != CONTENTS_SOLID )
	{
		Com_Errorf("Map leaf 0 is not CONTENTS_SOLID" );
	}

	cm.solidleaf = 0;
	cm.emptyleaf = -1;
	for ( int i = 1; i < cm.leafs.Count(); i++ )
	{
		if ( cm.leafs.Data( i ).contents == CONTENTS_EMPTY )
		{
			cm.emptyleaf = i;
			break;
		}
	}
	if ( cm.emptyleaf == -1 )
	{
		Com_Errorf("Map does not have an empty leaf" );
	}
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes( byte *cmod_base, lump_t *l )
{
	dplane_t *in = (dplane_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no planes" );
	}

	cm.planes.PrepForNewData( count, 12 );
	cplane_t *out = cm.planes.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		int bits = 0;
		for ( int j = 0; j < 3; j++ )
		{
			out->normal[j] = LittleFloat( in->normal[j] );
			if ( out->normal[j] < 0.0f )
			{
				bits |= 1 << j;
			}
		}

		out->dist = LittleFloat( in->dist );
		out->type = LittleLong( in->type );
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes( byte *cmod_base, lump_t *l )
{
	uint16 *in = (unsigned short *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no leaf brushes" );
	}

	cm.leafbrushes.PrepForNewData( count, 1 );
	uint16 *out = cm.leafbrushes.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		// SlartTodo: Truncation
		*out = LittleShort( *in );
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides( byte *cmod_base, lump_t *l )
{
	dbrushside_t *in = (dbrushside_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		Com_Errorf("Map with no brush sides" );
	}

	cm.brushsides.PrepForNewData( count, 6 );
	cbrushside_t *out = cm.brushsides.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		// SlartTodo: 2x Truncation in here
		int num = LittleShort( in->planenum );
		out->plane = &cm.planes.Data( num );
		int j = LittleShort( in->texinfo );
		if ( j >= cm.surfaces.Count() )
		{
			Com_Errorf("Bad brushside texinfo" );
		}
		out->surface = &cm.surfaces.Data( j );
	}
}

/*
=================
CMod_LoadAreas
=================
*/
static void CMod_LoadAreas( byte *cmod_base, lump_t *l )
{
	darea_t *in = (darea_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		cm.areas.Forget();
		return;
	}

	cm.areas.PrepForNewData( count );
	carea_t *out = cm.areas.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		out->numareaportals = LittleLong( in->numareaportals );
		out->firstareaportal = LittleLong( in->firstareaportal );
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

/*
=================
CMod_LoadAreaPortals
=================
*/
static void CMod_LoadAreaPortals( byte *cmod_base, lump_t *l )
{
	dareaportal_t *in = (dareaportal_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) )
	{
		Com_Errorf("CM_LoadMap: Funny lump size" );
	}
	int count = l->filelen / sizeof( *in );
	if ( count < 1 )
	{
		cm.areaportals.Forget();
		return;
	}

	cm.portalopen.PrepForNewData( count );
	cm.portalopen.Clear();

	cm.areaportals.PrepForNewData( count );
	dareaportal_t *out = cm.areaportals.Base();

	for ( int i = 0; i < count; i++, in++, out++ )
	{
		out->portalnum = LittleLong( in->portalnum );
		out->otherarea = LittleLong( in->otherarea );
	}
}

/*
=================
CMod_LoadVisibility
=================
*/
static void CMod_LoadVisibility( byte *cmod_base, lump_t *l )
{
	int count = l->filelen;

	if ( count < 1 )
	{
		cm.vis.Forget();
		return;
	}

	cm.vis.PrepForNewData( count );

	memcpy( cm.vis.Base(), cmod_base + l->fileofs, count );

	dvis_t *data = (dvis_t *)cm.vis.Base();

	data->numclusters = LittleLong( data->numclusters );
	for ( int i = 0; i < data->numclusters; i++ )
	{
		data->bitofs[i][0] = LittleLong( data->bitofs[i][0] );
		data->bitofs[i][1] = LittleLong( data->bitofs[i][1] );
	}
}

/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString( byte *cmod_base, lump_t *l )
{
	int count = l->filelen;

	if ( count < 1 )
	{
		cm.entitystring.Forget();
		return;
	}

	cm.entitystring.PrepForNewData( count );

	memcpy( cm.entitystring.Base(), cmod_base + l->fileofs, count );
}


/*
=================
CM_LoadMap

Loads in the map and all submodels
=================
*/
cmodel_t *CM_LoadMap( const char *name, bool clientload, unsigned *checksum )
{
	static unsigned	last_checksum;

	if ( !name || !name[0] )
	{
		Com_Errorf("CM_LoadMap: NULL name" );
	}

	cm_noareas = Cvar_Get( "cm_noareas", "0", 0 );

#if 0
	// Create a dummy map
	if ( !name )
	{
		cm.Reset();
		cm.CreateCinematicMap();
		*checksum = 0;
		return cm.cmodels.Base();		// cinematic servers won't have anything at all
	}

	if ( name && name[0] == '\0' )
	{
		Com_Errorf("CM_LoadMap: name was provided, but empty!" );
	}
#endif

	if ( Q_strcmp( cm.name, name ) == 0 && ( clientload || !Cvar_VariableValue( "flushmap" ) ) )
	{
		*checksum = last_checksum;
		if ( !clientload )
		{
			cm.portalopen.Clear();
			FloodAreaConnections();
		}
		return cm.cmodels.Base();		// still have the right version
	}

	// free old stuff
	cm.Reset();

	//
	// load the file
	//
	byte *buf;
	int length = FS_LoadFile( name, (void **)&buf );
	if ( !buf )
	{
		Com_Errorf("Couldn't load %s", name );
	}

	last_checksum = LittleLong( Com_BlockChecksum( buf, length ) );
	*checksum = last_checksum;

	dheader_t *header = (dheader_t *)buf;
#if 0 // Address Sanitizer aborts at this line
	for ( i = 0; i < sizeof( dheader_t ) / 4; i++ )
		( (int *)&header )[i] = LittleLong( ( (int *)&header )[i] );
#endif

	if ( header->version != BSPVERSION )
	{
		Com_Errorf("CM_LoadMap: %s has wrong version number (%d should be %d)"
			, name, header->version, BSPVERSION );
	}

	// load into heap
	CMod_LoadSurfaces( buf, &header->lumps[LUMP_TEXINFO] );
	CMod_LoadLeafs( buf, &header->lumps[LUMP_LEAFS] );
	CMod_LoadLeafBrushes( buf, &header->lumps[LUMP_LEAFBRUSHES] );
	CMod_LoadPlanes( buf, &header->lumps[LUMP_PLANES] );
	CMod_LoadBrushes( buf, &header->lumps[LUMP_BRUSHES] );
	CMod_LoadBrushSides( buf, &header->lumps[LUMP_BRUSHSIDES] );
	CMod_LoadSubmodels( buf, &header->lumps[LUMP_MODELS] );
	CMod_LoadNodes( buf, &header->lumps[LUMP_NODES] );
	CMod_LoadAreas( buf, &header->lumps[LUMP_AREAS] );
	CMod_LoadAreaPortals( buf, &header->lumps[LUMP_AREAPORTALS] );
	CMod_LoadVisibility( buf, &header->lumps[LUMP_VISIBILITY] );
	CMod_LoadEntityString( buf, &header->lumps[LUMP_ENTITIES] );

	FS_FreeFile( buf );

	CM_InitBoxHull();

	cm.portalopen.Clear();
	FloodAreaConnections();

	Q_strcpy_s( cm.name, name );

	return cm.cmodels.Base();
}

/*
==================
CM_InlineModel
==================
*/
cmodel_t *CM_InlineModel( const char *name )
{
	if ( !name || name[0] != '*' )
	{
		Com_Errorf("CM_InlineModel: bad name" );
	}
	int num = atoi( name + 1 );
	if ( num < 1 || num >= cm.cmodels.Count() )
	{
		Com_Errorf("CM_InlineModel: bad number" );
	}

	return &cm.cmodels.Data(num);
}

int CM_NumClusters()
{
	return cm.numclusters;
}

int CM_NumInlineModels()
{
	return cm.cmodels.Count();
}

char *CM_EntityString()
{
	return cm.entitystring.Base();
}

int CM_LeafContents( int leafnum )
{
	if ( leafnum < 0 || leafnum >= cm.leafs.Count() )
	{
		Com_Errorf("CM_LeafContents: bad number" );
	}
	return cm.leafs.Data(leafnum).contents;
}

int CM_LeafCluster (int leafnum)
{
	if ( leafnum < 0 || leafnum >= cm.leafs.Count() )
	{
		Com_Errorf("CM_LeafCluster: bad number" );
	}
	return cm.leafs.Data(leafnum).cluster;
}

int CM_LeafArea( int leafnum )
{
	if ( leafnum < 0 || leafnum >= cm.leafs.Count() )
	{
		Com_Errorf("CM_LeafArea: bad number" );
	}
	return cm.leafs.Data(leafnum).area;
}

//=======================================================================


cplane_t	*box_planes;
int			box_headnode;
cbrush_t	*box_brush;
cleaf_t		*box_leaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cnode_t		*c;
	cplane_t	*p;
	cbrushside_t	*s;

#if 0
	if ( cm.nodes.Count()+6 > MAX_MAP_NODES
		|| cm.brushes.Count()+1 > MAX_MAP_BRUSHES
		|| cm.leafbrushes.Count()+1 > MAX_MAP_LEAFBRUSHES
		|| cm.brushsides.Count()+6 > MAX_MAP_BRUSHSIDES
		|| cm.planes.Count()+12 > MAX_MAP_PLANES )
		Com_Errorf("Not enough room for box tree" );
#endif

	// Slart: This code assumes a spoof of the count, evil

	box_headnode = cm.nodes.Count();
	box_planes = &cm.planes.Data( cm.planes.Count() );

	box_brush = &cm.brushes.Data( cm.brushes.Count() );
	box_brush->numsides = 6;
	box_brush->firstbrushside = cm.brushsides.Count();
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &cm.leafs.Data( cm.leafs.Count() );
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = cm.leafbrushes.Count();
	box_leaf->numleafbrushes = 1;

	cm.leafbrushes.Data( cm.leafbrushes.Count() ) = cm.brushes.Count();

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &cm.brushsides.Data(cm.brushsides.Count()+i);
		s->plane = &cm.planes.Data(cm.planes.Count()+i*2+side);
		s->surface = &s_nullsurface;

		// nodes
		c = &cm.nodes.Data(box_headnode+i);
		c->plane = &cm.planes.Data(cm.planes.Count()+i*2);
		c->children[side] = -1 - cm.emptyleaf;
		if (i != 5)
			c->children[side^1] = box_headnode+i + 1;
		else
			c->children[side^1] = -1 - cm.leafs.Count();

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}	
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r( vec3_t p, int num )
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	while ( num >= 0 )
	{
		node = &cm.nodes.Data( num );
		plane = node->plane;

		if ( plane->type < 3 )
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct( plane->normal, p ) - plane->dist;
		if ( d < 0 )
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int CM_PointLeafnum( vec3_t p )
{
	if ( cm.planes.Count() == 0 )
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r( p, 0 );
}


/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leaf_count, leaf_maxcount;
int		*leaf_list;
float	*leaf_mins, *leaf_maxs;
int		leaf_topnode;

void CM_BoxLeafnums_r( int nodenum )
{
	cplane_t *plane;
	cnode_t *node;
	int s;

	while ( 1 )
	{
		if ( nodenum < 0 )
		{
			if ( leaf_count >= leaf_maxcount )
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}

		node = &cm.nodes.Data( nodenum );
		plane = node->plane;
		s = BoxOnPlaneSide( leaf_mins, leaf_maxs, plane );
		if ( s == 1 )
			nodenum = node->children[0];
		else if ( s == 2 )
			nodenum = node->children[1];
		else
		{	// go down both
			if ( leaf_topnode == -1 )
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r( node->children[0] );
			nodenum = node->children[1];
		}

	}
}

int	CM_BoxLeafnums_headnode (vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode )
{
	return CM_BoxLeafnums_headnode( mins, maxs, list,
		listsize, cm.cmodels.Base()->headnode, topnode );
}


/*
==================
CM_PointContents

==================
*/
int CM_PointContents( vec3_t p, int headnode )
{
	int l;

	if ( cm.nodes.Count() == 0 )	// map not loaded
		return 0;

	l = CM_PointLeafnum_r( p, headnode );

	return cm.leafs.Data( l ).contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents( vec3_t p, int headnode, vec3_t origin, vec3_t angles )
{
	vec3_t p_l;
	vec3_t temp;
	vec3_t forward, right, up;
	int l;

	// subtract origin offset
	VectorSubtract( p, origin, p_l );

	// rotate start and end into the models frame of reference
	if ( headnode != box_headnode &&
		( angles[0] || angles[1] || angles[2] ) )
	{
		AngleVectors( angles, forward, right, up );

		VectorCopy( p_l, temp );
		p_l[0] = DotProduct( temp, forward );
		p_l[1] = -DotProduct( temp, right );
		p_l[2] = DotProduct( temp, up );
	}

	l = CM_PointLeafnum_r( p_l, headnode );

	return cm.leafs.Data( l ).contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/8 epsilon to keep floating point happy
#define	DIST_EPSILON	0.125f

#define NEVER_UPDATED	-99999.0f

static vec3_t	trace_start, trace_end;
static vec3_t	trace_mins, trace_maxs;
static vec3_t	trace_extents;

static trace_t	trace_trace;
static int		trace_contents;
static bool		trace_ispoint;		// optimized case

/*
================
CM_ClipBoxToBrush
================
*/
void CM_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
					  trace_t *trace, cbrush_t *brush)
{
	int			i;
	cplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	bool		getout, startout;
	float		f;
	cbrushside_t	*side, *leadside;

	if (!brush->numsides)
		return;

	enterfrac = NEVER_UPDATED;
	leavefrac = 1.0f;
	clipplane = NULL;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &cm.brushsides.Data(brush->firstbrushside+i);
		plane = side->plane;

		// FIXME: special case for axial

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			ofs[0] = ( plane->normal[0] < 0.0f ) ? maxs[0] : mins[0];
			ofs[1] = ( plane->normal[1] < 0.0f ) ? maxs[1] : mins[1];
			ofs[2] = ( plane->normal[2] < 0.0f ) ? maxs[2] : mins[2];

			dist = plane->dist - DotProduct (ofs, plane->normal);
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		// if completely in front of face, no intersection
		if(d1 > 0.0f)
		{
			startout = true;

			if(d2 > 0.0f)
				return;
		}
		else
		{
			if(d2 <= 0.0f)
				continue;

			getout = true;
		}

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			// Quake 3 addition (verified needed):
			if (f < 0.0f)
			{
				f = 0.0f;
			}
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			assert( f <= 1.0f );
			// Quake 3 addition (not verified):
			/*if ( f > 1.0f )
			{
				f = 1.0f;
			}*/
			if (f < leavefrac)
			{
				leavefrac = f;
			}
		}
	}

	//
	// all planes have been checked, and the trace was not
	// completely outside the brush
	//
	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
		{
			trace->allsolid = true;
			trace->fraction = 0.0f;
			trace->contents = brush->contents;
		}
		return;
	}

	if (enterfrac < leavefrac)
	{
		if (enterfrac > NEVER_UPDATED && enterfrac < trace->fraction)
		{
			if (enterfrac < 0.0f)
				enterfrac = 0.0f;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = leadside->surface;
			trace->contents = brush->contents;
		}
	}
}

/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, cbrush_t *brush)
{
	int			i;
	cplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &cm.brushsides.Data(brush->firstbrushside+i);
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		ofs[0] = ( plane->normal[0] < 0.0f ) ? maxs[0] : mins[0];
		ofs[1] = ( plane->normal[1] < 0.0f ) ? maxs[1] : mins[1];
		ofs[2] = ( plane->normal[2] < 0.0f ) ? maxs[2] : mins[2];

		dist = plane->dist - DotProduct (ofs, plane->normal);

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0.0f)
			return;
	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &cm.leafs.Data(leafnum);
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = cm.leafbrushes.Data(leaf->firstleafbrush+k);
		b = &cm.brushes.Data(brushnum);
		if (b->checkcount == cm.checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = cm.checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &cm.leafs.Data(leafnum);
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = cm.leafbrushes.Data(leaf->firstleafbrush+k);
		b = &cm.brushes.Data(brushnum);
		if (b->checkcount == cm.checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = cm.checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
==================
CM_RecursiveHullCheck

==================
*/
void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	while( num >= 0 )
	{
		node = &cm.nodes.Data(num);
		plane = node->plane;

		if (plane->type < 3)
		{
			t1 = p1[plane->type] - plane->dist;
			t2 = p2[plane->type] - plane->dist;
			offset = trace_extents[plane->type];
		}
		else
		{
			t1 = DotProduct (plane->normal, p1) - plane->dist;
			t2 = DotProduct (plane->normal, p2) - plane->dist;
			if (trace_ispoint)
				offset = 0.0f;
			else
#if 1
				offset = (fabs(trace_extents[0]*plane->normal[0]) +
					fabs(trace_extents[1]*plane->normal[1]) +
					fabs(trace_extents[2]*plane->normal[2]) * 3.0f);
#else
				// Quake 3 does this instead?
				// Q3 dev: "this is silly"
				offset = 2048.0f;
#endif
		}

		// see which sides we need to consider
		if (t1 > offset && t2 > offset )
		{
			num = node->children[0];
			continue;
		}
		if (t1 < -offset && t2 < -offset)
		{
			num = node->children[1];
			continue;
		}

		break;
	}

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (-1-num);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0f/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset - DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0f/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1.0f;
		frac2 = 0.0f;
	}

	// move up to the node
	frac = Clamp( frac, 0.0f, 1.0f );
	midf = p1f + (p2f - p1f)*frac;
	VectorLerp( p1, p2, frac, mid );

	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);

	// go past the node
	frac2 = Clamp( frac2, 0.0f, 1.0f );
	midf = p1f + (p2f - p1f)*frac2;
	VectorLerp( p1, p2, frac2, mid );

	CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}



//======================================================================

/*
==================
CM_BoxTrace
==================
*/
trace_t CM_BoxTrace (vec3_t start, vec3_t end,
					 vec3_t mins, vec3_t maxs,
					 int headnode, int brushmask)
{
	cm.checkcount++;	// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &s_nullsurface;

	if (cm.nodes.Count() == 0)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	//
	// check for position test special case
	//
	if (VectorCompare(start, end))
	{
		int		leafs[1024];
		int		i, numleafs;
		vec3_t	c1, c2;
		int		topnode;

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			CM_TestInLeaf (leafs[i]);
			if (trace_trace.allsolid)
				break;
		}
		VectorCopy (start, trace_trace.endpos);
		return trace_trace;
	}

	//
	// check for point special case
	//
	if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
		&& maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
	{
		trace_ispoint = true;
		VectorClear (trace_extents);
	}
	else
	{
		trace_ispoint = false;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	//
	// general sweeping through world
	//
	CM_RecursiveHullCheck (headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1.0f)
	{
		VectorCopy (end, trace_trace.endpos);
	}
	else
	{
		VectorLerp( start, end, trace_trace.fraction, trace_trace.endpos );
	}
	return trace_trace;
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
void CM_TransformedBoxTrace (vec3_t start, vec3_t end,
							 vec3_t mins, vec3_t maxs,
							 int headnode, int brushmask,
							 vec3_t origin, vec3_t angles,
							 trace_t &trace)
{
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	bool		rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	rotated = ( headnode != box_headnode && ( angles[0] || angles[1] || angles[2] ) );

	if (rotated)
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace (start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && trace.fraction != 1.0f)
	{
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	VectorLerp( start, end, trace.fraction, trace.endpos );
}


/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
void CM_DecompressVis (byte *in, byte *out)
{
	int		c;
	byte	*out_p;
	int		row;

	row = (cm.numclusters+7)>>3;	
	out_p = out;

	if (!in || cm.vis.Count() == 0)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Com_DPrintf ("warning: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

byte	pvsrow[MAX_MAP_LEAFS/8];
byte	phsrow[MAX_MAP_LEAFS/8];

byte *CM_ClusterPVS (int cluster)
{
	if (cluster == -1)
		memset (pvsrow, 0, (cm.numclusters+7)>>3);
	else
		CM_DecompressVis (&cm.vis.Data(((dvis_t *)(cm.vis.Base()))->bitofs[cluster][DVIS_PVS]), pvsrow);
	return pvsrow;
}

byte *CM_ClusterPHS (int cluster)
{
	if (cluster == -1)
		memset (phsrow, 0, (cm.numclusters+7)>>3);
	else
		CM_DecompressVis (&cm.vis.Data(((dvis_t *)(cm.vis.Base()))->bitofs[cluster][DVIS_PHS]), phsrow);
	return phsrow;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void FloodArea_r (carea_t *area, int floodnum)
{
	int		i;
	dareaportal_t	*p;

	if (area->floodvalid == cm.floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		Com_Errorf ("FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = cm.floodvalid;
	p = &cm.areaportals.Data(area->firstareaportal-1);
	for (i=0 ; i<area->numareaportals ; i++, p++)
	{
		if (cm.portalopen.Data(p->portalnum))
			FloodArea_r (&cm.areas.Data(p->otherarea), floodnum);
	}
}

/*
====================
FloodAreaConnections


====================
*/
void	FloodAreaConnections (void)
{
	int		i;
	carea_t	*area;
	int		floodnum;

	// nothing to do if we have no areaportals
	if ( cm.areaportals.Count() == 0 )
	{
		return;
	}

	// all current floods are now invalid
	cm.floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<cm.areas.Count() ; i++)
	{
		area = &cm.areas.Data(i);
		if (area->floodvalid == cm.floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (area, floodnum);
	}

}

void	CM_SetAreaPortalState (int portalnum, qboolean open)
{
	if (portalnum > cm.areaportals.Count())
		Com_Errorf ("areaportal > numareaportals");

	cm.portalopen.Data(portalnum) = open;
	FloodAreaConnections ();
}

qboolean	CM_AreasConnected (int area1, int area2)
{
	if (cm_noareas->value)
		return true;

	if (area1 > cm.areas.Count() || area2 > cm.areas.Count())
		Com_Errorf ("area > numareas");

	if (cm.areas.Data(area1).floodnum == cm.areas.Data(area2).floodnum)
		return true;
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (cm.areas.Count()+7)>>3;

	if (cm_noareas->value)
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		floodnum = cm.areas.Data(area).floodnum;
		for (i=0 ; i<cm.areas.Count() ; i++)
		{
			if (cm.areas.Data(i).floodnum == floodnum || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void CM_WritePortalState( FILE *f )
{
	size_t count = cm.portalopen.Count();
	fwrite( &count, sizeof( count ), 1, f );
	fwrite( cm.portalopen.Base(), sizeof( bool ), count, f );
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void CM_ReadPortalState (FILE *f)
{
	size_t count;
	FS_Read( &count, sizeof( count ), f );
	cm.portalopen.PrepForNewData( count );
	FS_Read( cm.portalopen.Base(), count * sizeof( bool ), f );
	FloodAreaConnections ();
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
bool CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	int		leafnum;
	int		cluster;
	cnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = cm.leafs.Data(leafnum).cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &cm.nodes.Data(nodenum);
	if (CM_HeadnodeVisible(node->children[0], visbits))
		return true;
	return CM_HeadnodeVisible(node->children[1], visbits);
}


void CM_Shutdown()
{
	cm.Free();
}
