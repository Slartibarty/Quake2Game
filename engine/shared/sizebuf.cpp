//=================================================================================================
// Size buffers
//=================================================================================================

#include "engine.h"

#include "sizebuf.h"

void SZ_Init( sizebuf_t *buf, byte *data, int length )
{
	memset( buf, 0, sizeof( *buf ) );
	buf->data = data;
	buf->maxsize = length;
}

void SZ_Clear( sizebuf_t *buf )
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void *SZ_GetSpace( sizebuf_t *buf, int length )
{
	void *data;

	if ( buf->cursize + length > buf->maxsize )
	{
		if ( !buf->allowoverflow )
			Com_FatalError("SZ_GetSpace: overflow without allowoverflow set" );

		if ( length > buf->maxsize )
			Com_FatalErrorf("SZ_GetSpace: %i is > full buffer size", length );

		Com_Print( "SZ_GetSpace: overflow\n" );
		SZ_Clear( buf );
		buf->overflowed = true;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write( sizebuf_t *buf, const void *data, int length )
{
	memcpy( SZ_GetSpace( buf, length ), data, length );
}

void SZ_Print( sizebuf_t *buf, const char *data )
{
	int		len;

	len = (int)( strlen( data ) + 1 );

	if ( buf->cursize )
	{
		if ( buf->data[buf->cursize - 1] )
			memcpy( (byte *)SZ_GetSpace( buf, len ), data, len ); // no trailing 0
		else
			memcpy( (byte *)SZ_GetSpace( buf, len - 1 ) - 1, data, len ); // write over trailing 0
	}
	else
	{
		memcpy( (byte *)SZ_GetSpace( buf, len ), data, len );
	}
}
