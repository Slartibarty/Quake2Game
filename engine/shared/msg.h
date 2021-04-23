//=================================================================================================
// Message IO functions
//
// Handles byte ordering and avoids alignment errors
//=================================================================================================

#pragma once

#include "../../core/sys_types.h"
#include "../../core/math.h"
#include "sizebuf.h"

// q_shared
struct usercmd_t;
struct entity_state_t;

//
// Writing
//

void	MSG_WriteChar( sizebuf_t *sb, int c );
void	MSG_WriteByte( sizebuf_t *sb, int c );
void	MSG_WriteShort( sizebuf_t *sb, int c );
void	MSG_WriteLong( sizebuf_t *sb, int c );
void	MSG_WriteFloat( sizebuf_t *sb, float f );
void	MSG_WriteString( sizebuf_t *sb, const char *s );
void	MSG_WriteCoord( sizebuf_t *sb, float f );
void	MSG_WritePos( sizebuf_t *sb, vec3_t pos );
void	MSG_WriteAngle( sizebuf_t *sb, float f );
void	MSG_WriteAngle16( sizebuf_t *sb, float f );
void	MSG_WriteDeltaUsercmd( sizebuf_t *sb, usercmd_t *from, usercmd_t *cmd );
void	MSG_WriteDeltaEntity( entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force, qboolean newentity );
void	MSG_WriteDir( sizebuf_t *sb, vec3_t vector );

//
// Reading
//

void	MSG_BeginReading( sizebuf_t *sb );

int		MSG_ReadChar( sizebuf_t *sb );
int		MSG_ReadByte( sizebuf_t *sb );
int		MSG_ReadShort( sizebuf_t *sb );
int		MSG_ReadLong( sizebuf_t *sb );
float	MSG_ReadFloat( sizebuf_t *sb );
char	*MSG_ReadString( sizebuf_t *sb );
char	*MSG_ReadStringLine( sizebuf_t *sb );

float	MSG_ReadCoord( sizebuf_t *sb );
void	MSG_ReadPos( sizebuf_t *sb, vec3_t pos );
float	MSG_ReadAngle( sizebuf_t *sb );
float	MSG_ReadAngle16( sizebuf_t *sb );
void	MSG_ReadDeltaUsercmd( sizebuf_t *sb, usercmd_t *from, usercmd_t *cmd );

void	MSG_ReadDir( sizebuf_t *sb, vec3_t vector );

void	MSG_ReadData( sizebuf_t *sb, void *buffer, int size );