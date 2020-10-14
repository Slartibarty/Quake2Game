// cl_inv.c -- client inventory screen

#include "client.h"

/*
================
CL_ParseInventory
================
*/
void CL_ParseInventory (void)
{
	int		i;

	for (i=0 ; i<MAX_ITEMS ; i++)
		cl.inventory[i] = MSG_ReadShort (&net_message);
}


/*
================
Inv_DrawString
================
*/
static void Inv_DrawString (int x, int y, const char *string)
{
	while (*string)
	{
		re.DrawChar (x, y, *string);
		x+=8;
		string++;
	}
}

static void SetStringHighBit (char *s)
{
	while (*s)
		*s++ |= 128;
}

/*
================
CL_DrawInventory
================
*/
#define	DISPLAY_ITEMS	17

void CL_DrawInventory (void)
{
	int			i, j;
	int			num, selected_num, item;
	int			index[MAX_ITEMS];
	char		string[1024];
	int			x, y;
	char		binding[1024];
	const char	*bind;
	int			selected;
	int			top;

	selected = cl.frame.playerstate.stats[STAT_SELECTED_ITEM];

	num = 0;
	selected_num = 0;
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		if (i==selected)
			selected_num = num;
		if (cl.inventory[i])
		{
			index[num] = i;
			num++;
		}
	}

	// determine scroll point
	top = selected_num - DISPLAY_ITEMS/2;
	if (num - top < DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (viddef.width-256)/2;
	y = (viddef.height-240)/2;

	// repaint everything next frame
	SCR_DirtyScreen ();

	re.DrawPic (x, y+8, "inventory");

	y += 24;
	x += 24;
	Inv_DrawString (x, y, "hotkey ### item");
	Inv_DrawString (x, y+8, "------ --- ----");
	y += 16;
	for (i=top ; i<num && i < top+DISPLAY_ITEMS ; i++)
	{
		item = index[i];
		// search for a binding
		Com_sprintf (binding, "use %s", cl.configstrings[CS_ITEMS+item]);
		bind = "";
		for (j=0 ; j<256 ; j++)
			if (keybindings[j] && !Q_stricmp (keybindings[j], binding))
			{
				bind = Key_KeynumToString(j);
				break;
			}

		Com_sprintf (string, "%6s %3i %s", bind, cl.inventory[item],
			cl.configstrings[CS_ITEMS+item] );
		if (item != selected)
			SetStringHighBit (string);
		else	// draw a blinky cursor by the selected item
		{
			if ( (int)(cls.realtime*10) & 1)
				re.DrawChar (x-8, y, 15);
		}
		Inv_DrawString (x, y, string);
		y += 8;
	}

}
