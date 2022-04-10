// World query functions

#include "sv_local.h"

/*
===================================================================================================

	Entity Checking

	To avoid linearly searching through lists of entities during environment testing,
	the world is carved up with an evenly spaced, axially aligned bsp tree.  Entities
	are kept in chains either at the final leafs, or at the first node that splits
	them, which prevents having to deal with multiple fragments of a single entity.

===================================================================================================
*/

struct worldSector_t
{
	int axis;		// -1 = leaf node
	float dist;
	worldSector_t *children[2];
	link_t trigger_edicts;
	link_t solid_edicts;
};

// FIXME: Do this the Quake 3 way!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (intptr_t)&(((t *)0)->m)))
#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

#define	AREA_DEPTH	4
#define	AREA_NODES	32

static worldSector_t	sv_worldSectors[AREA_NODES];
static int				sv_numWorldSectors;

//===============================================

// ClearLink is used for new headnodes
static void ClearLink( link_t *l )
{
	l->prev = l->next = l;
}

static void RemoveLink( link_t *l )
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

static void InsertLinkBefore( link_t *l, link_t *before )
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

/*
========================
SV_SectorList_f
========================
*/
void SV_SectorList_f()
{
#if 0
	int				i, c;
	worldSector_t	*sec;
	edict_t			*ent;

	for ( i = 0; i < AREA_NODES; ++i )
	{
		sec = &sv_worldSectors[i];

		c = 0;
		for ( ent = sec->entities; ent; ent = ent->nextEntityInWorldSector ) {
			c++;
		}
		Com_Printf( "sector %i: %i entities\n", i, c );
	}
#endif
}

/*
========================
SV_CreateWorldSector

Builds a uniformly subdivided tree for the given world size
========================
*/
static worldSector_t *SV_CreateWorldSector( int depth, vec3_t mins, vec3_t maxs )
{
	worldSector_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &sv_worldSectors[sv_numWorldSectors];
	++sv_numWorldSectors;

	ClearLink( &anode->trigger_edicts );
	ClearLink( &anode->solid_edicts );

	if ( depth == AREA_DEPTH ) {
		anode->axis = -1;
		anode->children[0] = nullptr;
		anode->children[1] = nullptr;
		return anode;
	}

	VectorSubtract( maxs, mins, size );
	if ( size[0] > size[1] ) {
		anode->axis = 0;
	} else {
		anode->axis = 1;
	}

	anode->dist = 0.5f * ( maxs[anode->axis] + mins[anode->axis] );
	VectorCopy( mins, mins1 );
	VectorCopy( mins, mins2 );
	VectorCopy( maxs, maxs1 );
	VectorCopy( maxs, maxs2 );

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateWorldSector( depth + 1, mins2, maxs2 );
	anode->children[1] = SV_CreateWorldSector( depth + 1, mins1, maxs1 );

	return anode;
}

/*
========================
SV_ClearWorld
========================
*/
void SV_ClearWorld()
{
	memset( sv_worldSectors, 0, sizeof( sv_worldSectors ) );
	sv_numWorldSectors = 0;

	cmodel_t *world = sv.models[1];

	if ( world ) {
		SV_CreateWorldSector( 0, world->mins, world->maxs );
	}
}

/*
========================
SV_UnlinkEntity
========================
*/
void SV_UnlinkEntity( edict_t *ent )
{
	if ( !ent->area.prev ) {
		// not linked in anywhere
		return;
	}

	RemoveLink( &ent->area );
	ent->area.prev = nullptr;
	ent->area.next = nullptr;
}

/*
========================
SV_LinkEntity
========================
*/
#define MAX_TOTAL_ENT_LEAFS		128
void SV_LinkEntity( edict_t *ent )
{
	worldSector_t	*node;
	int			leafs[MAX_TOTAL_ENT_LEAFS];
	int			clusters[MAX_TOTAL_ENT_LEAFS];
	int			num_leafs;
	int			i, j, k;
	int			area;
	int			topnode;

	if ( ent->area.prev ) {
		// unlink from old position
		SV_UnlinkEntity( ent );
	}
		
	if ( ent == ge->edicts ) {
		// don't add the world
		return;
	}

	if ( !ent->inuse ) {
		return;
	}

	// set the size
	VectorSubtract( ent->maxs, ent->mins, ent->size );
	
	// encode the size into the entity_state for client prediction
	if ( ent->solid == SOLID_BBOX && !( ent->svflags & SVF_DEADMONSTER ) )
	{
		// assume that x/y are equal and symetric
		i = ent->maxs[0]/8;
		if (i<1)
			i = 1;
		if (i>31)
			i = 31;

		// z is not symetric
		j = (-ent->mins[2])/8;
		if (j<1)
			j = 1;
		if (j>31)
			j = 31;

		// and z maxs can be negative...
		k = (ent->maxs[2]+32)/8;
		if (k<1)
			k = 1;
		if (k>63)
			k = 63;

		ent->s.solid = (k<<10) | (j<<5) | i;
	}
	else if ( ent->solid == SOLID_BSP )
	{
		// a solid_bbox will never create this value
		ent->s.solid = 31;
	}
	else
	{
		ent->s.solid = 0;
	}

	// set the abs box
	if ( ent->solid == SOLID_BSP &&
	   ( ent->s.angles[0] || ent->s.angles[1] || ent->s.angles[2] ) )
	{
		// expand for rotation
		float max, v;
		int i;

		max = 0;
		for ( i = 0; i < 3; i++ )
		{
			v = fabs( ent->mins[i] );
			if ( v > max ) {
				max = v;
			}
			v = fabs( ent->maxs[i] );
			if ( v > max ) {
				max = v;
			}
		}
		for ( i = 0; i < 3; i++ )
		{
			ent->absmin[i] = ent->s.origin[i] - max;
			ent->absmax[i] = ent->s.origin[i] + max;
		}
	}
	else
	{
		// normal
		VectorAdd( ent->s.origin, ent->mins, ent->absmin );
		VectorAdd( ent->s.origin, ent->maxs, ent->absmax );
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	ent->absmin[0] -= 1;
	ent->absmin[1] -= 1;
	ent->absmin[2] -= 1;
	ent->absmax[0] += 1;
	ent->absmax[1] += 1;
	ent->absmax[2] += 1;

	// link to PVS leafs
	ent->num_clusters = 0;
	ent->areanum = 0;
	ent->areanum2 = 0;

	//get all leafs, including solids
	num_leafs = CM_BoxLeafnums( ent->absmin, ent->absmax, leafs, MAX_TOTAL_ENT_LEAFS, &topnode );

	// set areas
	for ( i = 0; i < num_leafs; i++ )
	{
		clusters[i] = CM_LeafCluster( leafs[i] );
		area = CM_LeafArea( leafs[i] );
		if ( area )
		{
			// doors may legally straggle two areas,
			// but nothing should ever need more than that
			if ( ent->areanum && ent->areanum != area )
			{
				if ( ent->areanum2 && ent->areanum2 != area && sv.state == ss_loading )
				{
					Com_DPrintf( "Object touching 3 areas at %f %f %f\n",
						ent->absmin[0], ent->absmin[1], ent->absmin[2] );
				}
				ent->areanum2 = area;
			}
			else
			{
				ent->areanum = area;
			}
		}
	}

	if ( num_leafs >= MAX_TOTAL_ENT_LEAFS )
	{
		// assume we missed some leafs, and mark by headnode
		ent->num_clusters = -1;
		ent->headnode = topnode;
	}
	else
	{
		ent->num_clusters = 0;
		for (i=0 ; i<num_leafs ; i++)
		{
			if ( clusters[i] == -1 ) {
				// not a visible leaf
				continue;
			}
			for ( j = 0; j < i; j++ )
			{
				if ( clusters[j] == clusters[i] ) {
					break;
				}
			}
			if (j == i)
			{
				if (ent->num_clusters == MAX_ENT_CLUSTERS) {
					// assume we missed some leafs, and mark by headnode
					ent->num_clusters = -1;
					ent->headnode = topnode;
					break;
				}

				ent->clusternums[ent->num_clusters++] = clusters[i];
			}
		}
	}

	// if first time, make sure old_origin is valid
	if ( !ent->linkcount ) {
		VectorCopy( ent->s.origin, ent->s.old_origin );
	}
	ent->linkcount++;

	if ( ent->solid == SOLID_NOT ) {
		return;
	}

	// find the first node that the ent's box crosses
	node = sv_worldSectors;
	while ( 1 )
	{
		if ( node->axis == -1 ) {
			break;
		}
		if ( ent->absmin[node->axis] > node->dist )
		{
			node = node->children[0];
		}
		else if ( ent->absmax[node->axis] < node->dist )
		{
			node = node->children[1];
		}
		else
		{
			// crosses the node
			break;
		}
	}
	
	// link it in
	if ( ent->solid == SOLID_TRIGGER ) {
		InsertLinkBefore( &ent->area, &node->trigger_edicts );
	} else {
		InsertLinkBefore( &ent->area, &node->solid_edicts );
	}
}

/*
===================================================================================================

	Area Query

	Fills in a list of all entities who's absmin / absmax intersects the given
	bounds.  This does NOT mean that they actually touch in the case of bmodels.

===================================================================================================
*/

struct areaParams_t
{
	float *mins, *maxs;
	edict_t **list;
	int count, maxCount;
	int type;
};

/*
========================
SV_AreaEntities_r
========================
*/
static void SV_AreaEntities_r( worldSector_t *node, areaParams_t &ap )
{
	link_t		*l, *next, *start;
	edict_t		*check;
	int			count;

	count = 0;

	// touch linked edicts
	if ( ap.type == AREA_SOLID ) {
		start = &node->solid_edicts;
	} else {
		start = &node->trigger_edicts;
	}

	for ( l = start->next; l != start; l = next )
	{
		next = l->next;
		check = EDICT_FROM_AREA( l );

		if ( check->solid == SOLID_NOT ) {
			// deactivated
			continue;
		}

		if ( check->absmin[0] > ap.maxs[0] ||
			 check->absmin[1] > ap.maxs[1] ||
			 check->absmin[2] > ap.maxs[2] ||
			 check->absmax[0] < ap.mins[0] ||
			 check->absmax[1] < ap.mins[1] ||
			 check->absmax[2] < ap.mins[2] ) {
			// not touching
			continue;
		}

		if ( ap.count == ap.maxCount )
		{
			Com_Print( "SV_AreaEntities: MAXCOUNT\n" );
			return;
		}

		ap.list[ap.count] = check;
		++ap.count;
	}
	
	if ( node->axis == -1 ) {
		// terminal node
		return;
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
========================
SV_AreaEntities
========================
*/
int SV_AreaEntities( vec3_t mins, vec3_t maxs, edict_t **list, int maxcount, int areatype )
{
	areaParams_t ap;

	ap.mins = mins;
	ap.maxs = maxs;
	ap.list = list;
	ap.count = 0;
	ap.maxCount = maxcount;
	ap.type = areatype;

	SV_AreaEntities_r( sv_worldSectors, ap );

	return ap.count;
}

//=================================================================================================

struct moveclip_t
{
	vec3_t		boxmins, boxmaxs;	// enclose the test object along entire move
	float		*mins, *maxs;		// size of the moving object
	vec3_t		mins2, maxs2;		// size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	edict_t		*passedict;
	int			contentmask;
};

/*
========================
SV_HullForEntity

Returns a headnode that can be used for testing or clipping an
object of mins/maxs size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
========================
*/
static int SV_HullForEntity( edict_t *ent )
{
	// decide which clipping hull to use, based on the size
	if ( ent->solid == SOLID_BSP )
	{
		// explicit hulls in the BSP model
		cmodel_t *model = sv.models[ent->s.modelindex];

		if ( !model ) {
			Com_FatalError( "MOVETYPE_PUSH with a non bsp model\n" );
		}

		return model->headnode;
	}

	// create a temp hull from bounding box sizes

	return CM_HeadnodeForBox( ent->mins, ent->maxs );
}

/*
========================
SV_PointContents
========================
*/
int SV_PointContents( vec3_t p )
{
	edict_t *touch[MAX_EDICTS];

	// get base contents from world
	int contents = CM_PointContents( p, sv.models[1]->headnode );

	// or in contents from all the other entities
	const int numEntities = SV_AreaEntities( p, p, touch, MAX_EDICTS, AREA_SOLID );

	for ( int i = 0; i < numEntities; ++i )
	{
		edict_t *hit = touch[i];

		if ( hit->solid == SOLID_PHYSICS )
		{
			// SlartTodo
		}
		else
		{
			// might intersect, so do an exact clip
			int headnode = SV_HullForEntity( hit );
			float *angles = hit->s.angles;
			if ( hit->solid != SOLID_BSP ) {
				// boxes don't rotate
				angles = vec3_origin;
			}

			contents |= CM_TransformedPointContents( p, headnode, hit->s.origin, angles );
		}
	}

	return contents;
}

//=================================================================================================

/*
========================
SV_ClipMoveToEntities
========================
*/
static void SV_ClipMoveToEntities( moveclip_t &clip )
{
	edict_t *touchlist[MAX_EDICTS];
	trace_t trace;

	const int numEntities = SV_AreaEntities( clip.boxmins, clip.boxmaxs, touchlist, MAX_EDICTS, AREA_SOLID );

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for ( int i = 0; i < numEntities; ++i )
	{
		edict_t *touch = touchlist[i];
		if ( touch->solid == SOLID_NOT ) {
			continue;
		}
		if ( touch == clip.passedict ) {
			continue;
		}
		if ( clip.trace.allsolid ) {
			return;
		}
		if ( clip.passedict )
		{
			if ( touch->owner == clip.passedict ) {
				// don't clip against own missiles
				continue;
			}
			if ( clip.passedict->owner == touch ) {
				// don't clip against owner
				continue;
			}
		}

		if ( !( clip.contentmask & CONTENTS_DEADMONSTER ) &&
			  ( touch->svflags & SVF_DEADMONSTER ) ) {
			continue;
		}

		if ( touch->solid == SOLID_PHYSICS )
		{
			rayCast_t rayCast( clip.start, clip.end, clip.mins, clip.maxs );
			Physics::GetPhysicsSystem()->Trace( rayCast, touch->pPhysBody->GetShape(), touch->s.origin, touch->s.angles, trace );
		}
		else
		{
			// might intersect, so do an exact clip
			int headnode = SV_HullForEntity( touch );
			float *angles = touch->s.angles;
			if ( touch->solid != SOLID_BSP ) {
				// boxes don't rotate
				angles = vec3_origin;
			}

			if ( touch->svflags & SVF_MONSTER )
			{
				CM_TransformedBoxTrace( clip.start, clip.end,
					clip.mins2, clip.maxs2, headnode, clip.contentmask,
					touch->s.origin, angles, trace );
			}
			else
			{
				CM_TransformedBoxTrace( clip.start, clip.end,
					clip.mins, clip.maxs, headnode, clip.contentmask,
					touch->s.origin, angles, trace );
			}
		}

		if ( trace.allsolid || trace.startsolid || trace.fraction < clip.trace.fraction )
		{
			trace.ent = touch;
			if ( clip.trace.startsolid )
			{
				clip.trace = trace;
				clip.trace.startsolid = true;
			}
			else
			{
				clip.trace = trace;
			}
		}
		else if ( trace.startsolid )
		{
			clip.trace.startsolid = true;
		}
	}
}

/*
========================
SV_TraceBounds
========================
*/
static void SV_TraceBounds( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, vec3_t boxmins, vec3_t boxmaxs )
{
#if 0
	// debug to test against everything
	boxmins[0] = boxmins[1] = boxmins[2] = -9999;
	boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
#else	
	for ( int i = 0; i < 3; i++ )
	{
		if ( end[i] > start[i] )
		{
			boxmins[i] = start[i] + mins[i] - 1.0f;
			boxmaxs[i] = end[i] + maxs[i] + 1.0f;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1.0f;
			boxmaxs[i] = start[i] + maxs[i] + 1.0f;
		}
	}
#endif
}

/*
========================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.

Passedict and edicts owned by passedict are explicitly not checked.
========================
*/
trace_t SV_Trace( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask )
{
	moveclip_t clip;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	memset( &clip, 0, sizeof( moveclip_t ) );

	// clip to world
	clip.trace = CM_BoxTrace( start, end, mins, maxs, 0, contentmask );
	clip.trace.ent = ge->edicts;
	if ( clip.trace.fraction == 0.0f ) {
		// blocked by the world
		return clip.trace;
	}

	clip.contentmask = contentmask;
	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passedict = passedict;

	VectorCopy( mins, clip.mins2 );
	VectorCopy( maxs, clip.maxs2 );

	// create the bounding box of the entire move
	SV_TraceBounds( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

	// clip to other solid entities
	SV_ClipMoveToEntities( clip );

	return clip.trace;
}
