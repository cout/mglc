
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This program has been modified from its original form to provide
 * some additional tests for GLC.
 */

#include <string.h>
#include <GL/glut.h>
#include "glc.h"

int font = 1;
char defaultMessage[] = "GLUT means OpenGL.";
char *message = defaultMessage;
char *facelist[] = {"Bold", "Italic", "Bold Italic", "Normal"};
int angle = 0;

void
selectFont(int newfont)
{
  font = newfont;
  glutPostRedisplay();
}

void
selectFace(int newface)
{
  newface--;
  glcFontFace(1, facelist[newface]);
  glcFontFace(2, facelist[newface]);
  glcFontFace(3, facelist[newface]);
}

void
selectMessage(int msg)
{
  switch (msg) {
  case 1:
    message = "abcdefghijklmnop";
    break;
  case 2:
    message = "ABCDEFGHIJKLMNOP";
    break;
  }
}

void
tick(void)
{
  angle -= 2;
  glutPostRedisplay();
}

void
display(void)
{
#if 0
  int len, i;

  glClear(GL_COLOR_BUFFER_BIT);
  glPushMatrix();
  glRotatef(angle, 0.0, 0.0, 1.0);
  glTranslatef(-750, 0, 0);
  len = (int) strlen(message);
  for (i = 0; i < len; i++) {
    glutStrokeCharacter(font, message[i]);
  }
  glPopMatrix();
#else
  glPushMatrix();
  glClear(GL_COLOR_BUFFER_BIT);
  glcRotate(2);
  /* glRotatef(angle, 0.0, 0.0, 1.0); */
  glRasterPos2f(100, 100);
  glcFont(font);
  glcRenderString(message);
  glPopMatrix();
#endif
  glutSwapBuffers();
}

int
main(int argc, char **argv)
{
  int submenu;
  int subfaces;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(600, 600);
  glutCreateWindow("GLC font library demo");
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, 2000, 0, 2000);
  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(3.0);
  glTranslatef(1000, 1000, 0);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glColor3f(1.0, 1.0, 1.0);
  glutDisplayFunc(display);
  glutIdleFunc(tick);
  subfaces = glutCreateMenu(selectFace);
  glutAddMenuEntry("Bold", 1);
  glutAddMenuEntry("Italic", 2);
  glutAddMenuEntry("Bold Italic", 3);
  glutAddMenuEntry("Normal", 4);
  submenu = glutCreateMenu(selectMessage);
  glutAddMenuEntry("abc", 1);
  glutAddMenuEntry("ABC", 2);
  glutCreateMenu(selectFont);
  glutAddMenuEntry("Helvetica", 1);
  glutAddMenuEntry("Courier", 2);
  glutAddMenuEntry("Times", 3);
  glutAddSubMenu("Faces", subfaces);
  glutAddSubMenu("Messages", submenu);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glcContext(glcGenContext());
  glcScale(30, 30);
  glcNewFontFromFamily(1, "Helvetica");
  glcNewFontFromFamily(2, "Courier");
  glcNewFontFromFamily(3, "Times");
  glcFont(font);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
