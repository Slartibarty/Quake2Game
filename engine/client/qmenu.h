
#pragma once

#define MAXMENUITEMS	64

#define MTYPE_SLIDER		0
#define MTYPE_LIST			1
#define MTYPE_ACTION		2
#define MTYPE_SPINCONTROL	3
#define MTYPE_SEPARATOR  	4
#define MTYPE_FIELD			5

#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32

// normal keys should be passed as lowercased ascii

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131

#define QMF_LEFT_JUSTIFY	0x00000001
#define QMF_GRAYED			0x00000002
#define QMF_NUMBERSONLY		0x00000004

typedef struct _tag_menuframework
{
	int x, y;
	int	cursor;

	int	nitems;
	int nslots;
	void *items[64];

	const char *statusbar;

	void (*cursordraw)( struct _tag_menuframework *m );
	
} menuframework_s;

typedef struct
{
	int type;
	const char *name;
	int x, y;
	menuframework_s *parent;
	int cursor_offset;
	int	localdata[4];
	unsigned flags;

	const char *statusbar;

	void (*callback)( void *self );
	void (*statusbarfunc)( void *self );
	void (*ownerdraw)( void *self );
	void (*cursordraw)( void *self );
} menucommon_s;

typedef struct
{
	menucommon_s generic;

	char		buffer[80];
	int			cursor;
	int			length;
	int			visible_length;
	int			visible_offset;
} menufield_s;

typedef struct 
{
	menucommon_s generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_s;

typedef struct
{
	menucommon_s generic;

	int curvalue;

	const char **itemnames;
} menulist_s;

typedef struct
{
	menucommon_s generic;
} menuaction_s;

typedef struct
{
	menucommon_s generic;
} menuseparator_s;

qboolean Field_Key( menufield_s *field, int key );

void	Menu_AddItem( menuframework_s *menu, void *item );
void	Menu_AdjustCursor( menuframework_s *menu, int dir );
void	Menu_Center( menuframework_s *menu );
void	Menu_Draw( menuframework_s *menu );
void	*Menu_ItemAtCursor( menuframework_s *m );
qboolean Menu_SelectItem( menuframework_s *s );
void	Menu_SetStatusBar( menuframework_s *s, const char *string );
void	Menu_SlideItem( menuframework_s *s, int dir );
int		Menu_TallySlots( menuframework_s *menu );

void	 Menu_DrawString( int, int, const char * );
void	 Menu_DrawStringDark( int, int, const char * );
void	 Menu_DrawStringR2L( int, int, const char * );
void	 Menu_DrawStringR2LDark( int, int, const char * );
