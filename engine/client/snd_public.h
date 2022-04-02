// Interfaces to the sound API

#pragma once

struct sfx_t;

void		S_Init();
void		S_Shutdown();

// if origin is NULL, the sound will be dynamically sourced from the entity
void		S_StartSound( vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float timeofs );
void		S_StartLocalSound( const char *s, float volume = 1.0f );

// Play raw sound data (used by cinematics and VOIP)
void		S_RawSamples( int samples, int rate, int width, int channels, byte *data );

void		S_StopAllSounds();
void		S_Update( vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up );

void		S_Activate( bool active );

// Registration
void		S_BeginRegistration();
sfx_t *		S_RegisterSound( const char *sample );
void		S_EndRegistration();

// Find a sound, create it only if create is true
sfx_t *		S_FindName( const char *name, bool create );

// The sound code makes callbacks to the client for entity position
// information, so entities can be dynamically re-spatialized
void		CL_GetEntitySoundOrigin( int ent, vec3_t org );
