// trace.c

#include "qrad.h"
// This file doesn't make use of any private qrad definitions
// (mostly because it's largely the same as light.exe's)

typedef struct tnode_s
{
	int		type;
	vec3_t	normal;
	float	dist;
	int		children[2];
	int		pad;
} tnode_t;

tnode_t		*tnodes, *tnode_p;

/*
==============
MakeTnode

Converts the disk node structure into the efficient tracing structure
==============
*/
void MakeTnode (int nodenum)
{
	tnode_t			*t;
	dplane_t		*plane;
	int				i;
	dnode_t 		*node;
	
	t = tnode_p++;

	node = dnodes + nodenum;
	plane = dplanes + node->planenum;

	t->type = plane->type;
	VectorCopy (plane->normal, t->normal);
	t->dist = plane->dist;
	
	for (i=0 ; i<2 ; i++)
	{
		if (node->children[i] < 0)
			t->children[i] = (dleafs[-node->children[i] - 1].contents & CONTENTS_SOLID) | (1<<31);
		else
		{
			t->children[i] = tnode_p - tnodes;
			MakeTnode (node->children[i]);
		}
	}
			
}


/*
=============
MakeTnodes

Loads the node structure out of a .bsp file to be used for light occlusion
=============
*/
void MakeTnodes (dmodel_t *bm)
{
	// 32 byte align the structs
	tnodes = (tnode_t *)malloc( (numnodes+1) * sizeof(tnode_t));
	tnodes = (tnode_t *)(((intptr_t)tnodes + 31)&~31);
	tnode_p = tnodes;

	MakeTnode (0);
}

//==========================================================

int TestLine_r (int node, vec3_t start, vec3_t stop)
{
	tnode_t	*tnode;
	float	front, back;
	vec3_t	mid;
	float	frac;
	int		side;
	int		r;

	if (node & (1<<31))
		return node & ~(1<<31);	// leaf node

	tnode = &tnodes[node];
	switch (tnode->type)
	{
	case PLANE_X:
		front = start[0] - tnode->dist;
		back = stop[0] - tnode->dist;
		break;
	case PLANE_Y:
		front = start[1] - tnode->dist;
		back = stop[1] - tnode->dist;
		break;
	case PLANE_Z:
		front = start[2] - tnode->dist;
		back = stop[2] - tnode->dist;
		break;
	default:
		front = (start[0]*tnode->normal[0] + start[1]*tnode->normal[1] + start[2]*tnode->normal[2]) - tnode->dist;
		back = (stop[0]*tnode->normal[0] + stop[1]*tnode->normal[1] + stop[2]*tnode->normal[2]) - tnode->dist;
		break;
	}

	if (front >= -ON_EPSILON && back >= -ON_EPSILON)
		return TestLine_r (tnode->children[0], start, stop);
	
	if (front < ON_EPSILON && back < ON_EPSILON)
		return TestLine_r (tnode->children[1], start, stop);

	side = front < 0;
	
	frac = front / (front-back);

	mid[0] = start[0] + (stop[0] - start[0])*frac;
	mid[1] = start[1] + (stop[1] - start[1])*frac;
	mid[2] = start[2] + (stop[2] - start[2])*frac;

	r = TestLine_r (tnode->children[side], start, mid);
	if (r)
		return r;
	return TestLine_r (tnode->children[!side], mid, stop);
}
