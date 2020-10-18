//=================================================================================================
// Server commands
//=================================================================================================

#include "ng_local.h"

//-------------------------------------------------------------------------------------------------
// ServerCommand will be called when an "sv" command is issued.
// The game can issue gi.argc() / gi.argv() commands to get the rest
// of the parameters
//-------------------------------------------------------------------------------------------------
void ServerCommand()
{
	gi.cprintf(NULL, PRINT_HIGH, "Wanted to use a server command, but not implemented!\n");
}
