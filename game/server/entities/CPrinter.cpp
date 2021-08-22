
#include "g_local.h"

class CPrinter : public CBaseEntity
{
private:
	DECLARE_CLASS( CPrinter, CBaseEntity )

	int m_printCount = 0;

public:
	CPrinter() = default;
	~CPrinter() override = default;

	void Spawn() override
	{
		Com_Print( "Hello, a printer just spawned!\n" );
	}

	void Think() override
	{
		Com_Printf( "Hey, a printer is thinking, this is iteration: %d!\n", m_printCount );

		++m_printCount;
	}
};

LINK_ENTITY_TO_CLASS( point_printer, CPrinter )
