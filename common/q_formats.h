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
#define MAX_MD2SKINS	32
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
// .SMF Static Mesh Format
// 
// Indices are shorts
// 
// Version 2 added tangents
// Version 3 added seperate meshes
// Version 4 added the flags member, and changed the fourcc to QSMF
//-------------------------------------------------------------------------------------------------

namespace fmtSMF
{
	inline constexpr int32 fourCC = MakeFourCC( 'Q', 'S', 'M', 'F' );
	inline constexpr int32 version = 4;

	enum flags_t
	{
		eBigIndices = 1
	};

	struct header_t
	{
		int32 fourCC;
		int32 version;
		uint32 flags;
		uint32 numMeshes;
		uint32 offsetMeshes;
		uint32 numVerts;
		uint32 offsetVerts;			// offset in file to vertex data
		uint32 numIndices;
		uint32 offsetIndices;		// offset in file to index data
	};

	// a mesh represents a single draw call, a model can have multiple meshes
	struct mesh_t
	{
		char materialName[256];		// "materials/models/alien01/grimbles.mat"
		uint32 offsetIndices;		// offset into the index buffer
		uint32 countIndices;		// number of indices
	};

	struct vertex_t
	{
		vec3 pos;
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
// The max joints are 256 so that bone indices can be stored in bytes
// 
//-------------------------------------------------------------------------------------------------

namespace fmtSKL
{
	inline constexpr int32 MaxJoints = 256;			// 256 max joints per model
	inline constexpr int32 MaxVertexWeights = 4;	// 4 joints per vertex

	inline constexpr int32 fourCC = MakeFourCC( 'Q', 'S', 'K', 'L' );
	inline constexpr int32 version = 1;

	enum flags_t
	{
		eBigIndices		= BIT( 0 ),
		eHasJoints		= BIT( 1 )
	};

	struct header_t
	{
		int32 fourCC;
		int32 version;
		uint32 flags;
		uint32 numMeshes;
		uint32 offsetMeshes;		// offset in file to mesh definitions
		uint32 numVerts;
		uint32 offsetVerts;			// offset in file to vertex data
		uint32 numIndices;
		uint32 offsetIndices;		// offset in file to index data
		uint32 numJoints;
		uint32 offsetJoints;
	};

	// a mesh represents a single draw call, a model can have multiple meshes
	struct mesh_t
	{
		char materialName[256];		// "materials/models/alien01/grimbles.mat"
		uint32 offsetIndices;		// offset into the index buffer
		uint32 countIndices;		// number of indices
	};

	// 52-bytes :(
	struct vertex_t
	{
		vec3 pos;
		vec2 st;
		vec3 normal;
		vec3 tangent;
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
// .JMDL - JaffaModel
//-------------------------------------------------------------------------------------------------

namespace fmtJMDL
{
	inline constexpr uint32 maxBones = 256;			// 256 max bones per model
	inline constexpr uint32 maxVertexWeights = 4;	// 4 joints per vertex

	inline constexpr uint32 fourCC = MakeFourCC( 'J', 'M', 'D', 'L' );
	inline constexpr uint32 version = 1;

	enum flags_t
	{
		eBigIndices		= BIT( 0 ),		// If set, uses 32-bit indices
		eUsesBones		= BIT( 1 )		// If set, uses bones and may have animations
	};

	struct header_t
	{
		uint32 fourCC;
		uint32 version;
		uint32 flags;

		uint32 numMeshes;			// mesh count
		uint32 offsetMeshes;		// offset to mesh definitions

		uint32 numVerts;			// vertex count
		uint32 offsetVerts;			// offset to vertex data

		uint32 numIndices;			// index count
		uint32 offsetIndices;		// offset to index data

		uint32 numBones;			// bone count
		uint32 offsetBones;			// offset to bone data
	};

	struct mesh_t
	{
		char materialName[256];		// "materials/models/alien01/grimbles.mat"
		uint32 offsetIndices;		// offset into the index buffer
		uint32 countIndices;		// number of indices
	};

	// When eUsesBones is not set, this is the vertex format
	struct staticVertex_t
	{
		vec3 pos;
		vec2 st;
		vec3 normal;
		vec3 tangent;
	};

	// When eUsesBones is set, this is the vertex format
	struct skinnedVertex_t
	{
		vec3 pos;
		vec2 st;
		vec3 normal;
		vec3 tangent;
		uint8 indices[maxVertexWeights];	// bone indices
		uint8 weights[maxVertexWeights];	// 0 = no effect, 255 = total effect
	};

	// When eBigIndices is set, the index format is uint32, otherwise uint16

	struct bone_t
	{
		char name[64];
		int32 parentIndex;	// index of parent bone, -1 indicates a root bone
		vec3 pos;
		vec3 rot;
	};

}

//-------------------------------------------------------------------------------------------------
// .SP2 sprite file format
//-------------------------------------------------------------------------------------------------

inline constexpr int IDSPRITEHEADER = MakeFourCC('I', 'D', 'S', '2');

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
// .WAD legacy storage format
//-------------------------------------------------------------------------------------------------

namespace wad2
{
	inline constexpr int IDWADHEADER = MakeFourCC( 'W', 'A', 'D', '2' );
	
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

inline constexpr int IDBSPHEADER = MakeFourCC( 'I', 'B', 'S', 'P' ); // MakeFourCC( 'Q', 'B', 'S', 'P' );

#define BSPVERSION 38 // 42

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
	int32		fileofs, filelen;
};

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2		// Array of dvertex_t
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7		// Raw lightmap data
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11		// Array of dedge_t
#define	LUMP_SURFEDGES		12		// Sucks
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16		// POP lump
#define	LUMP_AREAS			17		// TODO: why doesn't Quake 3 have these?
#define	LUMP_AREAPORTALS	18		// TODO: why doesn't Quake 3 have these?
#define	HEADER_LUMPS		19

struct dheader_t
{
	int32		ident;
	int32		version;
	lump_t		lumps[HEADER_LUMPS];
};

struct dmodel_t
{
	float		mins[3], maxs[3];
	float		origin[3];				// for sounds or lights
	int32		headnode;
	int32		firstface, numfaces;	// submodels just draw faces without walking the bsp tree
};


struct dvertex_t
{
	float		point[3];
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
	float		normal[3];
	float		dist;
	int32		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
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
};


struct texinfo_t
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int32		flags;			// miptex flags + overrides
	int32		value;			// light emission, etc						DEPRECATED
	char		texture[32];	// texture name (textures/*.wal)
	int32		nexttexinfo;	// for animations, -1 = end of chain
};


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct dedge_t
{
	uint16		v[2];		// vertex numbers
};

#define	MAXLIGHTMAPS	4
struct dface_t
{
	uint16		planenum;
	int16		side;

	int32		firstedge;		// we must support > 64k edges
	int16		numedges;
	int16		texinfo;

// lighting info
	uint8		styles[MAXLIGHTMAPS];
	int32		lightofs;		// start of [numstyles*surfsize] samples
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
};

struct dbrushside_t
{
	uint16		planenum;		// facing out of the leaf
	int16		texinfo;
};

struct dbrush_t
{
	int32		firstside;
	int32		numsides;
	int32		contents;
};


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
struct dvis_t
{
	int32		numclusters;
	int32		bitofs[8][2];	// bitofs[numclusters][2]
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct dareaportal_t
{
	int32		portalnum;
	int32		otherarea;
};

struct darea_t
{
	int32		numareaportals;
	int32		firstareaportal;
};

//=============================================================================

#if 0 // Remains of the old plans to revise the BSP format...

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

#endif

/*
===================================================================================================
	BSPEXT file format

	An extension of the existing BSP format
===================================================================================================
*/

inline constexpr uint32 BSPEXT_IDENT = MakeFourCC( 'I', 'E', 'X', 'T' );
inline constexpr uint32 BSPEXT_VERSION = 1;

enum bspFlags_t
{
	BSPFLAG_EXTERNAL_LIGHTMAP = BIT( 0 )	// BSP uses an external lightmap
};

enum bspLumps_t
{
	LUMP_DRAWVERTICES,		// Raw vertex buffer
	LUMP_DRAWINDICES,		// Raw index buffer
	LUMP_DRAWMESHES,
	LUMP_MODELS_EXT,
	LUMP_FACES_EXT,
	BSPEXT_LUMPS
};

struct bspExtLump_t
{
	uint32 fileofs, filelen;
};

struct bspExtHeader_t
{
	uint32 ident;
	uint32 version;
	uint32 flags;
	bspExtLump_t lumps[BSPEXT_LUMPS];
};

// LUMP_DRAWVERTICES
struct bspDrawVert_t
{
	vec3_t pos;
	vec2_t st1;			// Normal UVs
	vec2_t st2;			// Lightmap UVs
	vec3_t normal;
};

// LUMP_DRAWINDICES
using bspDrawIndex_t = uint32;

// LUMP_DRAWMESHES
struct bspDrawMesh_t
{
	uint16 texinfo;
	uint32 firstIndex;
	uint32 numIndices;
};

// LUMP_MODELS_EXT
struct bspModelExt_t
{
	uint32 firstMesh, numMeshes;
};

// LUMP_FACES_EXT
struct bspFaceExt_t
{
	uint32 firstIndex, numIndices;
};
