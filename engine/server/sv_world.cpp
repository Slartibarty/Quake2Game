// World query functions

#include "sv_local.h"

/*
=================
RadiusFromBounds
=================
*/
static float RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
	int		i;
	vec3_t	corner;
	float	a, b;

	for ( i = 0; i < 3; i++ )
	{
		a = fabs( mins[i] );
		b = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength( corner );
}

/*
================
SV_ClipHandleForEntity

Returns a headnode that can be used for testing or clipping to a
given entity.  If the entity is a bsp model, the headnode will
be returned, otherwise a custom box tree will be constructed.
================
*/
int SV_ClipHandleForEntity( const sharedEntity_t *ent )
{
	if ( ent->bmodel ) {
		// explicit hulls in the BSP model
		return CM_InlineModel_Q3( ent->s.modelindex )->headnode;
	}

	// create a temp tree from bounding box sizes
	return CM_HeadnodeForBox( ent->mins, ent->maxs );
}

/*
===============================================================================

ENTITY CHECKING

To avoid linearly searching through lists of entities during environment testing,
the world is carved up with an evenly spaced, axially aligned bsp tree.  Entities
are kept in chains either at the final leafs, or at the first node that splits
them, which prevents having to deal with multiple fragments of a single entity.

===============================================================================
*/

struct worldSector_t
{
	int				axis;		// -1 = leaf node
	float			dist;
	worldSector_t *	children[2];
	svEntity_t *	entities;
};

#define	AREA_DEPTH	4
#define	AREA_NODES	64

worldSector_t	sv_worldSectors[AREA_NODES];
int				sv_numworldSectors;

/*
===============
SV_SectorList_f
===============
*/
void SV_SectorList_f()
{
	int				i, c;
	worldSector_t	*sec;
	svEntity_t		*ent;

	for ( i = 0; i < AREA_NODES; i++ ) {
		sec = &sv_worldSectors[i];

		c = 0;
		for ( ent = sec->entities; ent; ent = ent->nextEntityInWorldSector ) {
			c++;
		}
		Com_Printf( "sector %i: %i entities\n", i, c );
	}
}

/*
===============
SV_CreateworldSector

Builds a uniformly subdivided tree for the given world size
===============
*/
worldSector_t *SV_CreateworldSector( int depth, vec3_t mins, vec3_t maxs )
{
	worldSector_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &sv_worldSectors[sv_numworldSectors];
	sv_numworldSectors++;

	if ( depth == AREA_DEPTH ) {
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}

	VectorSubtract( maxs, mins, size );
	if ( size[0] > size[1] ) {
		anode->axis = 0;
	}
	else {
		anode->axis = 1;
	}

	anode->dist = 0.5 * ( maxs[anode->axis] + mins[anode->axis] );
	VectorCopy( mins, mins1 );
	VectorCopy( mins, mins2 );
	VectorCopy( maxs, maxs1 );
	VectorCopy( maxs, maxs2 );

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateworldSector( depth + 1, mins2, maxs2 );
	anode->children[1] = SV_CreateworldSector( depth + 1, mins1, maxs1 );

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld()
{
	memset( sv_worldSectors, 0, sizeof( sv_worldSectors ) );
	sv_numworldSectors = 0;

	// get world map bounds
	cmodel_t *h = CM_InlineModel_Q3( 0 );

	SV_CreateworldSector( 0, h->mins, h->maxs );
}

/*
===============
SV_UnlinkEntity

===============
*/
void SV_UnlinkEntity( sharedEntity_t *gEnt )
{
	svEntity_t		*ent;
	svEntity_t		*scan;
	worldSector_t	*ws;

	ent = SV_SvEntityForSharedEntity( gEnt );

	gEnt->linked = false;

	ws = ent->worldSector;
	if ( !ws ) {
		return;		// not linked in anywhere
	}
	ent->worldSector = NULL;

	if ( ws->entities == ent ) {
		ws->entities = ent->nextEntityInWorldSector;
		return;
	}

	for ( scan = ws->entities; scan; scan = scan->nextEntityInWorldSector ) {
		if ( scan->nextEntityInWorldSector == ent ) {
			scan->nextEntityInWorldSector = ent->nextEntityInWorldSector;
			return;
		}
	}

	Com_Printf( "WARNING: SV_UnlinkEntity: not found in worldSector\n" );
}

/*
===============
SV_LinkEntity

===============
*/
#define MAX_TOTAL_ENT_LEAFS		128
void SV_LinkEntity( sharedEntity_t *gEnt )
{
	worldSector_t	*node;
	int			leafs[MAX_TOTAL_ENT_LEAFS];
	int			cluster;
	int			num_leafs;
	int			i, j, k;
	int			area;
	int			lastLeaf;
	float		*origin, *angles;
	svEntity_t	*ent;

	ent = SV_SvEntityForSharedEntity( gEnt );

	if ( ent->worldSector ) {
		SV_UnlinkEntity( gEnt );	// unlink from old position
	}

	// encode the size into the entityState_t for client prediction
	if ( gEnt->bmodel ) {
		gEnt->s.solid = SOLID_BMODEL;		// a solid_box will never create this value
	}
	else if ( gEnt->contents & ( CONTENTS_SOLID | CONTENTS_MONSTER ) ) {
	 // assume that x/y are equal and symetric
		i = gEnt->maxs[0];
		if ( i < 1 )
			i = 1;
		if ( i > 255 )
			i = 255;

		// z is not symetric
		j = ( -gEnt->mins[2] );
		if ( j < 1 )
			j = 1;
		if ( j > 255 )
			j = 255;

		// and z maxs can be negative...
		k = ( gEnt->maxs[2] + 32 );
		if ( k < 1 )
			k = 1;
		if ( k > 255 )
			k = 255;

		gEnt->s.solid = ( k << 16 ) | ( j << 8 ) | i;
	}
	else {
		gEnt->s.solid = 0;
	}

	// get the position
	origin = gEnt->currentOrigin;
	angles = gEnt->currentAngles;

	// set the abs box
	if ( gEnt->bmodel && ( angles[0] || angles[1] || angles[2] ) ) {
		// expand for rotation
		float		max;
		int			i;

		max = RadiusFromBounds( gEnt->mins, gEnt->maxs );
		for ( i = 0; i < 3; i++ ) {
			gEnt->absmin[i] = origin[i] - max;
			gEnt->absmax[i] = origin[i] + max;
		}
	}
	else {
	 // normal
		VectorAdd( origin, gEnt->mins, gEnt->absmin );
		VectorAdd( origin, gEnt->maxs, gEnt->absmax );
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	gEnt->absmin[0] -= 1;
	gEnt->absmin[1] -= 1;
	gEnt->absmin[2] -= 1;
	gEnt->absmax[0] += 1;
	gEnt->absmax[1] += 1;
	gEnt->absmax[2] += 1;

	// link to PVS leafs
	ent->numClusters = 0;
	ent->lastCluster = 0;
	ent->areanum = -1;
	ent->areanum2 = -1;

	//get all leafs, including solids
	num_leafs = CM_BoxLeafnums( gEnt->absmin, gEnt->absmax,
		leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf );

	// if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if ( !num_leafs ) {
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for ( i = 0; i < num_leafs; i++ ) {
		area = CM_LeafArea( leafs[i] );
		if ( area != -1 ) {
			// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if ( ent->areanum != -1 && ent->areanum != area ) {
				if ( ent->areanum2 != -1 && ent->areanum2 != area && sv.state == ss_loading ) {
					Com_DPrintf( "Object %i touching 3 areas at %f %f %f\n",
						gEnt->s.number,
						gEnt->absmin[0], gEnt->absmin[1], gEnt->absmin[2] );
				}
				ent->areanum2 = area;
			}
			else {
				ent->areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	ent->numClusters = 0;
	for ( i = 0; i < num_leafs; i++ ) {
		cluster = CM_LeafCluster( leafs[i] );
		if ( cluster != -1 ) {
			ent->clusternums[ent->numClusters++] = cluster;
			if ( ent->numClusters == MAX_ENT_CLUSTERS ) {
				break;
			}
		}
	}

	// store off a last cluster if we need to
	if ( i != num_leafs ) {
		ent->lastCluster = CM_LeafCluster( lastLeaf );
	}

	gEnt->linkcount++;

	// find the first world sector node that the ent's box crosses
	node = sv_worldSectors;
	while ( 1 )
	{
		if ( node->axis == -1 )
			break;
		if ( gEnt->absmin[node->axis] > node->dist )
			node = node->children[0];
		else if ( gEnt->absmax[node->axis] < node->dist )
			node = node->children[1];
		else
			break;		// crosses the node
	}

	// link it in
	ent->worldSector = node;
	ent->nextEntityInWorldSector = node->entities;
	node->entities = ent;

	gEnt->linked = true;
}

/*
============================================================================

AREA QUERY

Fills in a list of all entities who's absmin / absmax intersects the given
bounds.  This does NOT mean that they actually touch in the case of bmodels.
============================================================================
*/

struct areaParms_t
{
	const float	*mins;
	const float	*maxs;
	int			*list;
	int			count, maxcount;
};

/*
====================
SV_AreaEntities_r

====================
*/
static void SV_AreaEntities_r( worldSector_t *node, areaParms_t &ap )
{
	svEntity_t	*check, *next;
	sharedEntity_t *gcheck;
	int			count;

	count = 0;

	for ( check = node->entities; check; check = next ) {
		next = check->nextEntityInWorldSector;

		gcheck = SV_SharedEntityForSvEntity( check );

		if ( gcheck->absmin[0] > ap.maxs[0]
			|| gcheck->absmin[1] > ap.maxs[1]
			|| gcheck->absmin[2] > ap.maxs[2]
			|| gcheck->absmax[0] < ap.mins[0]
			|| gcheck->absmax[1] < ap.mins[1]
			|| gcheck->absmax[2] < ap.mins[2] ) {
			continue;
		}

		if ( ap.count == ap.maxcount ) {
			Com_Print( "SV_AreaEntities: MAXCOUNT\n" );
			return;
		}

		ap.list[ap.count] = check - sv.svEntities;
		ap.count++;
	}

	if ( node->axis == -1 ) {
		return;		// terminal node
	}

	// recurse down both sides
	if ( ap.maxs[node->axis] > node->dist ) {
		SV_AreaEntities_r( node->children[0], ap );
	}
	if ( ap.mins[node->axis] < node->dist ) {
		SV_AreaEntities_r( node->children[1], ap );
	}
}

/*
================
SV_AreaEntities
================
*/
int SV_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount )
{
	areaParms_t ap;

	ap.mins = mins;
	ap.maxs = maxs;
	ap.list = entityList;
	ap.count = 0;
	ap.maxcount = maxcount;

	SV_AreaEntities_r( sv_worldSectors, ap );

	return ap.count;
}

//===========================================================================

struct moveclip_t {
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	const float	*mins;
	const float *maxs;	// size of the moving object
	const float	*start;
	vec3_t		end;
	trace_t		trace;
	int			passEntityNum;
	int			contentmask;
	int			capsule;
};

/*
====================
SV_ClipToEntity

====================
*/
void SV_ClipToEntity( trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule )
{
	sharedEntity_t	*touch;
	int				clipHandle;
	float			*origin, *angles;

	touch = SV_SharedEntityForNum( entityNum );

	memset( trace, 0, sizeof( trace_t ) );

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if ( !( contentmask & touch->contents ) ) {
		trace->fraction = 1.0f;
		return;
	}

	// might intersect, so do an exact clip
	clipHandle = SV_ClipHandleForEntity( touch );

	origin = touch->currentOrigin;
	angles = touch->currentAngles;

	if ( !touch->bmodel ) {
		angles = vec3_origin;	// boxes don't rotate
	}

	CM_TransformedBoxTrace( (float *)start, (float *)end,
		(float *)mins, (float *)maxs, clipHandle, contentmask,
		origin, angles, *trace );

	if ( trace->fraction < 1.0f ) {
		trace->entityNum = touch->s.number;
	}
}

/*
====================
SV_ClipMoveToEntities

====================
*/
void SV_ClipMoveToEntities( moveclip_t *clip ) {
	int			i, num;
	int			touchlist[MAX_EDICTS];
	sharedEntity_t *touch;
	int			passOwnerNum;
	trace_t		trace;
	int			clipHandle;
	float		*origin, *angles;

	num = SV_AreaEntities( clip->boxmins, clip->boxmaxs, touchlist, MAX_EDICTS );

	if ( clip->passEntityNum != ENTITYNUM_NONE ) {
		passOwnerNum = SV_SharedEntityForNum( clip->passEntityNum )->ownerNum;
		if ( passOwnerNum == ENTITYNUM_NONE ) {
			passOwnerNum = -1;
		}
	}
	else {
		passOwnerNum = -1;
	}

	for ( i = 0; i < num; i++ ) {
		if ( clip->trace.allsolid ) {
			return;
		}
		touch = SV_SharedEntityForNum( touchlist[i] );

		// see if we should ignore this entity
		if ( clip->passEntityNum != ENTITYNUM_NONE ) {
			if ( touchlist[i] == clip->passEntityNum ) {
				continue;	// don't clip against the pass entity
			}
			if ( touch->ownerNum == clip->passEntityNum ) {
				continue;	// don't clip against own missiles
			}
			if ( touch->ownerNum == passOwnerNum ) {
				continue;	// don't clip against other missiles from our owner
			}
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if ( !( clip->contentmask & touch->contents ) ) {
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle = SV_ClipHandleForEntity( touch );

		origin = touch->currentOrigin;
		angles = touch->currentAngles;


		if ( !touch->bmodel ) {
			angles = vec3_origin;	// boxes don't rotate
		}

		CM_TransformedBoxTrace( (float *)clip->start, (float *)clip->end,
			(float *)clip->mins, (float *)clip->maxs, clipHandle, clip->contentmask,
			origin, angles, trace );

		if ( trace.allsolid ) {
			clip->trace.allsolid = true;
			trace.entityNum = touch->s.number;
		}
		else if ( trace.startsolid ) {
			clip->trace.startsolid = true;
			trace.entityNum = touch->s.number;
		}

		if ( trace.fraction < clip->trace.fraction ) {
			bool oldStart;

			// make sure we keep a startsolid from a previous trace
			oldStart = clip->trace.startsolid;

			trace.entityNum = touch->s.number;
			clip->trace = trace;
			clip->trace.startsolid |= oldStart;
		}
	}
}

/*
==================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.
passEntityNum and entities owned by passEntityNum are explicitly not checked.
==================
*/
void SV_Trace( trace_t *results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask )
{
	moveclip_t	clip;
	int			i;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	memset( &clip, 0, sizeof( moveclip_t ) );

	// clip to world
	clip.trace = CM_BoxTrace( (float *)start, (float *)end, mins, maxs, 0, contentmask );
	clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	if ( clip.trace.fraction == 0 ) {
		*results = clip.trace;
		return;		// blocked immediately by the world
	}

	clip.contentmask = contentmask;
	clip.start = start;
//	VectorCopy( clip.trace.endpos, clip.end );
	VectorCopy( end, clip.end );
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passEntityNum = passEntityNum;
	clip.capsule = 0;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for ( i = 0; i < 3; i++ )
	{
		if ( end[i] > start[i] ) {
			clip.boxmins[i] = clip.start[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.end[i] + clip.maxs[i] + 1;
		}
		else {
			clip.boxmins[i] = clip.end[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.start[i] + clip.maxs[i] + 1;
		}
	}

	// clip to other solid entities
	SV_ClipMoveToEntities( &clip );

	*results = clip.trace;
}

/*
=============
SV_PointContents
=============
*/
int SV_PointContents( const vec3_t p, int passEntityNum )
{
	int			touch[MAX_GENTITIES];
	sharedEntity_t *hit;
	int			i, num;
	int			contents, c2;
	int			clipHandle;
	float		*angles;

	// get base contents from world
	contents = CM_PointContents( (float *)p, 0 );

	// or in contents from all the other entities
	num = SV_AreaEntities( p, p, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		if ( touch[i] == passEntityNum ) {
			continue;
		}
		hit = SV_SharedEntityForNum( touch[i] );
		// might intersect, so do an exact clip
		clipHandle = SV_ClipHandleForEntity( hit );
		angles = hit->s.angles;
		if ( !hit->bmodel ) {
			angles = vec3_origin;	// boxes don't rotate
		}

		c2 = CM_TransformedPointContents( (float *)p, clipHandle, hit->s.origin, hit->s.angles );

		contents |= c2;
	}

	return contents;
}
