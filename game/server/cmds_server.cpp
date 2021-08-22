/*
===================================================================================================

	Server commands

===================================================================================================
*/

#include "g_local.h"

/*
========================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
========================
*/
void ServerCommand()
{
	Com_Print( "No server commands exist yet\n" );
}
