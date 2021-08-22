
#include "g_local.h"

static const char *g_legacy_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
"	xv	262 "
"	num	2	10 "
"	xv	296 "
"	pic	9 "
"endif "

//  help / weapon icon 
"if 11 "
"	xv	148 "
"	pic	11 "
"endif "
;

class CWorld : public CBaseNetworkable
{
private:
	DECLARE_CLASS( CWorld, CBaseNetworkable )

public:
	CWorld() = default;
	~CWorld() override = default;

	void Spawn() override
	{
		m_moveType = MOVETYPE_PUSH;
		m_edict.solid = SOLID_BSP;
		m_edict.inuse = true;			// since the world doesn't use G_Spawn()
		m_edict.s.modelindex = 1;		// world model is always index 1

		//---------------

		if ( st.nextmap )
			strcpy( level.nextmap, st.nextmap );

		// make some data visible to the server

		if (st.sky && st.sky[0])
			gi.configstring (CS_SKY, st.sky);
		else
			gi.configstring (CS_SKY, "unit1_");

		gi.configstring (CS_SKYROTATE, va("%f", st.skyrotate) );

		gi.configstring (CS_SKYAXIS, va("%f %f %f",
			st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]) );

		gi.configstring (CS_CDTRACK, "0" );

		gi.configstring (CS_MAXCLIENTS, va("%i", maxclients->GetInt()));

		// status bar program
		gi.configstring (CS_STATUSBAR, g_legacy_statusbar);

		//---------------


		// help icon for statusbar
		gi.imageindex ("i_help");
		level.pic_health = gi.imageindex ("i_health");
		gi.imageindex ("help");
		gi.imageindex ("field_3");

		if (!st.gravity)
			gi.cvar_set("sv_gravity", "800");
		else
			gi.cvar_set("sv_gravity", st.gravity);

		gi.soundindex ("player/lava1.wav");
		gi.soundindex ("player/lava2.wav");

		gi.soundindex ("misc/pc_up.wav");
		gi.soundindex ("misc/talk1.wav");

		gi.soundindex ("misc/udeath.wav");

		// gibs
		gi.soundindex ("items/respawn1.wav");

		// sexed sounds
		gi.soundindex ("*death1.wav");
		gi.soundindex ("*death2.wav");
		gi.soundindex ("*death3.wav");
		gi.soundindex ("*death4.wav");
		gi.soundindex ("*fall1.wav");
		gi.soundindex ("*fall2.wav");	
		gi.soundindex ("*gurp1.wav");		// drowning damage
		gi.soundindex ("*gurp2.wav");	
		gi.soundindex ("*jump1.wav");		// player jump
		gi.soundindex ("*pain25_1.wav");
		gi.soundindex ("*pain25_2.wav");
		gi.soundindex ("*pain50_1.wav");
		gi.soundindex ("*pain50_2.wav");
		gi.soundindex ("*pain75_1.wav");
		gi.soundindex ("*pain75_2.wav");
		gi.soundindex ("*pain100_1.wav");
		gi.soundindex ("*pain100_2.wav");

		// sexed models
		// THIS ORDER MUST MATCH THE DEFINES IN g_local.h
		// you can add more, max 15
		gi.modelindex ("#w_blaster.md2");
		gi.modelindex ("#w_shotgun.md2");
		gi.modelindex ("#w_sshotgun.md2");
		gi.modelindex ("#w_machinegun.md2");
		gi.modelindex ("#w_chaingun.md2");
		gi.modelindex ("#a_grenades.md2");
		gi.modelindex ("#w_glauncher.md2");
		gi.modelindex ("#w_rlauncher.md2");
		gi.modelindex ("#w_hyperblaster.md2");
		gi.modelindex ("#w_railgun.md2");
		gi.modelindex ("#w_bfg.md2");

		//-------------------

		gi.soundindex ("player/gasp1.wav");		// gasping for air
		gi.soundindex ("player/gasp2.wav");		// head breaking surface, not gasping

		gi.soundindex ("player/watr_in.wav");	// feet hitting water
		gi.soundindex ("player/watr_out.wav");	// feet leaving water

		gi.soundindex ("player/watr_un.wav");	// head going underwater
	
		gi.soundindex ("player/u_breath1.wav");
		gi.soundindex ("player/u_breath2.wav");

		gi.soundindex ("items/pkup.wav");		// bonus item pickup
		gi.soundindex ("world/land.wav");		// landing thud
		gi.soundindex ("misc/h2ohit1.wav");		// landing splash

		gi.soundindex ("items/damage.wav");
		gi.soundindex ("items/protect.wav");
		gi.soundindex ("items/protect4.wav");
		gi.soundindex ("weapons/noammo.wav");

		gi.soundindex ("infantry/inflies1.wav");

		gi.modelindex ("models/objects/gibs/arm/tris.md2");
		gi.modelindex ("models/objects/gibs/bone/tris.md2");
		gi.modelindex ("models/objects/gibs/bone2/tris.md2");
		gi.modelindex ("models/objects/gibs/chest/tris.md2");
		gi.modelindex ("models/objects/gibs/skull/tris.md2");
		gi.modelindex ("models/objects/gibs/head2/tris.md2");

		//
		// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
		//

		// 0 normal
		gi.configstring(CS_LIGHTS+0, "m");
	
		// 1 FLICKER (first variety)
		gi.configstring(CS_LIGHTS+1, "mmnmmommommnonmmonqnmmo");
	
		// 2 SLOW STRONG PULSE
		gi.configstring(CS_LIGHTS+2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	
		// 3 CANDLE (first variety)
		gi.configstring(CS_LIGHTS+3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	
		// 4 FAST STROBE
		gi.configstring(CS_LIGHTS+4, "mamamamamama");
	
		// 5 GENTLE PULSE 1
		gi.configstring(CS_LIGHTS+5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	
		// 6 FLICKER (second variety)
		gi.configstring(CS_LIGHTS+6, "nmonqnmomnmomomno");
	
		// 7 CANDLE (second variety)
		gi.configstring(CS_LIGHTS+7, "mmmaaaabcdefgmmmmaaaammmaamm");
	
		// 8 CANDLE (third variety)
		gi.configstring(CS_LIGHTS+8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	
		// 9 SLOW STROBE (fourth variety)
		gi.configstring(CS_LIGHTS+9, "aaaaaaaazzzzzzzz");
	
		// 10 FLUORESCENT FLICKER
		gi.configstring(CS_LIGHTS+10, "mmamammmmammamamaaamammma");

		// 11 SLOW PULSE NOT FADE TO BLACK
		gi.configstring(CS_LIGHTS+11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	
		// styles 32-62 are assigned by the light program for switchable lights

		// 63 testing
		gi.configstring(CS_LIGHTS+63, "a");

		//
		// reserve clients
		//

		for ( int i = 0; i < game.maxclients; ++i )
		{
			CBaseEntity *pEntity = Ent_CreateEntityByName( "player" );
		}

	}

	void Think() override
	{
	}
};

LINK_ENTITY_TO_CLASS( worldspawn, CWorld )
