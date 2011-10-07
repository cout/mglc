/* Mesa-GLC library version 0.3
 * Copyright (C) 2000 Paul Brannan
 * This library is released under the GNU LGPL.  For more information, please
 * see the file LICENSE, or visit the GNU web page at www.gnu.org.
 */

#ifndef _GLC_H
#define _GLC_H

#include <GL/gl.h>

typedef char GLCchar;
typedef void(*GLCfunc)(int);

typedef enum {
	GLC_NONE = 0,
	GLC_AUTO_FONT			= 0x0010,
	GLC_GL_OBJECTS			= 0x0011,
	GLC_MIPMAP			= 0x0012,
	GLC_OP_glcUnmappedCode		= 0x0020,
	GLC_BASELINE			= 0x0030,
	GLC_BOUNDS			= 0x0031,
	GLC_PARAMETER_ERROR		= 0x0040,
	GLC_RESOURCE_ERROR		= 0x0041,
	GLC_STATE_ERROR			= 0x0042,
	GLC_CHAR_LIST			= 0x0050,
	GLC_FACE_LIST			= 0x0051,
	GLC_FAMILY			= 0x0060,
	GLC_MASTER_FORMAT		= 0x0061,
	GLC_VENDOR			= 0x0062,
	GLC_VERSION			= 0x0063,
	GLC_CHAR_COUNT			= 0x0070,
	GLC_FACE_COUNT			= 0x0071,
	GLC_IS_FIXED_PITCH		= 0x0072,
	GLC_MAX_MAPPED_CODE		= 0x0073,
	GLC_MIN_MAPPED_CODE		= 0x0074,
	GLC_CATALOG_LIST		= 0x0080,
	GLC_CURRENT_FONT_LIST		= 0x0090,
	GLC_FONT_LIST			= 0x0091,
	GLC_LIST_OBJECT_LIST		= 0x0092,
	GLC_TEXTURE_OBJECT_LIST		= 0x0093,
	GLC_DATA_POINTER		= 0x00A0,
	GLC_EXTENSIONS			= 0x00B0,
	GLC_RELEASE			= 0x00B1,
	/* GLC_VENDOR			= 0x00B2, */
	GLC_RESOLUTION			= 0x00C0,
	GLC_BITMAP_MATRIX		= 0x00D0,
	GLC_CATALOG_COUNT		= 0x00E0,
	GLC_CURRENT_FONT_COUNT		= 0x00E1,
	GLC_FONT_COUNT			= 0x00E2,
	GLC_LIST_OBJECT_COUNT		= 0x00E3,
	GLC_MASTER_COUNT		= 0x00E4,
	GLC_MEASURED_CHAR_COUNT		= 0x00E5,
	GLC_RENDER_STYLE		= 0x00E6,
	GLC_REPLACEMENT_CODE		= 0x00E7,
	GLC_STRING_TYPE			= 0x00E8,
	GLC_TEXTURE_OBJECT_COUNT	= 0x00E9,
	GLC_VERSION_MAJOR		= 0x00EA,
	GLC_VERSION_MINOR		= 0x00EB,
	GLC_BITMAP			= 0x0100,
	GLC_LINE			= 0x0101,
	GLC_TEXTURE			= 0x0102,
	GLC_TRIANGLE			= 0x0103,
	GLC_UCS1			= 0x0110,
	GLC_UCS2			= 0x0111,
	GLC_UCS4			= 0x0112
} GLCenum;
	

/* Context functions */
GLint glcGenContext();
void glcContext(GLint g);
void glcDeleteContext(GLint g);
GLint* glcGetAllContexts();
GLint glcGetCurrentContext();
GLboolean glcIsContext(GLint context);

/* Rendering functions */
void glcRenderChar(char c);
void glcRenderString(const char *s);
void glcRenderCountedString(GLint count, const GLCchar* s);

/* Font functions */
void glcFont(GLint font);
GLboolean glcFontFace(GLint fontnum, const char *style);
GLint glcNewFontFromFamily(GLint fontnum, const char *s);
GLboolean glcIsFont(GLint font);
void glcDeleteFont(GLint font);

/* Information-retrival functions */
const GLCchar* glcGetc(GLCenum attrib);
GLfloat glcGetf(GLCenum attrib);
GLfloat* glcGetfv(GLCenum attrib, GLfloat *vec);
GLint glcGeti(GLCenum attrib);
GLboolean glcIsEnabled(GLCenum attrib);
void glcGenFontID();

/* Error-handling functions */
GLCenum glcGetError();

/* Data pointer functions */
void glcDataPointer(GLvoid *pointer);
GLvoid* glcGetPointer(GLCenum attrib);

/* Unsupported functions */
void glcCallbackFunc(GLCenum opcode, GLCfunc func);
GLCfunc glGetCallbackFunc(GLCenum opcode);
void glcDeleteGLObjects();
void glcDisable(GLCenum attrib);
const GLCchar* glcGetListc(GLCenum attrib, GLint index);
GLint glcGetListi(GLCenum attrib, GLint index);
void glcStringType(GLCenum type);
void glcAppendCatalog(const GLCchar* catalog);
const GLCchar* glcGetMasterListc(GLint master, GLCenum attrib, GLint index);
const GLCchar* glcGetMasterMap(GLint master, GLint code);
const GLCchar glcGetMasterc(GLint master, GLCenum attrib);
GLint glcGetMasteri(GLint master, GLCenum attrib);
void glcPrependCatalog(const GLCchar* catalog);
void glcRemoveCatalog(GLint index);
void glcAppendFont(GLint font);
void glcFontMap(GLint font, GLint code, const GLCchar *name);
const GLCchar* glcGetFontFace(GLint font);
const GLCchar* glcGetFontListc(GLint font, GLCenum attrib, GLint index);
const GLCchar* glcGetFontMap(GLint font, GLint code);
const GLCchar* glcGetFontc(GLint font, GLCenum attrib);
GLint glcGetFonti(GLint font, GLCenum attrib);
GLint glcNewFontFromMaster(GLint font, GLint master);
void glcLoadIdentity();
void glcLoadMatrix(const GLfloat *matrix);
void glcMultMatrix(const GLfloat *matrix);
void glcRotate(GLfloat angle);
void glcScale(GLfloat x, GLfloat y);
void glcRenderStyle(GLCenum style);
void glcReplacementCode(GLint code);
void glcResolution(GLfloat res);
GLfloat* glcGetCharMetric(GLint code, GLCenum metric, GLfloat* vec);
GLfloat* glcGetMaxCharMetric(GLCenum metric, GLfloat* vec);
GLfloat* glcGetStringCharMetric(GLint index, GLCenum metric, GLfloat *vec);
GLfloat* glcGetStringMetric(GLCenum metric, GLfloat *vec);
GLint glcMeasureCountedString(GLboolean chars, GLint count, const GLCchar *s);
GLint glcMeasureString(GLboolean chars, const GLCchar *s);

#endif
