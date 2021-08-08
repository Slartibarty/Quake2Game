
#pragma once

enum cvarFlags_t : uint32
{
	CVAR_ARCHIVE		= 1,	// will be written to config.cfg
	CVAR_USERINFO		= 2,	// added to userinfo when changed
	CVAR_SERVERINFO		= 4,	// added to serverinfo when changed
	CVAR_INIT			= 8,	// can only be set from the command-line
	CVAR_LATCH			= 16,	// save changes until server restart
	CVAR_CHEAT			= 32,	// variable is considered a cheat
	CVAR_MODIFIED		= 64	// set when the variable is modified
};

// nothing outside the Cvar_*() functions should modify these fields!
struct cvar_t
{
	lab::string		name;
	lab::string		value;
	lab::string		help;				// null if no help
	lab::string		latchedValue;		// null if no latched value
	uint32			flags;
	float			fltValue;
	int				intValue;
	cvar_t *		pNext;

	const char *	GetName() const		{ return name.c_str(); }
	const char *	GetHelp() const		{ return help.c_str(); }

	bool			IsModified() const	{ return ( flags & CVAR_MODIFIED ) != 0; }
	void			SetModified()		{ flags |= CVAR_MODIFIED; }
	void			ClearModified()		{ flags &= ~CVAR_MODIFIED; }

	const char *	GetString() const	{ return value.c_str(); }
	float			GetFloat() const	{ return fltValue; }
	int				GetInt() const		{ return intValue; }
	bool			GetBool() const		{ return intValue != 0; }

	uint32			GetFlags() const	{ return flags; }
};
