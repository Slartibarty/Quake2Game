
#pragma once

#include "../../core/core.h"
#include "../../common/q_formats.h"

#include <vector>
#include <string>

extern std::vector<dplane_t>	g_planes;
extern std::vector<dvertex_t>	g_vertices;
extern std::vector<dnode_t>	g_nodes;
extern std::vector<texinfo_t>	g_texinfos;
extern std::vector<dface_t>	g_faces;
extern std::vector<dleaf_t>	g_leafs;
extern std::vector<uint16>		g_leafFaces;
extern std::vector<dedge_t>	g_edges;
extern std::vector<int>		g_surfedges;

// new stuff!

extern std::vector<bspDrawFace_t>		g_drawFaces;
extern std::vector<bspDrawVert_t>		g_drawVerts;
extern std::vector<bspDrawIndex_t>		g_drawIndices;

// decompose

void Decompose_Main();

