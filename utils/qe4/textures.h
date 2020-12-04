

typedef struct
{
	char	name[32];
	float	shift[2];
	float	rotate;
	float	scale[2];
	int		contents;
	int		flags;
	int		value;
} texdef_t;


typedef struct
{
	int			width, height;
	int			originy;
	texdef_t	texdef;
} texturewin_t;

typedef struct qtexture_s
{
	struct	qtexture_s *next;
	char	name[64];		// includes partial directory and extension
    int		width,  height;
	int		contents;
	int		flags;
	int		value;
	int		texture_number;	// gl bind number
	vec3_t	color;			// for flat shade mode
	qboolean	inuse;		// true = is present on the level
} qtexture_t;


// a texturename of the form (0 0 0) will
// create a solid color texture

void	Texture_Init (void);
void	Texture_Flush (void);
void	Texture_ClearInuse (void);
void	Texture_ShowInuse (void);
void	Texture_ShowDirectory (int menunum);

qtexture_t *Texture_ForName (char *name);

void	Texture_Init (void);
void	Texture_SetTexture (texdef_t *texdef);

void	Texture_SetMode(int iMenu);	// GL_TEXTURE_NEAREST, etc..
