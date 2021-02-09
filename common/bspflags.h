//
// BSP flags
//

#pragma once

// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

enum contentFlags_t
{
	// lower bits are stronger, and will eat weaker brushes completely
	CONTENTS_SOLID			= 0x1,			// an eye is never valid in a solid
	CONTENTS_WINDOW			= 0x2,			// translucent, but not watery
	CONTENTS_AUX			= 0x4,
	CONTENTS_LAVA			= 0x8,
	CONTENTS_SLIME			= 0x10,
	CONTENTS_WATER			= 0x20,
	CONTENTS_MIST			= 0x40,
	LAST_VISIBLE_CONTENTS	= CONTENTS_MIST,

// remaining contents are non-visible, and don't eat brushes

	CONTENTS_AREAPORTAL		= 0x8000,

	CONTENTS_PLAYERCLIP		= 0x10000,
	CONTENTS_MONSTERCLIP	= 0x20000,

// currents can be added to any other contents, and may be mixed
	CONTENTS_CURRENT_0		= 0x40000,
	CONTENTS_CURRENT_90		= 0x80000,
	CONTENTS_CURRENT_180	= 0x100000,
	CONTENTS_CURRENT_270	= 0x200000,
	CONTENTS_CURRENT_UP		= 0x400000,
	CONTENTS_CURRENT_DOWN	= 0x800000,

	CONTENTS_ORIGIN			= 0x1000000,	// removed before bsping an entity

	CONTENTS_MONSTER		= 0x2000000,	// should never be on a brush, only in game
	CONTENTS_DEADMONSTER	= 0x4000000,
	CONTENTS_DETAIL			= 0x8000000,	// brushes to be added after vis leafs
	CONTENTS_TRANSLUCENT	= 0x10000000,	// auto set if any surface has trans
	CONTENTS_LADDER			= 0x20000000
};

// Surface types

constexpr int NUM_SURFACE_BITS = 4;
constexpr int MAX_SURFACE_TYPES = 1 << NUM_SURFACE_BITS;

enum surfaceTypes_t
{
	SURFTYPE_CONCRETE,		// default type
	SURFTYPE_METAL,			// HL2 style solid metal
	SURFTYPE_VENT,			// Half-Life clumpy vent
	SURFTYPE_DIRT,			// Muddy mucky dirt, like HL1
	SURFTYPE_GRASS,			// HL2 style grass
	SURFTYPE_SAND,			// Grainy sand
	SURFTYPE_ROCK,			// Cave rock
	SURFTYPE_WOOD,			// Wood plank floor, like HL2
	SURFTYPE_TILE,			// Slippery tile floor
	SURFTYPE_COMPUTER,		// Sharp hardware
	SURFTYPE_GLASS,			// Glass pane, decently thick
	SURFTYPE_FLESH,			// Meat! Yuck!
	SURFTYPE_12,
	SURFTYPE_13,
	SURFTYPE_14,
	SURFTYPE_15
};

// Surface flags

enum surfaceFlags_t
{
	SURF_TYPE_BIT0 = 0x1,	// First X bits are surface type parameters
	SURF_TYPE_BIT1 = 0x2,
	SURF_TYPE_BIT2 = 0x4,
	SURF_TYPE_BIT3 = 0x8,
	SURF_TYPE_MASK = ( 1 << NUM_SURFACE_BITS ) - 1,

	SURF_LIGHT		= 0x10,		// value will hold the light strength

	SURF_SLICK		= 0x20,		// affects game physics

	SURF_SKY		= 0x40,		// don't draw, but add to skybox
	SURF_WARP		= 0x80,		// turbulent water warp
	SURF_TRANS33	= 0x100,
	SURF_TRANS66	= 0x200,
	SURF_FLOWING	= 0x400,	// scroll towards angle
	SURF_NODRAW		= 0x800,	// don't bother referencing the texture

	// From utils qfiles.h
	SURF_HINT		= 0x1000,	// make a primary bsp splitter
	SURF_SKIP		= 0x2000,	// completely ignore, allowing non-closed brushes
};

inline surfaceTypes_t SurfaceTypeFromFlags( int flags )
{
	return (surfaceTypes_t)( flags & SURF_TYPE_MASK );
}

// Content masks

#define MASK_ALL				(-1)
#define MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// Surface masks

#define SURFMASK_UNLIT			(SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)
