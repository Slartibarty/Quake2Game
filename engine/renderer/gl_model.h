
/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
===================================================================================================

	Brush models

===================================================================================================
*/

struct mvertex_t
{
	vec3_t		position;
};

struct mmodel_t
{
	vec3_t		mins, maxs;
	vec3_t		origin;			// for sounds or lights
	float		radius;
	int32		headnode;
	int32		visleafs;		// not including the solid leaf 0
	int32		firstface, numfaces;
	uint32		firstMesh, numMeshes;
};

#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWTURB		0x10
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80

struct medge_t
{
	uint16		v[2];
};

struct mtexinfo_t
{
	float		vecs[2][4];
	int32		flags;
	int32		numframes;
	mtexinfo_t	*next;		// animation chain
	material_t	*material;
};

#define	VERTEXSIZE	7

struct glpoly_t
{
	glpoly_t	*next;
	glpoly_t	*chain;
	int			numverts;
	int			flags;					// for SURF_UNDERWATER (not needed anymore?)
	float		verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
};

struct msurface_t
{
	int32		frameCount;		// The frame that this surface was last visible on

	cplane_t *	plane;
	int32		flags;

	int32		firstedge;			// look up in model->surfedges[], negative numbers
	int32		numedges;			// are backwards edges
	
	int16		texturemins[2];
	int16		extents[2];

	int32		light_s, light_t;	// gl lightmap coordinates

	glpoly_t *	polys;				// multiple if warped
	uint32		firstIndex;			// First vertex into the world render data
	uint32		numIndices;			// Number of vertices to draw

	mtexinfo_t	*texinfo;

// lighting info
	int32		dlightframe;
	int32		dlightbits;

	int32		lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	float		cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	byte *		samples;			// [numstyles*surfsize]
};

struct mnode_t
{
// common with leaf
	int32		contents;		// -1, to differentiate from leafs
	int32		visframe;		// node needs to be traversed if current
	
	vec3_t		mins;			// for bounding box culling
	vec3_t		maxs;

	mnode_t *	parent;

// node specific
	cplane_t *	plane;
	mnode_t *	children[2];

	int32		firstsurface;
	int32		numsurfaces;
};

struct mleaf_t
{
// common with node
	int32		contents;		// will be a negative contents number
	int32		visframe;		// node needs to be traversed if current

	vec3_t		mins;			// for bounding box culling
	vec3_t		maxs;

	mnode_t *	parent;

// leaf specific
	int32		cluster;
	int32		area;

	int32		firstmarksurface;
	int32		nummarksurfaces;
};

/*
===================================================================================================

	Models

===================================================================================================
*/

struct mSMFMesh_t
{
	uint32			offset, count;
	material_t *	material;
};

// in-memory SMF data
struct mSMF_t
{
	GLuint		vao, vbo, ebo;
	GLenum		type;			// index type
	uint32		numMeshes;
	// followed by a variable number of meshes
};


//=================================================================================================

//
// Whole model
//

enum modtype_t { mod_bad, mod_brush, mod_sprite, mod_alias, mod_smf, mod_jmdl };

struct model_t
{
	char		name[MAX_QPATH];

	int			registration_sequence;

	modtype_t	type;
	int			numframes;
	
	int			flags;

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping (unused)
//
//	qboolean	clipbox;
//	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	uint32		firstMesh, numMeshes;

	int			numsubmodels;
	mmodel_t	*submodels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	int			firstnode;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	dvis_t		*vis;

	byte		*lightdata;

	// for alias models and skins
	material_t	*skins[MAX_MD2SKINS];

	size_t		extradatasize;
	void *		extradata;
};

struct staticLight_t
{
	vec3_t origin;
	vec3_t color;
	uint32 intensity;	// 0 - 1
};

extern staticLight_t	mod_staticLights[MAX_ENTITIES];
extern int				mod_numStaticLights;

//-------------------------------------------------------------------------------------------------

void		Mod_Init();
mleaf_t *	Mod_PointInLeaf( vec3_t p, model_t *model );
model_t *	Mod_ForName( const char *name, bool crash );
byte *		Mod_ClusterPVS( int cluster, model_t *model );

void		Mod_Modellist_f();

void		Mod_Free( model_t *pModel );
void		Mod_FreeAll();
