//=================================================================================================
// Collision models!
//
// This is the engine's side of the BSP loader
//=================================================================================================

#pragma once

#include "../../common/q_shared.h" // cmodel_t, trace_t

cmodel_t	*CM_LoadMap( const char *name, bool clientload, unsigned *checksum );
cmodel_t	*CM_InlineModel( const char *name );	// *1, *2, etc

int			CM_NumClusters( void );
int			CM_NumInlineModels( void );
char		*CM_EntityString( void );

// creates a clipping hull for an arbitrary box
int			CM_HeadnodeForBox( vec3_t mins, vec3_t maxs );


// returns an ORed contents mask
int			CM_PointContents( vec3_t p, int headnode );
int			CM_TransformedPointContents( vec3_t p, int headnode, vec3_t origin, vec3_t angles );

trace_t		CM_BoxTrace( vec3_t start, vec3_t end,
					vec3_t mins, vec3_t maxs,
					int headnode, int brushmask );
trace_t		CM_TransformedBoxTrace( vec3_t start, vec3_t end,
								vec3_t mins, vec3_t maxs,
								int headnode, int brushmask,
								vec3_t origin, vec3_t angles );

byte		*CM_ClusterPVS( int cluster );
byte		*CM_ClusterPHS( int cluster );

int			CM_PointLeafnum( vec3_t p );

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int			CM_BoxLeafnums( vec3_t mins, vec3_t maxs, int *list,
						int listsize, int *topnode );

int			CM_LeafContents( int leafnum );
int			CM_LeafCluster( int leafnum );
int			CM_LeafArea( int leafnum );

void		CM_SetAreaPortalState( int portalnum, qboolean open );
qboolean	CM_AreasConnected( int area1, int area2 );

int			CM_WriteAreaBits( byte *buffer, int area );
bool		CM_HeadnodeVisible( int headnode, byte *visbits );

void		CM_WritePortalState( FILE *f );
void		CM_ReadPortalState( FILE *f );
