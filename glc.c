/* mGLC library version 0.3
 * Copyright (C) 2000 Paul Brannan
 * This library is released under the GNU LGPL.  For more information, please
 * see the file LICENSE, or visit the GNU web page at www.gnu.org.
 */

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include "glc.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !FALSE
#endif

#define MAX_GLC_CONTEXTS 256
#define MAX_GLC_FONTS 16

#define GLC_EXTENSIONS_STRING ""
#define GLC_RELEASE_STRING "0.2 mGLC 0.4"
#define GLC_VENDOR_STRING "Paul Brannan"
#define GLC_VERSION_MAJOR_NUMBER 0
#define GLC_VERSION_MINOR_NUMBER 2

enum GLCStyles {
	GLC_STYLE_NORMAL,
	GLC_STYLE_BOLD,
	GLC_STYLE_ITALIC,
	GLC_STYLE_BOLD_ITALIC,
	NUM_GLC_STYLES
};
#define GLC_NOT_A_FONT NUM_GLC_STYLES

typedef struct {
	GLboolean isFont;
	GLCchar *name;
	GLCchar *xfontprefix, *xfontsuffix;
	Font xfont;
	XFontStruct *xfontinfo;
	GLboolean can_transform;
} GLCfont;

typedef struct {
	GLboolean isContext;
	Display *display;
	GLint font_list[MAX_GLC_FONTS+2];
	GLCfont fonts[MAX_GLC_FONTS+1][NUM_GLC_STYLES];
	GLint font_faces[MAX_GLC_FONTS+1];
	GLint num_fonts;
	GLfloat bitmap_matrix[4];
	GLfloat bitmap_scale[2];
	GLvoid* data_pointer;
	GLint current_font_list[MAX_GLC_FONTS];
	GLint num_current_fonts;
	GLint list_base;
	GLboolean reload_font;
	GLboolean scale_change;
} GLCcontext;

static int glcContextList[MAX_GLC_CONTEXTS+2];
static GLCcontext glcContexts[MAX_GLC_CONTEXTS+1];
static int numGlcContexts = 0;
static int currentGlcContext = 0;
static GLboolean isFirstCall = TRUE;
static GLCenum last_error = GLC_NONE;
static GLCcontext *g = NULL;

static const GLfloat identity_matrix_2_by_2[] = {1, 0, 0, 1};

/* A hack for Mesa, since glXGetCurrentDisplay is not defined; this should
 * not affect any other systems, since the prototype should be the same for
 * all.
 * */
Display* glXGetCurrentDisplay(void);

/* --------------------------------------------------------------------------
 * Global Commands
 * --------------------------------------------------------------------------
 */

/* Create a new GLC context and return its ID */
GLint glcGenContext() {
	GLCcontext *c;
	int i, j;

	/* If this is the first call, initialize our data */
	if(numGlcContexts==0) {
		memset(glcContextList, 0, sizeof(glcContextList));
		memset(glcContexts, 0, sizeof(glcContexts));
	}
	isFirstCall = FALSE;

	/* Find context that is not in use */
	for(j = 1; j < MAX_GLC_CONTEXTS; j++)
		if(!glcContexts[j].isContext) break;
	if(j == MAX_GLC_CONTEXTS) return 0;

	/* Add this context to the list */
	glcContextList[numGlcContexts++] = j;
	glcContextList[numGlcContexts] = 0;

	/* Initialize this context */
	c = &glcContexts[j];
	c->isContext = TRUE;
	c->display = (Display*)glXGetCurrentDisplay();
	c->bitmap_matrix[0] = 1.0;
	c->bitmap_matrix[1] = 0.0;
	c->bitmap_matrix[2] = 0.0;
	c->bitmap_matrix[3] = 1.0;
	c->bitmap_scale[0] = 1.0;
	c->bitmap_scale[1] = 1.0;
	last_error = GLC_NONE;
	c->data_pointer = 0;
	c->num_fonts = 0;
	c->list_base = glGenLists(256);
	c->reload_font = TRUE;
	c->scale_change = TRUE;
	memcpy(c->bitmap_matrix, identity_matrix_2_by_2, sizeof(c->bitmap_matrix));

	/* Turn on synchronization.  This does not affear to affect performance,
	 * and seems to keep my X server from crashing.  Still, this should
	 * not be necessary, and an alternative solution would probably be
	 * preferable.
	 */
	XSynchronize(c->display, TRUE);

	/* Load a default font */
	memset(c->font_list, 0, sizeof(c->font_list));
	c->fonts[0][GLC_STYLE_NORMAL].xfontinfo = XLoadQueryFont(c->display, "variable");
	c->fonts[0][GLC_STYLE_NORMAL].xfont = c->fonts[0][GLC_STYLE_NORMAL].xfontinfo->fid;
	c->fonts[0][GLC_STYLE_NORMAL].name = strdup("base");
	c->fonts[0][GLC_STYLE_NORMAL].xfontprefix =
	c->fonts[0][GLC_STYLE_NORMAL].xfontsuffix = strdup("");
	c->fonts[0][GLC_STYLE_NORMAL].isFont = TRUE;
	glXUseXFont(c->fonts[0][GLC_STYLE_NORMAL].xfont, 0, 256, c->list_base);
	for(i = 1; i < NUM_GLC_STYLES; i++) c->fonts[0][i].isFont = FALSE;

	return j;
}

/* Assign the value c to the current thread's GLC context ID variable.
 * FIX ME!!  Because we use static globals to keep track of the GLC context
 * ID, this code is not thread-safe!  Additionally, we should return
 * GLC_STATE_ERROR if the requested context does not belong to the thread
 * that requested the context.
 */
void glcContext(GLint c){
	if(!glcIsContext(c) || c == 0) {
		last_error = GLC_PARAMETER_ERROR;
	} else {
		currentGlcContext = c;
		g = &glcContexts[currentGlcContext];
	}
}

/* Mark for deletion GLC context c.
 * FIX ME!! If no client is using the GLC context, then we should delete the 
 * context immediately.  This requires adding thread support.
 */
void glcDeleteContext(GLint c) {
	int i, j;

	if(!glcIsContext(c)) {
		last_error = GLC_PARAMETER_ERROR;
		return;
	}

	/* Find this context in the list */
	for(j = 0; glcContextList[j] != 0; j++)
		if(glcContextList[j] == c) break;

	/* Delete all fonts in this context */
	for(i = glcContexts[c].num_fonts-1; i > 0; i--)
		glcDeleteFont(i);

	/* Delete the GL lists we are using */
	glDeleteLists(g->list_base, 256);

	/* Mark this context as unused */
	glcContexts[j].isContext = FALSE;

	/* Delete this context from the list */
	for(; glcContextList[j] != 0; j++)
		glcContextList[j] = glcContextList[j + 1];
}

/* Return a zero-terminated array of GLC context IDs */
GLint* glcGetAllContexts() {
	int *list = malloc(sizeof(glcContextList));
	memcpy(list, glcContextList, sizeof(glcContextList));
	return list;
}

/* Return the value of the current issuing thread's current GLC context ID
 * variable.  FIX ME!!  We only keep track of one GLC context, so our code
 * is not thread-safe.
 */
GLint glcGetCurrentContext() {
	return currentGlcContext;
}

/* Return GL_TRUE if the GLC context identified by context is one of the
 * client's GLC contexts.
 */
GLboolean glcIsContext(GLint context) {
	if(context > MAX_GLC_CONTEXTS || context < 0) {
		last_error = GLC_PARAMETER_ERROR;
		return FALSE;
	}
	return glcContexts[context].isContext;
}

/* Retrive the value of the issuing thread's GLC error code variable, and
 * reset that variable to GLC_NONE.
 * FIX ME!! This variable should be per-thread.
 */
GLCenum glcGetError() { 
	GLCenum error_code;
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return GLC_NONE;
	}
	error_code = last_error;
	last_error = GLC_NONE;
	return error_code;
}

/* --------------------------------------------------------------------------
 * Context commands
 * NOTE: All functions beyond this point return GLC_STATE_ERROR if there is
 * no current context.
 * --------------------------------------------------------------------------
 */

/* reloadFont() is a local function that deletes the current display list
 * and re-creates it using the font specified in current_font_list[0].
 * FIX ME!!  We should not use display lists for texture-mapped fonts (which
 * aren't supported yet anyway), and if we are to support font maps, then
 * we should be able to load more than one font at a time into the font
 * list.  As is, we can load more than one font, but we will only use the
 * first font loaded.
 */
static void reloadFont() {
	char fontbuf[2048], matbuf[256];
	int i = g->current_font_list[0], j;
	GLCfont *f = &g->fonts[i][g->font_faces[i]];

	/* If we changed scale (or changed font faces), then we need to
	 * reload the xfontinfo structure, which keeps track of the size
	 * of the current font.
	 */
	if(g->scale_change) {
		if(f->can_transform) {
			snprintf(matbuf, sizeof(matbuf), "[%f %f %f %f]",
				g->bitmap_scale[0], 0.0,
				0.0, g->bitmap_scale[1]);
			for(j = 0; matbuf[j] != 0; j++) if(matbuf[j] == '-') matbuf[j] = '~';
			snprintf(fontbuf, sizeof(fontbuf), "%s-%s-%s",
				f->xfontprefix, matbuf, f->xfontsuffix);
		} else {
			snprintf(fontbuf, sizeof(fontbuf), "%s-%d-%s",
				f->xfontprefix, (int)g->bitmap_scale[0], f->xfontsuffix);
		}
		if(f->xfontinfo) XFreeFont(g->display, f->xfontinfo);
		f->xfontinfo = XLoadQueryFont(g->display, fontbuf);
#ifdef DEBUG
		fprintf(stderr, "Reloading font info for %s\n", fontbuf);
#endif
		g->scale_change = FALSE;
	}

	/* We ALWAYS need to reload the display list (unless we are using
	 * texture-mapped fonts)
	 */
	if(f->can_transform) {
		snprintf(matbuf, sizeof(matbuf), "[%f %f %f %f]",
			g->bitmap_matrix[0], g->bitmap_matrix[1],
			g->bitmap_matrix[2], g->bitmap_matrix[3]);
		for(j = 0; matbuf[j] != 0; j++) if(matbuf[j] == '-') matbuf[j] = '~';
		snprintf(fontbuf, sizeof(fontbuf), "%s-%s-%s",
			f->xfontprefix, matbuf, f->xfontsuffix);
	}
	if(f->xfont) XUnloadFont(g->display, f->xfont);
	f->xfont = XLoadFont(g->display, fontbuf);
	glXUseXFont(f->xfont, 0, 256, g->list_base);
	g->reload_font = FALSE;

}

/* --------------------------------------------------------------------------
 * Rendering commands
 * --------------------------------------------------------------------------
 */

/* Render the character that c is mapped to.  FIX ME!! We do not support
 * font maps yet, so we instead just render whatever c happens to come out
 * to.  Also, glcRenderString and glcRenderCountedString should probably
 * call this function, not the other way around.
 */
void glcRenderChar(char c) {
	glcRenderCountedString(1, &c);
}

/* Render a null-terminated string s */
void glcRenderString(const GLCchar *s) {
	glcRenderCountedString(strlen(s), s);
}

/* Render a string s of n characters.  Note that if the string is a UTF8
 * string (which is not yet supported), n may be less than count.
 */
void glcRenderCountedString(GLint count, const GLCchar* s) {
	int i, j;
	GLCfont *f;
	XCharStruct *p;
	int min, max;
	float font_width;

	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	if(count < 0) {
		last_error = GLC_PARAMETER_ERROR;
		return;
	}
	i = g->current_font_list[0];
	f = &g->fonts[i][g->font_faces[i]];
	if(g->reload_font) reloadFont();

	/* Find out which characters in the font have per-char data */
	min = f->xfontinfo->min_byte1;
	max = f->xfontinfo->max_byte1;
	if(min == 0 && max == 0) {
		min = f->xfontinfo->min_char_or_byte2;
		max = f->xfontinfo->max_char_or_byte2;
	} else {
		min += f->xfontinfo->min_char_or_byte2<<8;
		max += f->xfontinfo->max_char_or_byte2<<8;
	}

	/* Render the string */
	for(j = 0; j < count; j++) {
		if(f->xfontinfo->per_char)
			p = &f->xfontinfo->per_char[s[j] - min];
		else
			p = &f->xfontinfo->max_bounds;
		font_width = (float)p->width / g->bitmap_scale[0];

		glCallList(g->list_base + s[j]);

		/* Advance the raster position in the y direction.  We should
		 * only do this if GLX doesn't know about rotated fonts.
		 */
		glBitmap(0, 0, 0, 0,
			0.0,
			g->bitmap_matrix[1]*font_width,
			NULL);
	}
}

/* --------------------------------------------------------------------------
 * Font Commands
 * --------------------------------------------------------------------------
 */

/* Remove all the elements from the current font list, and, if font is
 * non-zero, append it to the list.  FIX ME!! What happens if we try to
 * render when the list is empty?
 */
void glcFont(GLint font) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	if(font != g->current_font_list[0]) {
		g->reload_font = TRUE;
		g->scale_change = TRUE;
	}
	if(font != 0) {
		g->num_current_fonts = 1;
		g->current_font_list[0] = font;
		g->current_font_list[1] = 0;
	} else {
		g->num_current_fonts = 0;
		g->current_font_list[0] = 0;
	}
}

/* Return GL_TRUE if font is the ID of a font. */
GLboolean glcIsFont(GLint font) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return FALSE;
	}
	return g->fonts[font][GLC_STYLE_NORMAL].isFont;
}

/* Delete the font identified by font.  FIX ME!!  What happens if this
 * leaves the current font list empty?
 */
void glcDeleteFont(GLint font) {
	int j;

	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	if(g->font_list[font] == 0) {
		last_error = GLC_PARAMETER_ERROR;
		return;
	}

	/* Delete this font */
	for(j = 0; j < NUM_GLC_STYLES; j++) {
		if(g->fonts[font][j].isFont) {
			if(g->fonts[font][j].name)
				free(g->fonts[font][j].name);
			if(g->fonts[font][j].xfontsuffix)
				free(g->fonts[font][j].xfontsuffix);
			g->fonts[font][j].isFont = FALSE;
		}
	}

	/* Find this font in the font list */
	for(j = 0; g->current_font_list[j] != 0; j++)
		if(g->current_font_list[j] == font) break;

	/* Delete this font from the list */
	for(; g->current_font_list[j] != 0; j++)
		g->current_font_list[j] = g->current_font_list[j+1];
}

/* Attempt to set the current face of the font identified by fontnum to the
 * face identified by the string style.  If the font does not support this
 * face, then do nothing and return GL_FALSE.  If fontnum is zero, then
 * do this for all fonts, and return GL_FALSE iff no fonts were changed.
 */
GLboolean glcFontFace(GLint fontnum, const char *style) {
	int fontface, j;

	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return FALSE;
	}

	if(!strcasecmp(style, "Bold")) {
		fontface = GLC_STYLE_BOLD;
	} else if(!strcasecmp(style, "Normal")) {
		fontface = GLC_STYLE_NORMAL;
	} else if(!strcasecmp(style, "Italic")) {
		fontface = GLC_STYLE_ITALIC;
	} else if(!strcasecmp(style, "Bold Italic")) {
		fontface = GLC_STYLE_BOLD_ITALIC;
	} else {
		last_error = GLC_PARAMETER_ERROR;
		return FALSE;
	}

	/* If fontnum is zero, iterate through all the fonts in the current
	 * font list.
	 */
	if(fontnum == 0) {
		GLboolean change = FALSE;
		for(j = 0; j < g->num_current_fonts; j++) {
			if(glcFontFace(j, style)) change = TRUE;
		}
		return change;
	}

	if(g->fonts[fontnum][fontface].isFont) {
		g->font_faces[fontnum] = fontface;

		if(g->current_font_list[0] == fontnum) {
			g->reload_font = TRUE;
			g->scale_change = TRUE;
		}

#ifdef DEBUG
		fprintf(stderr, "Switching font %d to face %s (%d)\n", fontnum, style, fontface);
#endif
		return TRUE;
	} else {
#ifdef DEBUG
		fprintf(stderr, "Not switching font %d to face %s\n", fontnum, style);
#endif
		return FALSE;
	}

}

/* Beginning with the first element of the GLC master list, find the first
 * master whose string attribute GLC_FAMILY equals inFamily.  We do not
 * implement masters, so we simulate this behaviour using the X font server's
 * list of fonts as our list of masters.  Return fontnum on success, or
 * zero otherwise.
 */
GLint glcNewFontFromFamily(GLint fontnum, const char *s) {
	char **xfontlist;
	int fontlen = strlen(s);
	char *fontname;
	int num_fonts;
	int font, our_font[NUM_GLC_STYLES];
	int i, j;
	char *t;
	int font_can_transform = TRUE;

#ifdef DEBUG
	fprintf(stderr, "glcNewFontFromFamily(%d, \"%s\")\n", fontnum, s);
#endif

	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return 0;
	}

	/* Get a list of fonts from the X server that match this font family,
	 * giving precedence to adobe fonts.
	 */
	fontname = (char*)malloc(fontlen+1024);
	sprintf(fontname, "-adobe-%s-*-*-*-*-[1 0 0 1]-0-*-*-*-*-*-*", s);
	xfontlist = XListFonts(g->display, fontname, 1000, &num_fonts);
	if(num_fonts == 0) {
		sprintf(fontname, "-*-%s-*-*-*-*-[1 0 0 1]-0-*-*-*-*-*-*", s);
		xfontlist = XListFonts(g->display, fontname, 1000, &num_fonts);
		if(num_fonts == 0) {
			font_can_transform = FALSE;
			sprintf(fontname, "-*-%s-*-*-*-*-0-0-*-*-*-*-*-*", s);
			xfontlist = XListFonts(g->display, fontname, 1000, &num_fonts);
			if(num_fonts == 0) {
				for(j = 0; j < NUM_GLC_STYLES; j++)
					g->fonts[fontnum][j].isFont = FALSE;
#ifdef DEBUG
				fprintf(stderr, "No X fonts found for request %s.\n", fontname);
#endif
				free(fontname);
				return 0;
			}
		}
	}
	free(fontname);

	/* Determine which fonts in this family have the faces we are looking
	 * for.  We search the structure upside-down, because fonts that
	 * come first should take precedence.
	 */
	for(j = 0; j < NUM_GLC_STYLES; j++) our_font[j] = INT_MAX;
	for(font = num_fonts-1; font >= 0; font--) {
		if(strstr(xfontlist[font], "medium-r")) {
			our_font[GLC_STYLE_NORMAL] = font;
		} else if(strstr(xfontlist[font], "bold-r")) {
			our_font[GLC_STYLE_BOLD] = font;
		} else if(strstr(xfontlist[font], "medium-i")) {
			our_font[GLC_STYLE_ITALIC] = font;
		} else if(strstr(xfontlist[font], "bold-i")) {
			our_font[GLC_STYLE_BOLD_ITALIC] = font;
		}
	}

	/* Set the font information in the GLC context */
	for(j = 0; j < NUM_GLC_STYLES; j++) {
		if(our_font[j] != INT_MAX) {
			if(g->fonts[fontnum][j].isFont) glcDeleteFont(fontnum);
			g->fonts[fontnum][j].xfont = XLoadFont(g->display, xfontlist[our_font[j]]);
			g->fonts[fontnum][j].isFont = TRUE;
			g->fonts[fontnum][j].name = strdup(s);
			g->fonts[fontnum][j].xfontprefix = strdup(xfontlist[our_font[j]]);
			for(t = g->fonts[fontnum][j].xfontprefix,i=0; *t != 0 && i < 7; t++)
				if(*t == '-') i++;
			if(*t != 0) {
				*(t-1) = 0;
				for(t++; *t != 0 && i < 8; t++) if(*t == '-') i++;
			}
			g->fonts[fontnum][j].xfontsuffix = t;
			g->fonts[fontnum][j].can_transform = font_can_transform;
			g->fonts[fontnum][j].xfontinfo = NULL;
			g->fonts[fontnum][j].xfont = 0;
#ifdef DEBUG
			fprintf(stderr, "Using font %s-[1 0 0 1]-%s\n",
				g->fonts[fontnum][j].xfontprefix,
				g->fonts[fontnum][j].xfontsuffix);
#endif
		} else {
			g->fonts[fontnum][j].isFont = FALSE;
#ifdef DEBUG
			fprintf(stderr, "Not using font style %d.\n", j);
#endif
		}
	}

	/* Free the list of fonts and return */
	XFreeFontNames(xfontlist);
	return fontnum;
}

/* Return a fond ID that is not an element of the GLC font list.
 * FIX ME!! We should not add this font ID to the font list yet.  This is
 * done when we call glcNewFontFromXYZ
 */
void glcGenFontID() {
	int j;

	/* Find a font that is not in use */
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	for(j = 1; j < MAX_GLC_FONTS; j++)
		if(!g->fonts[j][GLC_STYLE_NORMAL].isFont) break;
	if(j == MAX_GLC_FONTS) return;

	/* Add this font to the list */
	g->font_list[g->num_fonts++] = j;
	g->font_list[g->num_fonts] = 0;

	/* Initialize the font */
	g->fonts[j][GLC_STYLE_NORMAL].isFont = TRUE;
}

/* --------------------------------------------------------------------------
 * More context commands -- Data retrieval and storage
 * --------------------------------------------------------------------------
 */

/* Return the value of the string constant identified by attrib. */
const GLCchar* glcGetc(GLCenum attrib) { 
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return NULL;
	}
	switch(attrib) {
		case GLC_EXTENSIONS: return GLC_EXTENSIONS_STRING;
		case GLC_RELEASE: return GLC_RELEASE_STRING;
		case GLC_VENDOR: return GLC_VENDOR_STRING;
		default: return NULL;
	}
}

/* Return the value of the floating point variable identified by attrib.
 * FIX ME!! We should return a real value for resolution, not just 0.0
 */
GLfloat glcGetf(GLCenum attrib) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return 0.0;
	}
	switch(attrib) {
		case GLC_RESOLUTION: return 0.0;
		default: return 0.0;
	}
}

/* Store into vec the value of the floating point vector variable identified
 * by attrib.
 */
GLfloat* glcGetfv(GLCenum attrib, GLfloat *vec) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return NULL;
	}
	switch(attrib) {
		case GLC_BITMAP_MATRIX:
			memcpy(vec, g->bitmap_matrix, sizeof(g->bitmap_matrix));
			return vec;
		default:
			last_error = GLC_PARAMETER_ERROR;
			return NULL;
	}
}

/* Return the value of the integer variable or constant identified by attrib.
 * FIX ME!! We should return a real value for some of these, not just 0.
 */
GLint glcGeti(GLCenum attrib) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return GLC_NONE;
	}
	switch(attrib) {
		case GLC_CATALOG_COUNT: return 0;
		case GLC_CURRENT_FONT_COUNT: return 0;
		case GLC_FONT_COUNT: return 0;
		case GLC_LIST_OBJECT_COUNT: return 0;
		case GLC_MASTER_COUNT: return 0;
		case GLC_MEASURED_CHAR_COUNT: return 0;
		case GLC_RENDER_STYLE: return GLC_BITMAP;
		case GLC_REPLACEMENT_CODE: return 0;
		case GLC_STRING_TYPE: return GLC_UCS1;
		case GLC_TEXTURE_OBJECT_COUNT: return 0;
		case GLC_VERSION_MAJOR: return GLC_VERSION_MAJOR_NUMBER;
		case GLC_VERSION_MINOR: return GLC_VERSION_MINOR_NUMBER;
		default: return 0;
	}
}

/* Return TRUE if the boolean value identified by attrib is true.
 * Unfortunately, there are no boolean values in the GLC specification that
 * we can return.
 */
GLboolean glcIsEnabled(GLCenum attrib) { 
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return FALSE;
	}
	return 0;
}

/* Assign the value pointer to the variable GLC_DATA_POINTER */
void glcDataPointer(GLvoid *pointer) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	g->data_pointer = pointer;
}

/* Return the valuie of the pointer identified by attrib.  There is
 * only one possible pointer that we can return.
 */
GLvoid* glcGetPointer(GLCenum attrib) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return NULL;
	}
	switch(attrib) {
		case GLC_DATA_POINTER: return g->data_pointer;
		default: return NULL;
	}
}

/* --------------------------------------------------------------------------
 * Bitmap Matrix Commands
 * --------------------------------------------------------------------------
 */

/* Assign the value [ 1 0 0 1 ] to the floating point vector
 * GLC_BITMAP_MATRIX.  This causes a change in scale and forces the current
 * font to be reloaded.
 */
void glcLoadIdentity() {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	memcpy(g->bitmap_matrix, identity_matrix_2_by_2,
		sizeof(g->bitmap_matrix));
	g->bitmap_scale[0] = 1.0;
	g->bitmap_scale[1] = 1.0;
	g->reload_font = TRUE;
	g->scale_change = TRUE;
}

/* Assign the value [ matrix[0] matrix[1] matrix[2] matrix[3] ] to the floating
 * point vector GLC_BITMAP_MATRIX.  This causes a change in scale and causes
 * the current font to be reloaded.
 */
void glcLoadMatrix(const GLfloat *matrix) {
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	memcpy(g->bitmap_matrix, matrix, sizeof(g->bitmap_matrix));
	g->reload_font = TRUE;
	g->scale_change = TRUE;
}

/* Multiply the bitmap matrix by m1 and put the result back into the bitmap
 * matrix.
 */
void glcMultMatrix(const GLfloat *m1) {
	GLfloat m2[4];
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	memcpy(m2, g->bitmap_matrix, sizeof(m2));
        g->bitmap_matrix[0] = m1[0]*m2[0] + m1[2]*m2[1];
	g->bitmap_matrix[1] = m1[1]*m2[0] + m1[3]*m2[1];
	g->bitmap_matrix[2] = m1[0]*m2[2] + m1[2]*m2[3];
	g->bitmap_matrix[3] = m1[1]*m2[2] + m1[3]*m2[3];
	/* FIX ME!!  We need to know the new bitmap_scale vector */
	g->reload_font = TRUE;
	g->scale_change = TRUE;
}

/* Rotate the bitmap matrix by angle, which is in degrees.  We must be careful
 * that no change in scale occurs, as a change in scale decreases speed.
 */
void glcRotate(GLfloat angle) {
	GLfloat m[4];
	GLboolean t;
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	m[0] = cos(2*M_PI*angle/360.0);
	m[1] = sin(2*M_PI*angle/360.0);
	m[2] = -m[1];
	m[3] = m[0];
	t = g->scale_change;
	glcMultMatrix(m);
	g->reload_font = TRUE;
	g->scale_change = t;
}

/* Scale the bitmap matrix by x and y and reload the current font when it is
 * next rendered.
 */
void glcScale(GLfloat x, GLfloat y) {
	GLfloat m[4];
	if(currentGlcContext == 0) {
		last_error = GLC_STATE_ERROR;
		return;
	}
	/* the GLC spec says [x 1 1 y], but that can't be right? */
	m[0] = x;
	m[1] = 0;
	m[2] = 0;
	m[3] = y;
	glcMultMatrix(m);
	g->reload_font = TRUE;
	g->scale_change = TRUE;
	g->bitmap_scale[0] *= x;
	g->bitmap_scale[1] *= y;
}

/* Unsupported functions */
void glcCallbackFunc(GLCenum opcode, GLCfunc func) {}
GLCfunc glGetCallbackFunc(GLCenum opcode) { return NULL; }
void glcDeleteGLObjects() {}
void glcDisable(GLCenum attrib) {}
const GLCchar* glcGetListc(GLCenum attrib, GLint index) { return NULL; }
GLint glcGetListi(GLCenum attrib, GLint index) { return 0; }
void glcStringType(GLCenum type) {}
void glcAppendCatalog(const GLCchar* catalog) {}
const GLCchar* glcGetMasterListc(GLint master, GLCenum attrib, GLint index) { return NULL; }
const GLCchar* glcGetMasterMap(GLint master, GLint code) { return NULL; }
const GLCchar glcGetMasterc(GLint master, GLCenum attrib) { return 0; }
GLint glcGetMasteri(GLint master, GLCenum attrib) { return 0; }
void glcPrependCatalog(const GLCchar* catalog) {}
void glcRemoveCatalog(GLint index) {}
void glcAppendFont(GLint font) {}
void glcFontMap(GLint font, GLint code, const GLCchar *name) {}
const GLCchar* glcGetFontFace(GLint font) { return NULL; }
const GLCchar* glcGetFontListc(GLint font, GLCenum attrib, GLint index) { return NULL; }
const GLCchar* glcGetFontMap(GLint font, GLint code) { return NULL; }
const GLCchar* glcGetFontc(GLint font, GLCenum attrib) { return NULL; }
GLint glcGetFonti(GLint font, GLCenum attrib) { return 0; }
GLint glcNewFontFromMaster(GLint font, GLint master) { return 0; }
void glcRenderStyle(GLCenum style) {}
void glcReplacementCode(GLint code) {}
void glcResolution(GLfloat res) {}
GLfloat* glcGetCharMetric(GLint code, GLCenum metric, GLfloat* vec) { return NULL; }
GLfloat* glcGetMaxCharMetric(GLCenum metric, GLfloat* vec) { return NULL; }
GLfloat* glcGetStringCharMetric(GLint index, GLCenum metric, GLfloat *vec) { return NULL; }
GLfloat* glcGetStringMetric(GLCenum metric, GLfloat *vec) { return NULL; }
GLint glcMeasureCountedString(GLboolean chars, GLint count, const GLCchar *s) { return 0; }
GLint glcMeasureString(GLboolean chars, const GLCchar *s) { return 0; }

