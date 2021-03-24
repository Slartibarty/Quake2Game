
#include "cg_local.h"

cgame_import_t	cgi;
cgame_export_t	cge;

/*
===================
CG_Init

Called just after the DLL is loaded
===================
*/
void CG_Init()
{
	Com_Print( "==== CG_Init ====\n" );
}

/*
===================
CG_Shutdown

Called before the DLL is unloaded
===================
*/
void CG_Shutdown()
{
	Com_Print( "==== CG_Shutdown ====\n" );
}

extern void CL_AddTEnts();
extern void CL_AddParticles();
extern void CL_AddDLights();
extern void CL_AddLightStyles();

/*
===================
CG_AddEntities
===================
*/
void CG_AddEntities()
{
	CL_AddTEnts();
	CL_AddParticles();
	CL_AddDLights();
	CL_AddLightStyles();
}

extern void CL_RunDLights();
extern void CL_RunLightStyles();

/*
===================
CG_Frame
===================
*/
void CG_Frame()
{
	CL_RunDLights();
	CL_RunLightStyles();
}

/*
===================
CG_Frame
===================
*/
void CG_ClearState()
{
	CL_ClearEffects ();
	CL_ClearTEnts ();
}

/*
===================
CG_RunParticles
===================
*/
void CG_RunParticles( int &i, uint effects, centity_t *cent, entity_t *ent, entity_state_t *s1 )
{
	if (effects & EF_ROCKET)
	{
		CL_RocketTrail (cent->lerp_origin, ent->origin, cent);
		cgi.AddLight (ent->origin, 200, 1, 1, 0);
	}
	// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER. 
	// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
	else if (effects & EF_BLASTER)
	{
//		CL_BlasterTrail (cent->lerp_origin, ent->origin);
//PGM
		if (effects & EF_TRACKER)	// lame... problematic?
		{
			CL_BlasterTrail2 (cent->lerp_origin, ent->origin);
			cgi.AddLight (ent->origin, 200, 0, 1, 0);		
		}
		else
		{
			CL_BlasterTrail (cent->lerp_origin, ent->origin);
			cgi.AddLight (ent->origin, 200, 1, 1, 0);
		}
//PGM
	}
	else if (effects & EF_HYPERBLASTER)
	{
		if (effects & EF_TRACKER)						// PGM	overloaded for blaster2.
			cgi.AddLight (ent->origin, 200, 0, 1, 0);		// PGM
		else											// PGM
			cgi.AddLight (ent->origin, 200, 1, 1, 0);
	}
	else if (effects & EF_GIB)
	{
		CL_DiminishingTrail (cent->lerp_origin, ent->origin, cent, effects);
	}
	else if (effects & EF_GRENADE)
	{
		CL_DiminishingTrail (cent->lerp_origin, ent->origin, cent, effects);
	}
	else if (effects & EF_FLIES)
	{
		CL_FlyEffect (cent, ent->origin);
	}
	else if (effects & EF_BFG)
	{
		static int bfg_lightramp[6] = {300, 400, 600, 300, 150, 75};

		if (effects & EF_ANIM_ALLFAST)
		{
			CL_BfgParticles (ent);
			i = 200;
		}
		else
		{
			i = bfg_lightramp[s1->frame];
		}
		cgi.AddLight (ent->origin, (float)i, 0, 1, 0);
	}
	// RAFAEL
	else if (effects & EF_TRAP)
	{
		ent->origin[2] += 32;
		CL_TrapParticles (ent);
		i = (rand()%100) + 100;
		cgi.AddLight (ent->origin, (float)i, 1, 0.8f, 0.1f);
	}
	else if (effects & EF_FLAG1)
	{
		CL_FlagTrail (cent->lerp_origin, ent->origin, 242);
		cgi.AddLight (ent->origin, 225, 1, 0.1, 0.1);
	}
	else if (effects & EF_FLAG2)
	{
		CL_FlagTrail (cent->lerp_origin, ent->origin, 115);
		cgi.AddLight (ent->origin, 225, 0.1f, 0.1f, 1);
	}
//======
//ROGUE
	else if (effects & EF_TAGTRAIL)
	{
		CL_TagTrail (cent->lerp_origin, ent->origin, 220);
		cgi.AddLight (ent->origin, 225, 1.0f, 1.0f, 0.0f);
	}
	else if (effects & EF_TRACKERTRAIL)
	{
		if (effects & EF_TRACKER)
		{
			float intensity;

			intensity = 50 + (500 * (sinf(cgi.time()/500.0f) + 1.0f));
			cgi.AddLight (ent->origin, intensity, -1.0f, -1.0f, -1.0f);
		}
		else
		{
			CL_Tracker_Shell (cent->lerp_origin);
			cgi.AddLight (ent->origin, 155, -1.0f, -1.0f, -1.0f);
		}
	}
	else if (effects & EF_TRACKER)
	{
		CL_TrackerTrail (cent->lerp_origin, ent->origin, 0);
		cgi.AddLight (ent->origin, 200, -1, -1, -1);
	}
//ROGUE
//======
	// RAFAEL
	else if (effects & EF_GREENGIB)
	{
		CL_DiminishingTrail (cent->lerp_origin, ent->origin, cent, effects);				
	}
	// RAFAEL
	else if (effects & EF_IONRIPPER)
	{
		CL_IonripperTrail (cent->lerp_origin, ent->origin);
		cgi.AddLight (ent->origin, 100, 1, 0.5, 0.5);
	}
	// RAFAEL
	else if (effects & EF_BLUEHYPERBLASTER)
	{
		cgi.AddLight (ent->origin, 200, 0, 0, 1);
	}
	// RAFAEL
	else if (effects & EF_PLASMA)
	{
		if (effects & EF_ANIM_ALLFAST)
		{
			CL_BlasterTrail (cent->lerp_origin, ent->origin);
		}
		cgi.AddLight (ent->origin, 130, 1, 0.5, 0.5);
	}
}

extern void CL_SetLightstyle( int index, const char *s );

/*
===================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
===================
*/
cgame_export_t *GetCGameAPI( cgame_import_t *import )
{
	cgi = *import;

	cge.apiversion = CGAME_API_VERSION;
	cge.Init = CG_Init;
	cge.Shutdown = CG_Shutdown;

	cge.RegisterTEntSounds = CL_RegisterTEntSounds;
	cge.RegisterTEntModels = CL_RegisterTEntModels;

	cge.AddEntities = CG_AddEntities;
	cge.Frame = CG_Frame;
	cge.ClearState = CG_ClearState;

	cge.EntityEvent = CL_EntityEvent;
	cge.TeleporterParticles = CL_TeleporterParticles;
	cge.RunParticles = CG_RunParticles;

	cge.ParseTEnt = CL_ParseTEnt;
	cge.ParseMuzzleFlash = CL_ParseMuzzleFlash;
	cge.ParseMuzzleFlash2 = CL_ParseMuzzleFlash2;

	cge.SetLightstyle = CL_SetLightstyle;

	cge.Pmove = PM_Simulate;

	return &cge;
}

#ifndef CGAME_HARD_LINKED

void Com_Print( const char *msg )
{
	cgi.Print( msg );
}

void Com_Printf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );
}

void Com_DPrint( const char *msg )
{
	cgi.DPrint( msg );
}

void Com_DPrintf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_DPrint( msg );
}

[[noreturn]]
void Com_Error( const char *msg )
{
	cgi.Error( msg );
}

[[noreturn]]
void Com_Errorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Error( msg );
}

[[noreturn]]
void Com_FatalError( const char *msg )
{
	cgi.FatalError( msg );
}

[[noreturn]]
void Com_FatalErrorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_FatalError( msg );
}

#endif
