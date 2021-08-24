
#pragma once

#include "../shared/engine.h"
#include "../../game/server/g_public.h"

#define	MAX_ENT_CLUSTERS	16

struct worldSector_t;

struct svEntity_t
{
	worldSector_t *	worldSector;
	svEntity_t *	nextEntityInWorldSector;

	entityState_t	baseline;			// for delta compression of initial sighting
	int				numClusters;		// if -1, use headnode instead
	int				clusternums[MAX_ENT_CLUSTERS];
	int				lastCluster;		// if all the clusters don't fit in clusternums
	int				areanum, areanum2;
	int				snapshotCounter;	// used to prevent double adding from portal views
	int				sharedEntityIndex;	// index for ge->GetEdict
};

//=================================================================================================

#define	MAX_MASTERS	8				// max recipients for heartbeat packets

enum serverState_t
{
	ss_dead,			// no map loaded
	ss_loading,			// spawning level edicts
	ss_game,			// actively running
	ss_cinematic,
	ss_demo,
	ss_pic
};

struct server_t
{
	serverState_t	state;			// precache commands are only valid during load

	qboolean	attractloop;		// running cinematics and demos for the local system only
	qboolean	loadgame;			// client begins should reuse existing entity

	unsigned	time;				// always sv.framenum * 100 msec
	int			framenum;

	char		name[MAX_QPATH];			// map name, or cinematic name
	cmodel_t	*models[MAX_MODELS];

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];
	svEntity_t	svEntities[MAX_EDICTS];

	// the multicast buffer is used to send a message to a set of clients
	// it is only used to marshall data until SV_Multicast is called
	sizebuf_t	multicast;
	byte		multicast_buf[MAX_MSGLEN];

	// demo server information
	fsHandle_t	demofile;
	qboolean	timedemo;		// don't time sync
};

#define EDICT_NUM(n)		ge->GetEdict(n)
#define NUM_FOR_EDICT(e)	(int)(((intptr_t)e) - ((intptr_t)ge->GetEdict(0)))


enum clientState_t {
	cs_free,		// can be reused for a new connection
	cs_zombie,		// client has been disconnected, but don't reuse
					// connection for a couple seconds
	cs_connected,	// has been assigned to a client_t, but not in game yet
	cs_spawned		// client is fully in game
};

#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility

struct clientSnapshot_t
{
	int				areabytes;
	byte			areabits[MAX_MAP_AREA_BYTES];		// portalarea visibility bits
	playerState_t	ps;
	int				num_entities;
	int				first_entity;		// into the circular sv_packet_entities[]
										// the entities MUST be in increasing state number
										// order, otherwise the delta compression will fail
	int				senttime;			// LEGACY Q2
	int				messageSent;		// time the message was transmitted
	int				messageAcked;		// time the message was acked
	int				messageSize;		// used to rate drop packets
};

#define	LATENCY_COUNTS	16
#define	RATE_MESSAGES	10

struct client_t
{
	clientState_t	state;

	char			userinfo[MAX_INFO_STRING];		// name, etc

	int				lastframe;			// for delta compression
	usercmd_t		lastcmd;			// for filling in big drops

	int				commandMsec;		// every seconds this is reset, if user
										// commands exhaust it, assume time cheating

	int				frame_latency[LATENCY_COUNTS];
	int				ping;

	int				message_size[RATE_MESSAGES];	// used to rate drop packets
	int				rate;
	int				surpressCount;		// number of messages rate supressed

	edict_t			*edict;				// EDICT_NUM(clientnum+1)
	char			name[32];			// extracted from userinfo, high bits masked
	int				messagelevel;		// for filtering printed messages

	// The datagram is written to by sound calls, prints, temp ents, etc.
	// It can be harmlessly overflowed.
	sizebuf_t		datagram;
	byte			datagram_buf[MAX_MSGLEN];

	clientSnapshot_t	frames[UPDATE_BACKUP];	// updates can be delta'd from here

	byte			*download;			// file being downloaded
	int				downloadsize;		// total bytes (can't use EOF because of paks)
	int				downloadcount;		// bytes sent

	int				lastmessage;		// sv.framenum when packet was last received
	int				lastconnect;

	int				challenge;			// challenge of this user, randomly generated

	netchan_t		netchan;
};

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=================================================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

struct challenge_t
{
	netadr_t	adr;
	int			challenge;
	int			time;
};

struct serverStatic_t
{
	qboolean	initialized;				// sv_init has completed
	int			realtime;					// always increasing, no clamping, etc

	char		mapcmd[MAX_TOKEN_CHARS];	// ie: *intro.cin+base 

	int			spawncount;					// incremented each server start
											// used to check late spawns

	client_t	*clients;					// [maxclients->value];
	int			num_client_entities;		// maxclients->value*UPDATE_BACKUP*MAX_PACKET_ENTITIES
	int			next_client_entities;		// next client_entity to use
	entity_state_t	*client_entities;		// [num_client_entities]

	int			last_heartbeat;

	challenge_t	challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	// serverrecord values
	fsHandle_t	demofile;
	sizebuf_t	demo_multicast;
	byte		demo_multicast_buf[MAX_MSGLEN];
};

//=================================================================================================

// net_chan
extern netadr_t		net_from;
extern sizebuf_t	net_message;

extern netadr_t		master_adr[MAX_MASTERS];	// address of the master server

extern serverStatic_t	svs;				// persistant server info
extern server_t			sv;					// local server

extern cvar_t *		sv_paused;
extern cvar_t *		maxclients;
extern cvar_t *		sv_noreload;			// don't reload level state when reentering
											// development tool
extern cvar_t *		sv_enforcetime;

extern client_t *	sv_client;
extern edict_t *	sv_player;

//=================================================================================================

//
// sv_main.c
//
void SV_FinalMessage( const char *message, bool reconnect );
void SV_DropClient( client_t *drop );

int SV_ModelIndex( const char *name );
int SV_SoundIndex( const char *name );
int SV_ImageIndex( const char *name );

void SV_ExecuteUserCommand( char *s );

void SV_UserinfoChanged( client_t *cl );

void Master_Heartbeat();

//
// sv_init.c
//
void SV_InitGame();
void SV_Map( bool attractloop, const char *levelstring, bool loadgame );


//
// sv_phys.c
//
void SV_PrepWorldFrame (void);

//
// sv_send.c
//
enum redirect_t { RD_NONE, RD_CLIENT, RD_PACKET };
#define	SV_OUTPUTBUF_LENGTH	(MAX_MSGLEN - 16)

extern char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect (int sv_redirected, char *outputbuf);

void SV_SendClientMessages();

void SV_Multicast( vec3_t origin, multicast_t to );
void SV_StartSound( vec3_t origin, edict_t *entity, int channel,
					int soundindex, float volume,
					float attenuation, float timeofs );
void SV_ClientPrintf( client_t *cl, int level, _Printf_format_string_ const char *fmt, ... );
void SV_BroadcastPrintf( int level, _Printf_format_string_ const char *fmt, ... );
void SV_BroadcastCommand( const char *cmd );

//
// sv_user.c
//
void SV_Nextserver();
void SV_ExecuteClientMessage( client_t *cl );

//
// sv_ccmds.c
//
void SV_ReadLevelFile();
void SV_InitOperatorCommands();

//
// sv_ents.c
//
void SV_WriteFrameToClient (client_t *client, sizebuf_t *msg);
void SV_RecordDemoMessage (void);
void SV_BuildClientFrame (client_t *client);

//
// sv_game.c
//
extern game_export_t *ge;

void SV_InitGameProgs();
void SV_ShutdownGameProgs();

// wrong place for this? put here because it's there in q3
playerState_t *SV_PlayerStateForNum( int num );
sharedEntity_t *SV_SharedEntityForNum( int num );
svEntity_t *SV_SvEntityForSharedEntity( sharedEntity_t *gEnt );
sharedEntity_t *SV_SharedEntityForSvEntity( svEntity_t *svEnt );

//============================================================
//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld( void );
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEntity( sharedEntity_t *ent );
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity( sharedEntity_t *ent );
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid


int SV_ClipHandleForEntity( const sharedEntity_t *ent );


void SV_SectorList_f( void );


int SV_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );
// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.


int SV_PointContents( const vec3_t p, int passEntityNum );
// returns the CONTENTS_* value from the world and all entities at the given point.


void SV_Trace( trace_t *results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum is explicitly excluded from clipping checks (normally ENTITYNUM_NONE)


void SV_ClipToEntity( trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule );
// clip to a specific entity
