/*
===================================================================================================

	Contains defs shared between cgame, game and the engine

===================================================================================================
*/

#pragma once

#include "../core/core.h"

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString

//
// per-level limits
//
#define	MAX_CLIENTS			256		// absolute limit
#define	MAX_EDICTS			1024	// must change protocol to increase more
#define	MAX_LIGHTSTYLES		256
#define	MAX_MODELS			256		// these are sent over the net as bytes
#define	MAX_SOUNDS			256		// so they cannot be blindly increased
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings

// game print flags
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages

// destination class for gi.multicast()
enum multicast_t
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
};

//-------------------------------------------------------------------------------------------------
// key / value info strings - q_shared.cpp
//-------------------------------------------------------------------------------------------------

#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

// Searches the string for the given
// key and returns the associated value, or an empty string.
char *	Info_ValueForKey (const char *s, const char *key);

void	Info_RemoveKey (char *s, const char *key);
void	Info_SetValueForKey (char *s, const char *key, const char *value);

// Some characters are illegal in info strings because they
// can mess up the server's parsing
bool	Info_Validate (const char *s);

//-------------------------------------------------------------------------------------------------
// CVARS - cvars.cpp
//-------------------------------------------------------------------------------------------------

#define	CVAR_ARCHIVE		1	// will be written to config.cfg
#define	CVAR_USERINFO		2	// added to userinfo when changed
#define	CVAR_SERVERINFO		4	// added to serverinfo when changed
#define	CVAR_NOSET			8	// don't allow change from console at all, but can be set via command line
#define	CVAR_LATCH			16	// save changes until server restart
#define CVAR_CHEAT			32	// var cannot be changed unless sv_cheats is true (or singleplayer)
#define CVAR_MODIFIED		64	// set each time the cvar is changed

// nothing outside the Cvar_*() functions should modify these fields!
struct cvar_t
{
	lab::string		name;
	lab::string		value;
	lab::string		latchedValue;
	uint32			flags;
	float			fltValue;
	int				intValue;
	cvar_t *		pNext;

	const char *	GetName() const		{ return name.c_str(); }

	bool			IsModified() const	{ return ( flags & CVAR_MODIFIED ) != 0; }
	void			SetModified()		{ flags |= CVAR_MODIFIED; }
	void			ClearModified()		{ flags &= ~CVAR_MODIFIED; }

	const char *	GetString() const	{ return value.c_str(); }
	float			GetFloat() const	{ return fltValue; }
	int				GetInt() const		{ return intValue; }
	bool			GetBool() const		{ return intValue != 0; }

	uint32			GetFlags() const	{ return flags; }
};

//-------------------------------------------------------------------------------------------------
// Collision detection
//-------------------------------------------------------------------------------------------------

#include "bspflags.h"


// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2


struct cmodel_t
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	int			headnode;
};

struct csurface_t
{
	char		name[MAX_QPATH];
	int			flags;
};

struct edict_t;

// a trace is returned when a box is swept through the world
// this packed version of trace_t is 64 bytes, while the old one was 72
struct trace_t
{
	cplane_t	plane;		// surface normal at impact
	vec3_t		endpos;		// final position
	edict_t		*ent;		// not set by CM_*() functions
	csurface_t	*surface;	// surface hit
	float		fraction;	// time completed, 1.0 = didn't hit anything
	int			contents;	// contents on other side of surface hit
	bool		allsolid;	// if true, plane is not valid
	bool		startsolid;	// if true, the initial point was in a solid area
};

//-------------------------------------------------------------------------------------------------
// Player movement
//-------------------------------------------------------------------------------------------------

// pmove_state_t is the information necessary for client side movement
// prediction
enum pmType_t
{
	// Can accelerate and turn:

	PM_NORMAL,
	PM_SPECTATOR,	// Flying without gravity but with collision detection
	PM_NOCLIP,		// Flying without collision detection nor gravity

	// No acceleration or turning:

	PM_DEAD,		// No acceleration or turning, but free falling
	PM_GIB,			// Same as dead but with a different bounding box (SlartTodo: REMOVE THIS)
	PM_FREEZE		// Stuck in place with no control
};

// Identifies how submerged in water a player is
enum waterLevel_t
{
	WL_NONE,		// Not in water
	WL_FEET,		// Feet underwater
	WL_WAIST,		// Waist underwater
	WL_EYES			// Eyes underwater
};

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	PMF_TIME_TELEPORT	16	// pm_time is non-moving time
#define PMF_NO_PREDICTION	32	// temporarily disables prediction (used for grappling hook)

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_TELEPORT)

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
struct pmove_state_t
{
	pmType_t	pm_type;

	vec3_t		origin;
	vec3_t		velocity;
	byte		pm_flags;		// ducked, jump_held, etc
	byte		pm_time;		// each unit = 8 ms
	short		gravity;
	vec3_t		delta_angles;		// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int			time_step_sound;	// In milliseconds, the time until we can play another footstep
	int			step_left;			// > 0 if the next footstep is the left foot

	int			swim_time;
};

//
// button bits
//
#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define	BUTTON_ANY			128			// any key whatsoever

// usercmd_t is sent to the server each client frame
struct usercmd_t
{
	vec3_t	angles;
	float	forwardmove, sidemove, upmove;
	byte	buttons;
	byte	msec;
	byte	lightlevel;		// light level the player is standing on
};

#define	MAXTOUCH	32
struct pmove_t
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t	cmd;

	// results (out)
	int			numtouch;
	edict_t		*touchents[MAXTOUCH];

	vec3_t		viewangles;			// clamped
	float		viewheight;

	vec3_t		mins, maxs;			// bounding box size

	edict_t		*groundentity;
	int			watertype;
	int			waterlevel;

	// callbacks to test the world
	// these will be different functions during game and cgame
	trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int			(*pointcontents) (vec3_t point);
	void		(*playsound) (const char *sample, float volume);
};

//-------------------------------------------------------------------------------------------------
// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
//-------------------------------------------------------------------------------------------------

#define	EF_ROTATE			0x00000001		// rotate (bonus items)
#define	EF_GIB				0x00000002		// leave a trail
#define	EF_BLASTER			0x00000008		// redlight + trail
#define	EF_ROCKET			0x00000010		// redlight + trail
#define	EF_GRENADE			0x00000020
#define	EF_HYPERBLASTER		0x00000040
#define	EF_BFG				0x00000080
#define EF_COLOR_SHELL		0x00000100
#define EF_POWERSCREEN		0x00000200
#define	EF_ANIM01			0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define	EF_ANIM23			0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define	EF_FLIES			0x00004000
#define	EF_QUAD				0x00008000
#define	EF_PENT				0x00010000
#define	EF_TELEPORTER		0x00020000		// particle fountain
#define EF_FLAG1			0x00040000
#define EF_FLAG2			0x00080000
// RAFAEL
#define EF_IONRIPPER		0x00100000
#define EF_GREENGIB			0x00200000
#define	EF_BLUEHYPERBLASTER 0x00400000
#define EF_SPINNINGLIGHTS	0x00800000
#define EF_PLASMA			0x01000000
#define EF_TRAP				0x02000000

//ROGUE
#define EF_TRACKER			0x04000000
#define	EF_DOUBLE			0x08000000
#define	EF_SPHERETRANS		0x10000000
#define EF_TAGTRAIL			0x20000000
#define EF_HALF_DAMAGE		0x40000000
#define EF_TRACKERTRAIL		0x80000000
//ROGUE

// entity_state_t->renderfx flags
#define	RF_MINLIGHT			1		// allways have some light (viewmodel)
#define	RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define	RF_WEAPONMODEL		4		// only draw through eyes
#define	RF_FULLBRIGHT		8		// allways draw full intensity
#define	RF_DEPTHHACK		16		// for view weapon Z crunching
#define	RF_TRANSLUCENT		32
#define	RF_FRAMELERP		64
#define RF_BEAM				128
#define	RF_CUSTOMSKIN		256		// skin is an index in image_precache
#define	RF_GLOW				512		// pulse lighting for bonus items
#define RF_SHELL_RED		1024
#define	RF_SHELL_GREEN		2048
#define RF_SHELL_BLUE		4096

//ROGUE
#define RF_IR_VISIBLE		0x00008000		// 32768
#define	RF_SHELL_DOUBLE		0x00010000		// 65536
#define	RF_SHELL_HALF_DAM	0x00020000
#define RF_USE_DISGUISE		0x00040000
//ROGUE

// player_state_t->refdef flags
#define	RDF_UNDERWATER		1		// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2		// used for player configuration screen

//ROGUE
#define	RDF_IRGOGGLES		4
#define RDF_UVGOGGLES		8
//ROGUE

//-------------------------------------------------------------------------------------------------
// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
//-------------------------------------------------------------------------------------------------

#define	CHAN_AUTO               0
#define	CHAN_WEAPON             1
#define	CHAN_VOICE              2
#define	CHAN_ITEM               3
#define	CHAN_BODY               4
// modifier flags
#define	CHAN_NO_PHS_ADD			8	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define	CHAN_RELIABLE			16	// send by reliable message, not datagram

// sound attenuation values
#define	ATTN_NONE               0	// full volume the entire level
#define	ATTN_NORM               1
#define	ATTN_IDLE               2
#define	ATTN_STATIC             3	// diminish very rapidly with distance

//-------------------------------------------------------------------------------------------------
// player_state->stats[] indexes
//-------------------------------------------------------------------------------------------------

#define STAT_HEALTH_ICON		0
#define	STAT_HEALTH				1
#define	STAT_AMMO_ICON			2
#define	STAT_AMMO				3
#define	STAT_ARMOR_ICON			4
#define	STAT_ARMOR				5
#define	STAT_SELECTED_ICON		6
#define	STAT_PICKUP_ICON		7
#define	STAT_PICKUP_STRING		8
#define	STAT_TIMER_ICON			9
#define	STAT_TIMER				10
#define	STAT_HELPICON			11
#define	STAT_SELECTED_ITEM		12
#define	STAT_LAYOUTS			13
#define	STAT_FRAGS				14
#define	STAT_FLASHES			15		// cleared each frame, 1 = health, 2 = armor
#define STAT_CHASE				16
#define STAT_SPECTATOR			17

#define	MAX_STATS				32

//-------------------------------------------------------------------------------------------------
// dmflags->value flags
//-------------------------------------------------------------------------------------------------

#define	DF_NO_HEALTH		0x00000001	// 1
#define	DF_NO_ITEMS			0x00000002	// 2
#define	DF_WEAPONS_STAY		0x00000004	// 4
#define	DF_NO_FALLING		0x00000008	// 8
#define	DF_INSTANT_ITEMS	0x00000010	// 16
#define	DF_SAME_LEVEL		0x00000020	// 32
#define DF_SKINTEAMS		0x00000040	// 64
#define DF_MODELTEAMS		0x00000080	// 128
#define DF_NO_FRIENDLY_FIRE	0x00000100	// 256
#define	DF_SPAWN_FARTHEST	0x00000200	// 512
#define DF_FORCE_RESPAWN	0x00000400	// 1024
#define DF_NO_ARMOR			0x00000800	// 2048
#define DF_ALLOW_EXIT		0x00001000	// 4096
#define DF_INFINITE_AMMO	0x00002000	// 8192
#define DF_QUAD_DROP		0x00004000	// 16384
#define DF_FIXED_FOV		0x00008000	// 32768

// RAFAEL
#define	DF_QUADFIRE_DROP	0x00010000	// 65536

//ROGUE
#define DF_NO_MINES			0x00020000
#define DF_NO_STACK_DOUBLE	0x00040000
#define DF_NO_NUKES			0x00080000
#define DF_NO_SPHERES		0x00100000
//ROGUE

//-------------------------------------------------------------------------------------------------
// ELEMENTS COMMUNICATED ACROSS THE NET
//-------------------------------------------------------------------------------------------------

#define	ANGLE2SHORT(x)	x
#define	SHORT2ANGLE(x)	x

//-------------------------------------------------------------------------------------------------
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//-------------------------------------------------------------------------------------------------

#define	CS_NAME				0
#define	CS_CDTRACK			1
#define	CS_SKY				2
#define	CS_SKYAXIS			3		// %f %f %f format
#define	CS_SKYROTATE		4
#define	CS_STATUSBAR		5		// display program string

//#define CS_AIRACCEL		29		// air acceleration control
#define	CS_MAXCLIENTS		30
#define	CS_MAPCHECKSUM		31		// for catching cheater maps

#define	CS_MODELS			32
#define	CS_SOUNDS			(CS_MODELS+MAX_MODELS)
#define	CS_IMAGES			(CS_SOUNDS+MAX_SOUNDS)
#define	CS_LIGHTS			(CS_IMAGES+MAX_IMAGES)
#define	CS_ITEMS			(CS_LIGHTS+MAX_LIGHTSTYLES)
#define	CS_PLAYERSKINS		(CS_ITEMS+MAX_ITEMS)
#define CS_GENERAL			(CS_PLAYERSKINS+MAX_CLIENTS)

#define	MAX_CONFIGSTRINGS	(CS_GENERAL+MAX_GENERAL)

//-------------------------------------------------------------------------------------------------
// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
//-------------------------------------------------------------------------------------------------

enum entity_event_t
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
};

//-------------------------------------------------------------------------------------------------
// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
//-------------------------------------------------------------------------------------------------

struct entity_state_t
{
	int		number;			// edict index

	vec3_t	origin;
	vec3_t	angles;
	vec3_t	old_origin;		// for lerping
	int		modelindex;
	int		modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc
	int		frame;
	int		skinnum;
	uint	effects;		// PGM - we're filling it, so it needs to be unsigned
	int		renderfx;
	int		solid;			// for client side prediction, 8*(bits 0-4) is x/y radius
							// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
							// gi.linkentity sets this properly
	int		sound;			// for looping sounds, to guarantee shutoff
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
							// events only go out for a single frame, they
							// are automatically cleared each frame
};

//-------------------------------------------------------------------------------------------------
// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
//-------------------------------------------------------------------------------------------------

struct player_state_t
{
	pmove_state_t	pmove;		// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t		viewangles;		// for fixed views
	vec3_t		viewoffset;		// add to pmovestate->origin
	vec3_t		kick_angles;	// add to view direction to get render angles
								// set by weapon kicks, pain effects, etc

	int			gunindex;
	int			gunframe;

	float		blend[4];		// rgba full screen effect
	
	float		fov;			// horizontal field of view

	int			rdflags;		// refdef flags

	short		stats[MAX_STATS];		// fast status bar updates
};
