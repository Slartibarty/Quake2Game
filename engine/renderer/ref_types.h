
#pragma once

#define CONCHAR_WIDTH		8
#define CONCHAR_HEIGHT		8

#define	MAX_DLIGHTS			32
#define	MAX_ENTITIES		1024
#define	MAX_PARTICLES		4096
#define	MAX_LIGHTSTYLES		256

struct model_t;
struct material_t;

struct dlight_t
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
};

struct entity_t
{
	model_t		*model;				// opaque type outside refresh
	vec3_t		angles;

	/*
	** most recent data
	*/
	vec3_t		origin;				// also used as RF_BEAM's "from"
	int			frame;				// also used as RF_BEAM's diameter

	/*
	** previous data for lerping
	*/
	vec3_t		oldorigin;			// also used as RF_BEAM's "to"
	int			oldframe;

	/*
	** misc
	*/
	float		backlerp;			// 0.0 = current, 1.0 = old
	int			skinnum;			// also used as RF_BEAM's palette index

	int			lightstyle;			// for flashing entities
	float		alpha;				// ignore if RF_TRANSLUCENT isn't set

	material_t	*skin;				// NULL for inline skin
	int			flags;

};

struct particle_t
{
	vec3_t	origin;
	int		color;
	float	alpha;
};

struct lightstyle_t
{
	vec3_t		rgb;			// 0.0 - 2.0
	float		white;			// highest of rgb
};

struct refdef_t
{
	int			width, height;		// because we might be rendering to a framebuffer

	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewangles;
	vec4_t		blend;				// rgba 0-1 full screen blend
	float		time;				// time is used to auto animate
	float		frametime;			// time since last frame, in milliseconds
	int			rdflags;			// RDF_UNDERWATER, etc

	byte *		areabits;			// if not NULL, only areas with set bits will be drawn

	// client view information
	// separate from clView since we might want to
	// fake some counts

	dlight_t *		dlights;
	entity_t *		entities;
	particle_t *	particles;
	lightstyle_t *	lightstyles;

	int				num_dlights;
	int				num_entities;
	int				num_particles;
};
