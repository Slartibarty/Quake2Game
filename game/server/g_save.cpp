
#include "g_local.h"

/*
========================
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
========================
*/
void WriteGame( char *filename, qboolean autosave )
{
}

/*
========================
ReadGame
========================
*/
void ReadGame( char *filename )
{
}

//=================================================================================================

/*
========================
WriteLevel
========================
*/
void WriteLevel( char *filename )
{
}

/*
========================
ReadLevel

SpawnEntities will allready have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
========================
*/
void ReadLevel( char *filename )
{
}
