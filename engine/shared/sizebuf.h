//=================================================================================================
// Size buffers
//=================================================================================================

#pragma once

#include "../../common/q_types.h"

struct sizebuf_t
{
	byte	*data;
	int		maxsize;
	int		cursize;
	int		readcount;
	bool	allowoverflow;	// if false, do a Com_Error
	bool	overflowed;		// set to true if the buffer size failed
};

void SZ_Init( sizebuf_t *buf, byte *data, int length );
void SZ_Clear( sizebuf_t *buf );
void *SZ_GetSpace( sizebuf_t *buf, int length );
void SZ_Write( sizebuf_t *buf, const void *data, int length );
void SZ_Print( sizebuf_t *buf, const char *data );	// strcats onto the sizebuf
