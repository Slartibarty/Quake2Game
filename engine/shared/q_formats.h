//-------------------------------------------------------------------------------------------------
// q_formats.h: quake file formats
//-------------------------------------------------------------------------------------------------

#pragma once

#include "../../common/q_shared.h"

//-------------------------------------------------------------------------------------------------
// PAK files
// The .pak files are just a linear collapse of a directory tree
//-------------------------------------------------------------------------------------------------

constexpr int IDPAKHEADER = MakeID('P', 'A', 'C', 'K');

struct dpackfile_t
{
	char	name[56];
	int		filepos, filelen;
};

struct dpackheader_t
{
	int		ident;		// IDPAKHEADER
	int		dirofs;
	int		dirlen;
};

#define MAX_FILES_IN_PACK	4096

//-------------------------------------------------------------------------------------------------
// Legacy PCX files
//-------------------------------------------------------------------------------------------------

struct pcx_t
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
};

//-------------------------------------------------------------------------------------------------
// .MD2 triangle model file format
//-------------------------------------------------------------------------------------------------

constexpr int IDALIASHEADER = MakeID('I', 'D', 'P', '2');

#define ALIAS_VERSION	8

#define	MAX_TRIANGLES	4096
#define MAX_VERTS		2048
#define MAX_FRAMES		512
#define MAX_MD2SKINS	64		// Slart: Bumped to 64 from 32, the opfor scientist has 33 textures
#define	MAX_SKINNAME	64

struct dstvert_t
{
	short	s;
	short	t;
};

struct dtriangle_t
{
	short	index_xyz[3];
	short	index_st[3];
};

struct dtrivertx_t
{
	byte	v[3];			// scaled byte to fit in frame mins/maxs
	byte	lightnormalindex;
};

#define DTRIVERTX_V0   0
#define DTRIVERTX_V1   1
#define DTRIVERTX_V2   2
#define DTRIVERTX_LNI  3
#define DTRIVERTX_SIZE 4

struct daliasframe_t
{
	float		scale[3];		// multiply byte verts by this
	float		translate[3];	// then add this
	char		name[16];		// frame name from grabbing
	dtrivertx_t	verts[1];		// variable sized
};


// Header
struct dmdl_t
{
	int			ident;
	int			version;

	int			skinwidth;
	int			skinheight;
	int			framesize;		// byte size of each frame

	int			num_skins;
	int			num_xyz;
	int			num_st;			// greater than num_xyz for seams
	int			num_tris;
	int			num_glcmds;		// dwords in strip/fan command list
	int			num_frames;

	int			ofs_skins;		// each skin is a MAX_SKINNAME string
	int			ofs_st;			// byte offset from start for stverts
	int			ofs_tris;		// offset for dtriangles
	int			ofs_frames;		// offset for first frame
	int			ofs_glcmds;	
	int			ofs_end;		// end of file
};

// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.

//-------------------------------------------------------------------------------------------------
// Studio models
//
// Studio models are position independent, so the cache manager can move them.
//-------------------------------------------------------------------------------------------------

namespace
{
	constexpr int32 IDSTUDIOHEADER = MakeID( 'I', 'D', 'S', 'T' );
	constexpr int32 IDSTUDIOSEQHEADER = MakeID( 'I', 'D', 'S', 'Q' );

	constexpr auto MAXSTUDIOTRIANGLES	= 20000;	// TODO: tune this
	constexpr auto MAXSTUDIOVERTS		= 2048;		// TODO: tune this
	constexpr auto MAXSTUDIOSEQUENCES	= 2048;		// total animation sequences -- KSH incremented
	constexpr auto MAXSTUDIOSKINS		= 100;		// total textures
	constexpr auto MAXSTUDIOSRCBONES	= 512;		// bones allowed at source movement
	constexpr auto MAXSTUDIOBONES		= 128;		// total bones actually used
	constexpr auto MAXSTUDIOMODELS		= 32;		// sub-models per model
	constexpr auto MAXSTUDIOBODYPARTS	= 32;
	constexpr auto MAXSTUDIOGROUPS		= 16;
	constexpr auto MAXSTUDIOANIMATIONS	= 2048;
	constexpr auto MAXSTUDIOMESHES		= 256;
	constexpr auto MAXSTUDIOEVENTS		= 1024;
	constexpr auto MAXSTUDIOPIVOTS		= 256;
	constexpr auto MAXSTUDIOCONTROLLERS	= 8;

	struct studiohdr_t
	{
		int					id;
		int					version;

		char				name[64];
		int					length;

		vec3_t				eyeposition;	// ideal eye position
		vec3_t				min;			// ideal movement hull size
		vec3_t				max;

		vec3_t				bbmin;			// clipping bounding box
		vec3_t				bbmax;

		int					flags;

		int					numbones;			// bones
		int					boneindex;

		int					numbonecontrollers;		// bone controllers
		int					bonecontrollerindex;

		int					numhitboxes;			// complex bounding boxes
		int					hitboxindex;

		int					numseq;				// animation sequences
		int					seqindex;

		int					numseqgroups;		// demand loaded sequences
		int					seqgroupindex;

		int					numtextures;		// raw textures
		int					textureindex;
		int					texturedataindex;

		int					numskinref;			// replaceable textures
		int					numskinfamilies;
		int					skinindex;

		int					numbodyparts;
		int					bodypartindex;

		int					numattachments;		// queryable attachable points
		int					attachmentindex;

		int					soundtable;
		int					soundindex;
		int					soundgroups;
		int					soundgroupindex;

		int					numtransitions;		// animation node to animation node transition graph
		int					transitionindex;
	};

	// header for demand loaded sequence group data
	struct studioseqhdr_t
	{
		int					id;
		int					version;

		char				name[64];
		int					length;
	};

	// bones
	struct mstudiobone_t
	{
		char				name[32];	// bone name for symbolic links
		int		 			parent;		// parent bone
		int					flags;		// ??
		int					bonecontroller[6];	// bone controller index, -1 == none
		float				value[6];	// default DoF values
		float				scale[6];   // scale for delta DoF values
	};


	// bone controllers
	struct mstudiobonecontroller_t
	{
		int					bone;	// -1 == 0
		int					type;	// X, Y, Z, XR, YR, ZR, M
		float				start;
		float				end;
		int					rest;	// byte index value at rest
		int					index;	// 0-3 user set controller, 4 mouth
	};

	// intersection boxes
	struct mstudiobbox_t
	{
		int					bone;
		int					group;			// intersection group
		vec3_t				bbmin;		// bounding box
		vec3_t				bbmax;
	};

	//
	// demand loaded sequence groups
	//
	struct mstudioseqgroup_t
	{
		char				label[32];	// textual name
		char				name[64];	// file name
		int32				unused1;    // was "cache"  - index pointer
		int					unused2;    // was "data" -  hack for group 0
	};

	// sequence descriptions
	struct mstudioseqdesc_t
	{
		char				label[32];	// sequence label

		float				fps;		// frames per second	
		int					flags;		// looping/non-looping flags

		int					activity;
		int					actweight;

		int					numevents;
		int					eventindex;

		int					numframes;	// number of frames per sequence

		int					numpivots;	// number of foot pivots
		int					pivotindex;

		int					motiontype;
		int					motionbone;
		vec3_t				linearmovement;
		int					automoveposindex;
		int					automoveangleindex;

		vec3_t				bbmin;		// per sequence bounding box
		vec3_t				bbmax;

		int					numblends;
		int					animindex;		// mstudioanim_t pointer relative to start of sequence group data
											// [blend][bone][X, Y, Z, XR, YR, ZR]

		int					blendtype[2];	// X, Y, Z, XR, YR, ZR
		float				blendstart[2];	// starting value
		float				blendend[2];	// ending value
		int					blendparent;

		int					seqgroup;		// sequence group for demand loading

		int					entrynode;		// transition node at entry
		int					exitnode;		// transition node at exit
		int					nodeflags;		// transition rules

		int					nextseq;		// auto advancing sequences
	};

	// events
	struct mstudioevent_t
	{
		int 				frame;
		int					event;
		int					type;
		char				options[64];
	};

	// pivots
	struct mstudiopivot_t
	{
		vec3_t				org;	// pivot point
		int					start;
		int					end;
	};

	// attachment
	struct mstudioattachment_t
	{
		char				name[32];
		int					type;
		int					bone;
		vec3_t				org;	// attachment point
		vec3_t				vectors[3];
	};

	struct mstudioanim_t
	{
		unsigned short	offset[6];
	};

	// animation frames
	union mstudioanimvalue_t
	{
		struct {
			byte	valid;
			byte	total;
		} num;
		short		value;
	};



	// body part index
	struct mstudiobodyparts_t
	{
		char				name[64];
		int					nummodels;
		int					base;
		int					modelindex; // index into models array
	};



	// skin info
	struct mstudiotexture_t
	{
		char					name[64];
		int						flags;
		int						width;
		int						height;
		int						index;
	};


	// skin families
	// short	index[skinfamilies][skinref]

	// studio models
	struct mstudiomodel_t
	{
		char				name[64];

		int					type;

		float				boundingradius;

		int					nummesh;
		int					meshindex;

		int					numverts;		// number of unique vertices
		int					vertinfoindex;	// vertex bone info
		int					vertindex;		// vertex vec3_t
		int					numnorms;		// number of unique surface normals
		int					norminfoindex;	// normal bone info
		int					normindex;		// normal vec3_t

		int					numgroups;		// deformation groups
		int					groupindex;
	};


	// vec3_t	boundingbox[model][bone][2];	// complex intersection info


	// meshes
	struct mstudiomesh_t
	{
		int					numtris;
		int					triindex;
		int					skinref;
		int					numnorms;		// per mesh normals
		int					normindex;		// normal vec3_t
	};

	// triangles
#if 0
	struct mstudiotrivert_t
	{
		short				vertindex;		// index into vertex array
		short				normindex;		// index into normal array
		short				s, t;			// s,t position on skin
	};
#endif

	// lighting options
	constexpr auto STUDIO_NF_FLATSHADE	= 0x0001;
	constexpr auto STUDIO_NF_CHROME		= 0x0002;
	constexpr auto STUDIO_NF_FULLBRIGHT	= 0x0004;
	constexpr auto STUDIO_NF_NOMIPS		= 0x0008;
	constexpr auto STUDIO_NF_ALPHA		= 0x0010;
	constexpr auto STUDIO_NF_ADDITIVE	= 0x0020;
	constexpr auto STUDIO_NF_MASKED		= 0x0040;

	// motion flags
	constexpr auto STUDIO_X			= 0x0001;
	constexpr auto STUDIO_Y			= 0x0002;
	constexpr auto STUDIO_Z			= 0x0004;
	constexpr auto STUDIO_XR		= 0x0008;
	constexpr auto STUDIO_YR		= 0x0010;
	constexpr auto STUDIO_ZR		= 0x0020;
	constexpr auto STUDIO_LX		= 0x0040;
	constexpr auto STUDIO_LY		= 0x0080;
	constexpr auto STUDIO_LZ		= 0x0100;
	constexpr auto STUDIO_AX		= 0x0200;
	constexpr auto STUDIO_AY		= 0x0400;
	constexpr auto STUDIO_AZ		= 0x0800;
	constexpr auto STUDIO_AXR		= 0x1000;
	constexpr auto STUDIO_AYR		= 0x2000;
	constexpr auto STUDIO_AZR		= 0x4000;
	constexpr auto STUDIO_TYPES		= 0x7FFF;
	constexpr auto STUDIO_RLOOP		= 0x8000;	// controller that wraps shortest distance

	// sequence flags
	constexpr auto STUDIO_LOOPING = 0x0001;

	// bone flags
	constexpr auto STUDIO_HAS_NORMALS = 0x0001;
	constexpr auto STUDIO_HAS_VERTICES = 0x0002;
	constexpr auto STUDIO_HAS_BBOX = 0x0004;
	constexpr auto STUDIO_HAS_CHROME = 0x0008;	// if any of the textures have chrome on them

}

//-------------------------------------------------------------------------------------------------
// .SP2 sprite file format
//-------------------------------------------------------------------------------------------------

constexpr int IDSPRITEHEADER = MakeID('I', 'D', 'S', '2');

#define SPRITE_VERSION	2

struct dsprframe_t
{
	int		width, height;
	int		origin_x, origin_y;		// raster coordinates inside pic
	char	name[MAX_SKINNAME];		// name of pcx file
};

struct dsprite_t
{
	int			ident;
	int			version;
	int			numframes;
	dsprframe_t	frames[1];			// variable sized
};

//-------------------------------------------------------------------------------------------------
// .WAL texture file format
//-------------------------------------------------------------------------------------------------

#define	MIPLEVELS	4
struct miptex_t
{
	char		name[32];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	char		animname[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
};

//-------------------------------------------------------------------------------------------------
// .WAS material script format
//-------------------------------------------------------------------------------------------------

struct was_t
{
	char		animname[32];			// next frame in animation chain
	uint32		flags;
	uint32		contents;
	uint32		value;
};

//-------------------------------------------------------------------------------------------------
// .WAD legacy storage format
//-------------------------------------------------------------------------------------------------

namespace wad2
{
	constexpr int IDWADHEADER = MakeID( 'W', 'A', 'D', '2' );
	
	// obsolete
	enum compression_t : int8
	{
		CMP_NONE = 0,
		CMP_LZSS
	};

	enum type_t : int8
	{
		TYP_LUMPY = 64,					// 64 + grab command number
		TYP_PALETTE = 64,
		TYP_QTEX = 65,
		TYP_QPIC = 66,
		TYP_SOUND = 67,
		TYP_MIPTEX = 68,
	};

	struct qpic_t
	{
		int			width, height;
		//byte		data[4];			// variably sized
	};

	struct miptex_t
	{
		char		name[16];
		int32		width, height;
		int32		nOffsets[4];
	};

	struct wadinfo_t
	{
		int32		identification;		// should be WAD2 or 2DAW
		int32		numlumps;
		int32		infotableofs;
	};

	struct lumpinfo_t
	{
		int32			filepos;
		int32			disksize;
		int32			size;			// uncompressed
		type_t			type;
		compression_t	compression;	// Always uncompressed
		int16			pad;
		char			name[16];		// must be null terminated
	};
}

//-------------------------------------------------------------------------------------------------
// .BSP file format
//-------------------------------------------------------------------------------------------------

constexpr int IDBSPHEADER = MakeID('I', 'B', 'S', 'P');

#define BSPVERSION	38

// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	MAX_MAP_MODELS		1024
#define	MAX_MAP_BRUSHES		8192
#define	MAX_MAP_ENTITIES	2048
#define	MAX_MAP_ENTSTRING	0x40000
#define	MAX_MAP_TEXINFO		8192

#define	MAX_MAP_AREAS		256
#define	MAX_MAP_AREAPORTALS	1024
#define	MAX_MAP_PLANES		65536
#define	MAX_MAP_NODES		65536
#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
#define	MAX_MAP_LEAFFACES	65536
#define	MAX_MAP_LEAFBRUSHES 65536
#define	MAX_MAP_PORTALS		65536
#define	MAX_MAP_EDGES		128000
#define	MAX_MAP_SURFEDGES	256000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_VISIBILITY	0x100000

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================

struct lump_t
{
	int		fileofs, filelen;
};

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16
#define	LUMP_AREAS			17
#define	LUMP_AREAPORTALS	18
#define	HEADER_LUMPS		19

struct dheader_t
{
	int			ident;
	int			version;	
	lump_t		lumps[HEADER_LUMPS];
};

struct dmodel_t
{
	float		mins[3], maxs[3];
	float		origin[3];				// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces
										// without walking the bsp tree
};


struct dvertex_t
{
	float	point[3];
};


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are always opposites

struct dplane_t
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
};


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in q_shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	LAST_VISIBLE_CONTENTS	CONTENTS_MIST

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000



#define	SURF_LIGHT		0x1		// value will hold the light strength

#define	SURF_SLICK		0x2		// effects game physics

#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture




struct dnode_t
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
};


struct texinfo_t
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			flags;			// miptex flags + overrides
	int			value;			// light emission, etc
	char		texture[32];	// texture name (textures/*.wal)
	int			nexttexinfo;	// for animations, -1 = end of chain
};


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct dedge_t
{
	unsigned short	v[2];		// vertex numbers
};

#define	MAXLIGHTMAPS	4
struct dface_t
{
	unsigned short	planenum;
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
};

struct dleaf_t
{
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;
	short			area;

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
};

struct dbrushside_t
{
	unsigned short	planenum;		// facing out of the leaf
	short	texinfo;
};

struct dbrush_t
{
	int			firstside;
	int			numsides;
	int			contents;
};

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
struct dvis_t
{
	int			numclusters;
	int			bitofs[8][2];	// bitofs[numclusters][2]
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct dareaportal_t
{
	int		portalnum;
	int		otherarea;
};

struct darea_t
{
	int		numareaportals;
	int		firstareaportal;
};
