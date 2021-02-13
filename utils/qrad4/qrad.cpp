// qrad.c

#include "qrad.h"



/*

NOTES
-----

every surface must be divided into at least two patches each axis

*/

// Yeah, okay, cool, thanks
struct vec3hack_t
{
	vec3_t data;

	float operator[]( int i )
	{
		return data[i];
	}
};

std::vector<patch_t> g_patches;
patch_t		*face_patches[MAX_MAP_FACES];
entity_t	*face_entity[MAX_MAP_FACES];

std::vector<vec3hack_t> g_radiosity;		// light leaving a patch
std::vector<vec3hack_t> g_illumination;		// light arriving at a patch

vec3_t		face_offset[MAX_MAP_FACES];		// for rotating bmodels
dplane_t	backplanes[MAX_MAP_PLANES];

char		inbase[32], outbase[32];

int			fakeplanes;					// created planes for origin offset 

int			numbounce = 128;
qboolean	extrasamples;

float	subdiv = 64;

float	ambient = 0;
float	maxlight = 255;

float	lightscale = 1.0f;
float	g_smoothing_threshold;

qboolean	nopvs;

char		source[1024];

float	direct_scale =	0.4f;
float	entity_scale =	1.0f;

/*
===================================================================

MISC

===================================================================
*/


/*
=============
MakeBackplanes
=============
*/
void MakeBackplanes (void)
{
	int		i;

	for (i=0 ; i<numplanes ; i++)
	{
		backplanes[i].dist = -dplanes[i].dist;
		VectorSubtract (vec3_origin, dplanes[i].normal, backplanes[i].normal);
	}
}

int		leafparents[MAX_MAP_LEAFS];
int		nodeparents[MAX_MAP_NODES];

/*
=============
MakeParents
=============
*/
void MakeParents (int nodenum, int parent)
{
	int		i, j;
	dnode_t	*node;

	nodeparents[nodenum] = parent;
	node = &dnodes[nodenum];

	for (i=0 ; i<2 ; i++)
	{
		j = node->children[i];
		if (j < 0)
			leafparents[-j - 1] = nodenum;
		else
			MakeParents (j, nodenum);
	}
}


/*
===================================================================

TRANSFER SCALES

===================================================================
*/

int	PointInLeafnum (vec3_t point)
{
	int		nodenum;
	vec_t	dist;
	dnode_t	*node;
	dplane_t	*plane;

	nodenum = 0;
	while (nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planenum];
		dist = DotProduct (point, plane->normal) - plane->dist;
		if (dist > 0)
			nodenum = node->children[0];
		else
			nodenum = node->children[1];
	}

	return -nodenum - 1;
}


dleaf_t		*PointInLeaf (vec3_t point)
{
	int		num;

	num = PointInLeafnum (point);
	return &dleafs[num];
}


qboolean PvsForOrigin (vec3_t org, byte *pvs)
{
	dleaf_t	*leaf;

	if (!visdatasize)
	{
		memset (pvs, 255, (numleafs+7)/8 );
		return true;
	}

	leaf = PointInLeaf (org);
	if (leaf->cluster == -1)
		return false;		// in solid leaf

	DecompressVis (dvisdata + dvis->bitofs[leaf->cluster][DVIS_PVS], pvs);
	return true;
}


/*
=============
MakeTransfers

=============
*/
int	total_transfer;

void MakeTransfers (int i)
{
	int			j;
	vec3_t		delta;
	vec_t		dist, scale;
	float		trans;
	int			itrans;
	patch_t		*patch, *patch2;
	float		total;
	dplane_t	plane;
	vec3_t		origin;
	float		*transfers, *all_transfers;
	int			s;
	int			itotal;
	byte		pvs[(MAX_MAP_LEAFS+7)/8];
	int			cluster;

	patch = &g_patches[i];
	total = 0;

	VectorCopy (patch->origin, origin);
	plane = *patch->plane;

	if (!PvsForOrigin (patch->origin, pvs))
		return;

	// find out which patch2s will collect light
	// from patch

	transfers = (float *)malloc( g_patches.size() * sizeof( float ) );
	if ( !transfers )
		Error( "Memory allocation failure" );

	all_transfers = transfers;
	patch->numtransfers = 0;
	for (j=0; j<(int)g_patches.size(); j++)
	{
		patch2 = &g_patches[j];

		transfers[j] = 0;

		if (j == i)
			continue;

		// check pvs bit
		if (!nopvs)
		{
			cluster = patch2->cluster;
			if (cluster == -1)
				continue;
			if ( ! ( pvs[cluster>>3] & (1<<(cluster&7)) ) )
				continue;		// not in pvs
		}

		// calculate vector
		VectorSubtract (patch2->origin, origin, delta);
		dist = VectorNormalize (delta);
		if (!dist)
			continue;	// should never happen

		// reletive angles
		scale = DotProduct (delta, plane.normal);
		scale *= -DotProduct (delta, patch2->plane->normal);
		if (scale <= 0)
			continue;

		// check exact tramsfer
		if (TestLine_r (0, patch->origin, patch2->origin) )
			continue;

		trans = scale * patch2->area / (dist*dist);

		if (trans < 0)
			trans = 0;		// rounding errors...

		transfers[j] = trans;
		if (trans > 0)
		{
			total += trans;
			patch->numtransfers++;
		}
	}

	// copy the transfers out and normalize
	// total should be somewhere near PI if everything went right
	// because partial occlusion isn't accounted for, and nearby
	// patches have underestimated form factors, it will usually
	// be higher than PI
	if (patch->numtransfers)
	{
		transfer_t	*t;
		
		if (patch->numtransfers < 0 || patch->numtransfers > MAX_PATCHES)
			Error ("Weird numtransfers");
		s = patch->numtransfers * sizeof(transfer_t);
		patch->transfers = (transfer_t *)malloc (s);
		if (!patch->transfers)
			Error ("Memory allocation failure");

		//
		// normalize all transfers so all of the light
		// is transfered to the surroundings
		//
		t = patch->transfers;
		itotal = 0;
		for (j=0 ; j<(int)g_patches.size() ; j++)
		{
			if (transfers[j] <= 0)
				continue;
			itrans = transfers[j]*0x10000 / total;
			itotal += itrans;
			t->transfer = itrans;
			t->patch = j;
			t++;
		}
	}

	free( transfers );

	// don't bother locking around this.  not that important.
	total_transfer += patch->numtransfers;
}


/*
=============
FreeTransfers
=============
*/
void FreeTransfers (void)
{
	int		i;

	for (i=0 ; i<g_patches.size() ; i++)
	{
		free (g_patches[i].transfers);
		g_patches[i].transfers = NULL;
	}
}

//==============================================================

/*
=============
CollectLight
=============
*/
static void CollectLight( vec3_t &added )
{
	size_t i;
	int j;

	VectorClear( added );

	for ( i = 0; i < g_patches.size(); i++ )
	{
		patch_t *patch = &g_patches[i];

		// skys never collect light, it is just dropped
		if ( patch->sky )
		{
			VectorClear( g_radiosity[i].data );
			VectorClear( g_illumination[i].data );
			continue;
		}

		for ( j = 0; j < 3; j++ )
		{
			patch->totallight[j] += g_illumination[i][j] / patch->area;
			g_radiosity[i].data[j] = g_illumination[i].data[j] * patch->reflectivity[j];
		}
		
		VectorAdd( added, g_radiosity[i].data, added );
		VectorClear( g_illumination[i].data );
	}
}

/*
=============
ShootLight

Send light out to other patches
  Run multi-threaded
=============
*/
static void ShootLight( int patchnum )
{
	int			k, l;
	transfer_t	*trans;
	int			num;
	patch_t		*patch;
	vec3_t		send;

	// this is the amount of light we are distributing
	// prescale it so that multiplying by the 16 bit
	// transfer values gives a proper output value
	for ( k = 0; k < 3; k++ )
		send[k] = g_radiosity[patchnum][k] / 0x10000;
	patch = &g_patches[patchnum];

	trans = patch->transfers;
	num = patch->numtransfers;

	for ( k = 0; k < num; k++, trans++ )
	{
		for ( l = 0; l < 3; l++ )
			g_illumination[trans->patch].data[l] += send[l] * trans->transfer;
	}
}

/*
=============
BounceLight
=============
*/
static void BounceLight()
{
	int		i, j;
	vec3_t	added;
	patch_t *p;
	bool	bouncing = numbounce > 0;

	for ( i = 0; i < (int)g_patches.size(); i++ )
	{
		p = &g_patches[i];
		for ( j = 0; j < 3; j++ )
		{
			g_radiosity[i].data[j] = p->samplelight[j] * p->reflectivity[j] * p->area;
		}
	}

	for ( i = 0; bouncing; i++ )
	{
		RunThreadsOnIndividual( (int)g_patches.size(), false, ShootLight );
		CollectLight( added );

		qprintf( "\tBounce #%d added RGB(%.0f, %.0f, %.0f)\n", i + 1, added[0], added[1], added[2] ); // DIRECT_LIGHT

		if ( i + 1 == numbounce || ( added[0] < 1.0f && added[1] < 1.0f && added[2] < 1.0f ) )
			bouncing = false;
	}
}

//==============================================================

void CheckPatches (void)
{
	size_t i;
	patch_t *patch;

	for (i=0 ; i<g_patches.size() ; i++)
	{
		patch = &g_patches[i];
		if (patch->totallight[0] < 0 || patch->totallight[1] < 0 || patch->totallight[2] < 0)
			Error ("negative patch totallight\n");
	}
}

/*
=============
RadWorld
=============
*/
void RadWorld (void)
{
	if (numnodes == 0 || numfaces == 0)
		Error ("Empty map");
	MakeBackplanes ();
	MakeParents (0, -1);
	MakeTnodes (&dmodels[0]);

	// turn each face into a single patch
	MakePatches ();

	// subdivide patches to a maximum dimension
	SubdividePatches ();

	// create directlights out of patches and lights
	CreateDirectLights ();

	// build initial facelights
	RunThreadsOnIndividual (numfaces, true, BuildFacelights);

	if (numbounce > 0)
	{
		// build transfer lists
		RunThreadsOnIndividual ((int)g_patches.size(), true, MakeTransfers);
		qprintf ("transfer lists: %5.1f megs\n"
		, (float)total_transfer * sizeof(transfer_t) / (1024*1024));

		// allocate memory for g_radiosity/g_illumination
		g_radiosity.resize( g_patches.size() );
		g_illumination.resize( g_patches.size() );

		// spread light around
		BounceLight ();
		
		FreeTransfers ();

		CheckPatches ();
	}

	// blend bounced light into direct light and save
	PairEdges ();
	LinkPlaneFaces ();

	lightdatasize = 0;
	RunThreadsOnIndividual (numfaces, true, FinalLightFace);
}


/*
========
main

light modelfile
========
*/
int main (int argc, char **argv)
{
	int		i;
	double	start, end;
	char	name[1024];

	printf ("----- lunar radiosity ----\n");

	g_smoothing_threshold = cos( DEG2RAD( 45.0f ) );

	g_patches.reserve( MIN_PATCHES );

	Time_Init();

	for (i=1 ; i<argc ; i++)
	{
		if (!strcmp(argv[i],"-bounce"))
		{
			numbounce = atoi (argv[i+1]);
			i++;
		}
		else if (!strcmp(argv[i],"-v"))
		{
			verbose = true;
		}
		else if (!strcmp(argv[i],"-extra"))
		{
			extrasamples = true;
			printf ("extrasamples = true\n");
		}
		else if (!strcmp(argv[i],"-threads"))
		{
			numthreads = atoi (argv[i+1]);
			i++;
		}
		else if (!strcmp(argv[i],"-chop"))
		{
			subdiv = atoi (argv[i+1]);
			i++;
		}
		else if (!strcmp(argv[i],"-scale"))
		{
			lightscale = (float)atof (argv[i+1]);
			i++;
		}
		else if (!strcmp(argv[i],"-direct"))
		{
			direct_scale *= (float)atof(argv[i+1]);
			printf ("direct light scaling at %f\n", direct_scale);
			i++;
		}
		else if (!strcmp(argv[i],"-entity"))
		{
			entity_scale *= (float)atof(argv[i+1]);
			printf ("entity light scaling at %f\n", entity_scale);
			i++;
		}
		else if (!strcmp(argv[i],"-nopvs"))
		{
			nopvs = true;
			printf ("nopvs = true\n");
		}
		else if (!strcmp(argv[i],"-ambient"))
		{
			ambient = (float)(atof (argv[i+1]) * 128);
			i++;
		}
		else if (!strcmp(argv[i],"-maxlight"))
		{
			maxlight = (float)(atof (argv[i+1]) * 128);
			i++;
		}
		else if (!strcmp( argv[i], "-smooth"))
		{
			++i;
			g_smoothing_threshold = cos( DEG2RAD( (float)atof( argv[i] ) ) );
		}
		else if (!strcmp (argv[i],"-tmpin"))
			strcpy (inbase, "/tmp");
		else if (!strcmp (argv[i],"-tmpout"))
			strcpy (outbase, "/tmp");
		else
			break;
	}

	ThreadSetDefault ();

	if (maxlight > 255)
		maxlight = 255;

	if (i != argc - 1)
		Error ("usage: qrad [-v] [-chop num] [-scale num] [-ambient num] [-maxlight num] [-threads num] bspfile");

	start = Time_FloatSeconds ();

	SetQdirFromPath (argv[i]);	
	strcpy (source, ExpandArg(argv[i]));
	StripExtension (source);
	DefaultExtension (source, ".bsp");

//	ReadLightFile ();

	Q_sprintf_s (name, "%s%s", inbase, source);
	printf ("reading %s\n", name);
	LoadBSPFile (name);
	ParseEntities ();
	LoadMaterials ();

	if (!visdatasize)
	{
		printf ("No vis information, direct lighting only.\n");
		numbounce = 0;
		ambient = 0.1f;
	}

	RadWorld ();

	Q_sprintf_s (name, "%s%s", outbase, source);
	printf ("writing %s\n", name);
	WriteBSPFile (name);

	end = Time_FloatSeconds ();
	printf ("%5.1f seconds elapsed\n", end-start);
	
	return 0;
}

