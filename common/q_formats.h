//-------------------------------------------------------------------------------------------------
// q_formats.h: quake file formats
//
// bspflags.h is included down below
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
// PAK files
// The .pak files are just a linear collapse of a directory tree
//-------------------------------------------------------------------------------------------------

inline constexpr int IDPAKHEADER = MakeFourCC('P', 'A', 'C', 'K');

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

inline constexpr int IDALIASHEADER = MakeFourCC('I', 'D', 'P', '2');

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
// Please remove me
//-------------------------------------------------------------------------------------------------

namespace
{
	constexpr int32 IDSTUDIOHEADER = MakeFourCC( 'I', 'D', 'S', 'T' );
	constexpr int32 IDSTUDIOSEQHEADER = MakeFourCC( 'I', 'D', 'S', 'Q' );

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
// .SMF Static Mesh Format
// 
// Indices are shorts
//-------------------------------------------------------------------------------------------------

namespace fmtSMF
{
	inline constexpr int32 fourCC = MakeFourCC( 'B', 'R', 'U', 'H' );
	inline constexpr int32 version = 2;

	struct header_t
	{
		int32 fourCC;
		int32 version;
		uint32 numVerts;
		uint32 offsetVerts;		// offset in file to vertex data
		uint32 numIndices;
		uint32 offsetIndices;	// offset in file to index data
		char materialName[256];	// "materials/models/alien01/grimbles.mat"
	};

	struct vertex_t
	{
		vec3 xyz;
		vec2 st;
		vec3 normal;
		vec3 tangent;
	};

	using index_t = uint16;
}

//-------------------------------------------------------------------------------------------------
// .SKL SKeLetal model format
// 
// Indices are shorts
//-------------------------------------------------------------------------------------------------

namespace fmtSKL
{
	inline constexpr int32 MaxJoints = 256;			// 256 max joints per model
	inline constexpr int32 MaxVertexWeights = 4;	// 4 joints per vertex

	inline constexpr int32 fourCC = MakeFourCC( 'M', 'A', 'T', 'E' );
	inline constexpr int32 version = 1;

	struct header_t
	{
		int32 fourCC;
		int32 version;
		uint32 numVerts;
		uint32 offsetVerts;
		uint32 numIndices;
		uint32 offsetIndices;
		uint32 numJoints;
		uint32 offsetJoints;
		char materialName[256];	// "materials/models/alien01/grimbles.mat"
	};

	struct vertex_t
	{
		float x, y, z;
		float s, t;
		float n1, n2, n3;
		uint8 indices[MaxVertexWeights];	// bone indices
		uint8 weights[MaxVertexWeights];	// 0 = no effect, 255 = total effect
	};

	using index_t = uint16;

	struct joint_t
	{
		int32 parentIndex;	// index of parent bone, -1 indicates a root bone
		float x, y, z;		// relative
	};
}

//-------------------------------------------------------------------------------------------------
// .SP2 sprite file format
//-------------------------------------------------------------------------------------------------

constexpr int IDSPRITEHEADER = MakeFourCC('I', 'D', 'S', '2');

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
	constexpr int IDWADHEADER = MakeFourCC( 'W', 'A', 'D', '2' );
	
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

/*
===================================================================================================
.BSP file format

Notes on the new mesh data format:
	The old GL renderer traverses the BSP tree node by node. Nodes contain an offset and count into
	the face lump, those faces each have an offset into the texinfo lump that contain material name
	and etc. The faces are drawn via indexes into the edge lump that have indexes into the vertex
	lump, the new node format will contain a new offset and count into the "new" face lump, the new
	faces are obviously stored contiguously. A face is just an abstraction to allow multiple
	materials per-node, the faces will just contain the material name, and an offset into the mesh
	data lump, and a count.
	We don't have to worry too much about cache locality since this information will just be parsed
	locally before being stored into an optimised local format (see m<name>_t structures).

A few comments on maximum limits:
	Max limits are pretty much purely dependent on the size of the integers in the structures below
	legacy face data is bounded by uint16s, so 65536 max.
===================================================================================================
*/

inline constexpr int IDBSPHEADER = MakeFourCC( 'Q', 'B', 'S', 'P' );

#define BSPVERSION 42

// almost none of these constants are used by the game

#define	MAX_MAP_MODELS		2048	// 1024
#define	MAX_MAP_BRUSHES		8192
#define	MAX_MAP_ENTITIES	16384	// 2048
#define	MAX_MAP_ENTSTRING	0x40000
#define	MAX_MAP_TEXINFO		16384	// 8192

#define	MAX_MAP_AREAS		1024	// 256
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

// key / value pair sizes in the entities lump

#define	MAX_KEY		32
#define	MAX_VALUE	1024

// legacy predefined yaw angles, I don't think the game handles these anywhere anymore

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2

// world min/max values. arbitrary really, could be even bigger
// in fact, do we even need this?
// this is also the wrong place for these defines, should be in q_shared.h

inline constexpr float MAX_WORLD_COORD = 131072.0f;
inline constexpr float MIN_WORLD_COORD = -131072.0f;
inline constexpr float WORLD_SIZE = MAX_WORLD_COORD - MIN_WORLD_COORD;

// the maximum traceable distance, from corner to corner
inline constexpr float MAX_TRACE_LENGTH = mconst::sqrt3 * WORLD_SIZE;

//=============================================================================

struct lump_t
{
	int		fileofs, filelen;
};

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2		// deprecated, remove
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7		// deprecated, remove
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11		// deprecated, remove
#define	LUMP_SURFEDGES		12		// deprecated, remove
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_AREAS			16		// TODO: why doesn't Quake 3 have these?
#define	LUMP_AREAPORTALS	17		// TODO: why doesn't Quake 3 have these?
#define	LUMP_DRAWFACES		18		// NEW!
#define	LUMP_DRAWVERTS		19		// NEW!
#define	LUMP_DRAWINDICES	20		// NEW! uint16 indices
#define	HEADER_LUMPS		21

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
	int			firstface, numfaces;	// submodels just draw faces without walking the bsp tree
};


struct dvertex_t
{
	float	point[3];
};


// TODO: move these defines to math.h? plane maths should be more generic

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


#include "bspflags.h"


struct dnode_t
{
	int32		planenum;
	int32		children[2];	// negative numbers are -(leafs+1), not nodes
	int16		mins[3];		// for frustom culling
	int16		maxs[3];
	uint16		firstface;
	uint16		numfaces;		// counting both sides
	// new
	uint32		newFirstFace;
	uint32		newNumFaces;
};


struct texinfo_t
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			flags;			// miptex flags + overrides
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
	int32		contents;			// OR of all brushes (not needed?)

	int16		cluster;
	int16		area;

	int16		mins[3];			// for frustum culling
	int16		maxs[3];

	uint16		firstleafface;
	uint16		numleaffaces;

	uint16		firstleafbrush;
	uint16		numleafbrushes;

	uint16		newFirstFace;
	uint16		newNumFaces;
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

//=============================================================================

#define TEMP_MAX_QPATH 128		// temp bigger qpath

// unique for every node
struct bspDrawFace_t
{
	char	materialName[TEMP_MAX_QPATH];	// TODO: need a string table! the material associated with this polysoup
	uint16	firstIndex;						// index into draw index list, which is in turn an index into draw verts
	uint16	numIndices;						// number of indices
};

struct bspDrawVert_t
{
	vec3 xyz;
	vec2 st;
	vec2 st2;
	//vec3 normal;		// TODO: normals
};

using bspDrawIndex_t = uint16;
