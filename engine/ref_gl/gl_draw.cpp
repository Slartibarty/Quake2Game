//-------------------------------------------------------------------------------------------------
// Functions related to drawing 2D imagery
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"

static image_t *draw_chars;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_InitLocal (void)
{
	// load console characters (don't bilerp characters)
	draw_chars = GL_FindImage("pics/conchars.pcx", it_pic);
	if (!draw_chars) {
		RI_Sys_Error(ERR_FATAL, "Could not get console font: pics/conchars");
	}

	// Undo this if we ever make pics linear filtered again
//	GL_Bind(draw_chars->texnum);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

//-------------------------------------------------------------------------------------------------
// Draws one 8*8 graphics character with 0 being transparent.
// It can be clipped to the top of the screen to allow the console to be
// smoothly scrolled off.
//-------------------------------------------------------------------------------------------------
void Draw_Char (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;		// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	GL_Bind(draw_chars->texnum);

	glBegin(GL_QUADS);
	glTexCoord2f(fcol, frow);
	glVertex2i(x, y);
	glTexCoord2f(fcol + size, frow);
	glVertex2i(x + 8, y);
	glTexCoord2f(fcol + size, frow + size);
	glVertex2i(x + 8, y + 8);
	glTexCoord2f(fcol, frow + size);
	glVertex2i(x, y + 8);
	glEnd();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
image_t	*Draw_FindPic (const char *name)
{
	image_t *gl;

	if (name[0] != '/' && name[0] != '\\')
	{
		char fullname[MAX_QPATH];
		Q_sprintf_s(fullname, "pics/%s.pcx", name);
		gl = GL_FindImage(fullname, it_pic);
	}
	else
		gl = GL_FindImage(name + 1, it_pic);

	return gl;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_GetPicSize (int *w, int *h, const char *pic)
{
	image_t *gl;

	gl = Draw_FindPic(pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_StretchPic (int x, int y, int w, int h, const char *pic)
{
	image_t *gl;

	gl = Draw_FindPic(pic);
	if (!gl)
	{
		RI_Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL_Bind(gl->texnum);

	// SlartTodo: This is called every single frame a stretchpic needs to be drawn,
	// surely this must be a bad thing
	// Linear filter stretched pics, otherwise they look awful
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);
	glTexCoord2f(gl->sl, gl->tl);
	glVertex2i(x, y);
	glTexCoord2f(gl->sh, gl->tl);
	glVertex2i(x + w, y);
	glTexCoord2f(gl->sh, gl->th);
	glVertex2i(x + w, y + h);
	glTexCoord2f(gl->sl, gl->th);
	glVertex2i(x, y + h);
	glEnd();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_Pic (int x, int y, const char *pic)
{
	image_t *gl;

	gl = Draw_FindPic(pic);
	if (!gl)
	{
		RI_Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL_Bind(gl->texnum);
	glBegin(GL_QUADS);
	glTexCoord2f(gl->sl, gl->tl);
	glVertex2i(x, y);
	glTexCoord2f(gl->sh, gl->tl);
	glVertex2i(x + gl->width, y);
	glTexCoord2f(gl->sh, gl->th);
	glVertex2i(x + gl->width, y + gl->height);
	glTexCoord2f(gl->sl, gl->th);
	glVertex2i(x, y + gl->height);
	glEnd();
}

//-------------------------------------------------------------------------------------------------
// This repeats a 64*64 tile graphic to fill the screen around a sized down
// refresh window.
//-------------------------------------------------------------------------------------------------
void Draw_TileClear (int x, int y, int w, int h, const char *pic)
{
	image_t *image;

	image = Draw_FindPic(pic);
	if (!image)
	{
		RI_Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL_Bind(image->texnum);
	glBegin(GL_QUADS);
	glTexCoord2f(x / 64.0f, y / 64.0f);
	glVertex2i(x, y);
	glTexCoord2f((x + w) / 64.0f, y / 64.0f);
	glVertex2i(x + w, y);
	glTexCoord2f((x + w) / 64.0f, (y + h) / 64.0f);
	glVertex2i(x + w, y + h);
	glTexCoord2f(x / 64.0f, (y + h) / 64.0f);
	glVertex2i(x, y + h);
	glEnd();
}


//-------------------------------------------------------------------------------------------------
// Fills a box of pixels with a single color
//-------------------------------------------------------------------------------------------------
void Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		uint	c;
		byte	v[4];
	} color;

	if (c > 255)
		RI_Sys_Error(ERR_FATAL, "Draw_Fill: bad color");

	glDisable(GL_TEXTURE_2D);

	color.c = d_8to24table[c];
	glColor3f(color.v[0] / 255.0f,
		color.v[1] / 255.0f,
		color.v[2] / 255.0f);

	glBegin(GL_QUADS);

	glVertex2i(x, y);
	glVertex2i(x + w, y);
	glVertex2i(x + w, y + h);
	glVertex2i(x, y + h);

	glEnd();
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Draw_FadeScreen (void)
{
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
	glBegin(GL_QUADS);

	glVertex2i(0, 0);
	glVertex2i(r_newrefdef.width, 0);
	glVertex2i(r_newrefdef.width, r_newrefdef.height);
	glVertex2i(0, r_newrefdef.height);

	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

//-------------------------------------------------------------------------------------------------

extern unsigned	r_rawpalette[256];

//-------------------------------------------------------------------------------------------------
// Used for rendering cinematic frames
//-------------------------------------------------------------------------------------------------
void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{
	uint		image32[256 * 256];
	int			i, j, trows;
	byte*		source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	if (rows <= 256)
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows / 256.0f;
		trows = 256;
	}
	t = rows * hscale / 256;

	{
		uint* dest;

		for (i = 0; i < trows; i++)
		{
			row = (int)(i * hscale);
			if (row > rows)
				break;
			source = data + cols * row;
			dest = &image32[i * 256];
			fracstep = cols * 0x10000 / 256;
			frac = fracstep >> 1;
			for (j = 0; j < 256; j++)
			{
				dest[j] = r_rawpalette[source[frac >> 16]];
				frac += fracstep;
			}
		}
	}

	GLuint id;

	glGenTextures(1, &id);
	GL_Bind(id);

	// Upload frame
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(x, y);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2i(x + w, y);
	glTexCoord2f(1.0f, t);
	glVertex2i(x + w, y + h);
	glTexCoord2f(0.0f, t);
	glVertex2i(x, y + h);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);

	// Delete frame
	glDeleteTextures(1, &id);
}
