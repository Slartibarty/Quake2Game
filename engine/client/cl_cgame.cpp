// cl_cgame.c -- client system interaction with client game

#include "cl_local.h"

cgame_export_t *cge;

static centity_t *Wrap_GetEntityAtIndex( int index )
{
	return &cl_entities[index];
}

static entity_state_t *Wrap_GetParseEntityAtIndex( int index )
{
	return &cl_parse_entities[index];
}

static cmodel_t *Wrap_GetCModelAtIndex( int index )
{
	return cl.model_clip[index];
}

static int Wrap_time()
{
	return cl.time;
}

static int Wrap_servertime()
{
	return cl.frame.servertime;
}

static int Wrap_serverframe()
{
	return cl.frame.serverframe;
}

static int Wrap_frametime()
{
	return cls.frametime;
}

static float Wrap_lerpfrac()
{
	return cl.lerpfrac;
}

static int Wrap_playernum()
{
	return cl.playernum;
}

static player_state_t *Wrap_playerstate()
{
	return &cl.frame.playerstate;
}

static vec3_t *Wrap_vieworg()
{
	return &cl.refdef.vieworg;
}

static vec3_t *Wrap_v_forward()
{
	return &cl.v_forward;
}

static vec3_t *Wrap_v_right()
{
	return &cl.v_right;
}

static vec3_t *Wrap_v_up()
{
	return &cl.v_up;
}

static const char **Wrap_configstrings()
{
	return (const char **)cl.configstrings;
}

void CL_ShutdownCGame()
{
	if ( !cge ) {
		return;
	}
	cge->Shutdown();
	Sys_UnloadCGame();
	cge = nullptr;
}

void CL_InitCGame()
{
	cgame_import_t cgi;

	if ( cge ) {
		CL_ShutdownCGame();
	}

	cgi.Print = Com_Print;
	cgi.DPrint = Com_DPrint;
	cgi.Error = Com_Error;
	cgi.FatalError = Com_FatalError;

	cgi.StartSound = S_StartSound;

	cgi.RegisterSound = S_RegisterSound;
	cgi.RegisterPic = R_RegisterPic;
	cgi.RegisterModel = R_RegisterModel;

	cgi.AddEntity = V_AddEntity;
	cgi.AddParticle = V_AddParticle;
	cgi.AddLight = V_AddDLight;
	cgi.AddLightStyle = V_AddLightStyle;

	cgi.ReadByte = MSG_ReadByte;
	cgi.ReadShort = MSG_ReadShort;
	cgi.ReadLong = MSG_ReadLong;
	cgi.ReadPos = MSG_ReadPos;
	cgi.ReadDir = MSG_ReadDir;

	cgi.GetEntityAtIndex = Wrap_GetEntityAtIndex;

	cgi.time = Wrap_time;
	cgi.servertime = Wrap_servertime;
	cgi.serverframe = Wrap_serverframe;
	cgi.frametime = Wrap_frametime;
	cgi.lerpfrac = Wrap_lerpfrac;
	cgi.playernum = Wrap_playernum;
	cgi.playerstate = Wrap_playerstate;
	cgi.vieworg = Wrap_vieworg;
	cgi.v_forward = Wrap_v_forward;
	cgi.v_right = Wrap_v_right;
	cgi.v_up = Wrap_v_up;

	cgi.bytedirs = bytedirs;
	cgi.net_message = &net_message;

	cge = (cgame_export_t *)Sys_GetCGameAPI( &cgi );

	if ( !cge )
	{
		Com_Error( "failed to load cgame DLL" );
	}

	if ( cge->apiversion != CGAME_API_VERSION )
	{
		Com_Errorf( "cgame is version %d, not %d",
			cge->apiversion, CGAME_API_VERSION );
	}

	cge->Init();
}
