// The interface to the game DLL

#include "sv_local.h"

game_export_t *ge;

/*
========================
PF_Unicast

Sends the contents of the multicast buffer to a single client
========================
*/
static void PF_Unicast( edict_t *ent, qboolean reliable )
{
	int p;
	client_t *client;

	if ( !ent ) {
		return;
	}

	p = NUM_FOR_EDICT( ent );
	if ( p < 1 || p > maxclients->GetInt() ) {
		return;
	}

	client = svs.clients + ( p - 1 );

	if ( reliable ) {
		SZ_Write( &client->netchan.message, sv.multicast.data, sv.multicast.cursize );
	} else {
		SZ_Write( &client->datagram, sv.multicast.data, sv.multicast.cursize );
	}

	SZ_Clear( &sv.multicast );
}

/*
========================
PF_dprintf

Debug print to server console
========================
*/
static void PF_dprintf( const char *fmt, ... )
{
	char msg[MAX_PRINT_MSG];
	va_list argptr;

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );
}

/*
========================
PF_cprintf

Print to a single client
========================
*/
static void PF_cprintf( edict_t *ent, int level, const char *fmt, ... )
{
	char msg[MAX_PRINT_MSG];
	va_list argptr;
	int n;

	if ( ent ) {
		n = NUM_FOR_EDICT( ent );
		if ( n < 1 || n > maxclients->GetInt() ) {
			Com_Error( "cprintf to a non-client" );
		}
	}

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	if ( ent ) {
		SV_ClientPrintf( svs.clients + ( n - 1 ), level, "%s", msg );
	} else {
		Com_Print( msg );
	}
}

/*
========================
PF_centerprintf

centerprint to a single client
========================
*/
static void PF_centerprintf( edict_t *ent, const char *fmt, ... )
{
	char msg[MAX_PRINT_MSG];
	va_list argptr;
	int n;

	n = NUM_FOR_EDICT( ent );
	if ( n < 1 || n > maxclients->GetInt() ) {
		return;	// Com_Error ("centerprintf to a non-client");
	}

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	MSG_WriteByte( &sv.multicast, svc_centerprint );
	MSG_WriteString( &sv.multicast, msg );
	PF_Unicast( ent, true );
}

/*
========================
PF_error

Abort the server with a game error
========================
*/
static void PF_error( const char *fmt, ... )
{
	char msg[MAX_PRINT_MSG];
	va_list argptr;

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Errorf( "Game Error: %s", msg );
}

/*
========================
PF_setmodel

Also sets mins and maxs for inline bmodels
========================
*/
static void PF_setmodel( edict_t *ent, const char *name )
{
	int i;
	cmodel_t *mod;

	if ( !name ) {
		Com_Error( "PF_setmodel: NULL" );
	}

	i = SV_ModelIndex( name );

	//ent->model = name;
	ent->s.modelindex = i;

	// if it is an inline model, get the size information for it
	if ( name[0] == '*' )
	{
		mod = CM_InlineModel( name );
		VectorCopy( mod->mins, ent->mins );
		VectorCopy( mod->maxs, ent->maxs );
		SV_LinkEntity( ent );
	}
}

/*
========================
PF_Configstring
========================
*/
static void PF_Configstring( int index, const char *val )
{
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Errorf( "PF_Configstring: bad index %i\n", index );
	}

	if ( !val ) {
		val = "";
	}

	// change the string in sv
	strcpy( sv.configstrings[index], val );

	if ( sv.state != ss_loading )
	{
		// send the update to everyone
		SZ_Clear( &sv.multicast );
		MSG_WriteChar( &sv.multicast, svc_configstring );
		MSG_WriteShort( &sv.multicast, index );
		MSG_WriteString( &sv.multicast, val );

		SV_Multicast( vec3_origin, MULTICAST_ALL_R );
	}
}

static void PF_WriteChar( int c )			{ MSG_WriteChar( &sv.multicast, c ); }
static void PF_WriteByte( int c )			{ MSG_WriteByte( &sv.multicast, c ); }
static void PF_WriteShort( int c )			{ MSG_WriteShort( &sv.multicast, c ); }
static void PF_WriteLong( int c )			{ MSG_WriteLong( &sv.multicast, c ); }
static void PF_WriteFloat( float f )		{ MSG_WriteFloat( &sv.multicast, f ); }
static void PF_WriteString( const char *s )	{ MSG_WriteString( &sv.multicast, s ); }
static void PF_WritePos( vec3_t pos )		{ MSG_WritePos( &sv.multicast, pos ); }
static void PF_WriteDir( vec3_t dir )		{ MSG_WriteDir( &sv.multicast, dir ); }
static void PF_WriteAngle( float f )		{ MSG_WriteAngle( &sv.multicast, f ); }

/*
========================
PF_inPVS

Also checks portalareas so that doors block sight
========================
*/
qboolean PF_inPVS( vec3_t p1, vec3_t p2 )
{
	int leafnum;
	int cluster;
	int area1, area2;
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	area2 = CM_LeafArea( leafnum );

	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) ) {
		return false;
	}
	if ( !CM_AreasConnected( area1, area2 ) ) {
		// a door blocks sight
		return false;
	}

	return true;
}

/*
========================
PF_inPHS

Also checks portalareas so that doors block sound
========================
*/
static qboolean PF_inPHS( vec3_t p1, vec3_t p2 )
{
	int leafnum;
	int cluster;
	int area1, area2;
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPHS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	area2 = CM_LeafArea( leafnum );

	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) ) {
		// more than one bounce away
		return false;
	}
	if ( !CM_AreasConnected( area1, area2 ) ) {
		// a door blocks hearing
		return false;
	}

	return true;
}

/*
========================
PF_StartSound

Broadcasts a sound to clients
========================
*/
static void PF_StartSound( edict_t *entity, int channel, int sound_num, float volume, float attenuation, float timeofs )
{
	if ( !entity ) {
		return;
	}

	SV_StartSound( nullptr, entity, channel, sound_num, volume, attenuation, timeofs );
}

// need these due to zone api changes

static void *PF_TagMalloc( int size, int tag )
{
	return Mem_TagAlloc( size, tag );
}

static void PF_TagFreeGroup( int tag )
{
	Mem_TagFreeGroup( tag );
}

// need this due to cvar_get api change

static cvar_t *PF_Cvar_Get( const char *name, const char *value, uint32 flags )
{
	return Cvar_Get( name, value, flags );
}

//=================================================================================================

/*
========================
SV_InitGameProgs

Init the game subsystem for a new map
========================
*/
void SV_InitGameProgs()
{
	game_import_t gi;

	// unload anything we have now
	if ( ge ) {
		SV_ShutdownGameProgs();
	}

	// load a new game dll
	gi.multicast = SV_Multicast;
	gi.unicast = PF_Unicast;
	gi.bprintf = SV_BroadcastPrintf;
	gi.dprintf = PF_dprintf;
	gi.cprintf = PF_cprintf;
	gi.centerprintf = PF_centerprintf;
	gi.error = PF_error;

	gi.linkentity = SV_LinkEntity;
	gi.unlinkentity = SV_UnlinkEntity;
	gi.BoxEdicts = SV_AreaEntities;
	gi.trace = SV_Trace;
	gi.pointcontents = SV_PointContents;
	gi.setmodel = PF_setmodel;
	gi.inPVS = PF_inPVS;
	gi.inPHS = PF_inPHS;

	gi.modelindex = SV_ModelIndex;
	gi.soundindex = SV_SoundIndex;
	gi.imageindex = SV_ImageIndex;

	gi.configstring = PF_Configstring;
	gi.sound = PF_StartSound;
	gi.positioned_sound = SV_StartSound;

	gi.WriteChar = PF_WriteChar;
	gi.WriteByte = PF_WriteByte;
	gi.WriteShort = PF_WriteShort;
	gi.WriteLong = PF_WriteLong;
	gi.WriteFloat = PF_WriteFloat;
	gi.WriteString = PF_WriteString;
	gi.WritePosition = PF_WritePos;
	gi.WriteDir = PF_WriteDir;
	gi.WriteAngle = PF_WriteAngle;

	gi.TagMalloc = PF_TagMalloc;
	gi.TagFree = Mem_TagFree;
	gi.FreeTags = PF_TagFreeGroup;

	gi.cvar = PF_Cvar_Get;
	gi.cvar_set = Cvar_FindSetString;
	gi.cvar_forceset = Cvar_ForceSet;

	gi.argc = Cmd_Argc;
	gi.argv = Cmd_Argv;
	gi.args = Cmd_Args;
	gi.AddCommandString = Cbuf_AddText;

	gi.DebugGraph = SCR_DebugGraph;
	gi.SetAreaPortalState = CM_SetAreaPortalState;
	gi.AreasConnected = CM_AreasConnected;

	gi.fileSystem = &g_fileSystem;

	ge = (game_export_t *)Sys_GetGameAPI( &gi );

	if ( !ge ) {
		Com_Error( "failed to load game DLL" );
	}
	if ( ge->apiversion != GAME_API_VERSION ) {
		Com_Errorf( "game is version %i, not %i", ge->apiversion, GAME_API_VERSION );
	}

	ge->Init();
}

/*
========================
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
========================
*/
void SV_ShutdownGameProgs()
{
	if ( !ge ) {
		return;
	}

	ge->Shutdown();
	Sys_UnloadGame();
	ge = nullptr;
}
