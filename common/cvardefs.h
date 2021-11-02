
#pragma once

struct cvar_t;

typedef void ( *changeCallback_t )( cvar_t *pVar, const char *pOldString, float oldFloat, int oldInt );

enum cvarFlags_t : uint32
{
	CVAR_ARCHIVE		= BIT(0),	// will be written to config.cfg
	CVAR_USERINFO		= BIT(1),	// added to userinfo when changed
	CVAR_SERVERINFO		= BIT(2),	// added to serverinfo when changed
	CVAR_INIT			= BIT(3),	// can only be set from the command-line
	CVAR_LATCH			= BIT(4),	// save changes until server restart
	CVAR_CHEAT			= BIT(5),	// variable is considered a cheat
	CVAR_MODIFIED		= BIT(6),	// set when the variable is modified
	CVAR_STATIC			= BIT(7)	// this cvar is static (do not free)
};

// nothing outside the Cvar_*() functions should modify these fields!
// minVal and maxVal are strings so we can call .assign with them
struct cvar_t
{
//private:

	lab::string			name;
	lab::string			value;
	lab::string			help;				// null if no help
	lab::string			latchedValue;		// null if no latched value
	const char			*minVal, *maxVal;	// TODO: When the game DLL closes and restarts, these will be invalid ptrs!
	uint32				flags;
	float				fltValue;
	int					intValue;
	changeCallback_t	pCallback;
	cvar_t *			pNext;

//public:

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

#define CVAR_CALLBACK(name) \
static void name( cvar_t *var, const char *oldString, float oldFloat, int oldInt )
