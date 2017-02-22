
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>
#include "GL/gl.h"

static void Init( void )
{
   static const char *modulate2D =
      "!!ARBfp1.0\n"
      "MOV result.color, fragment.color; \n"
      "END"
      ;
   GLuint modulateProg;

   if (!glutExtensionSupported("GL_ARB_fragment_program")) {
      printf("Error: GL_ARB_fragment_program not supported!\n");
      exit(1);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* Setup the fragment program */
   glGenProgramsARB(1, &modulateProg);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, modulateProg);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(modulate2D), (const GLubyte *)modulate2D);

   printf("glGetError = 0x%x\n", (int) glGetError());
   printf("glError(GL_PROGRAM_ERROR_STRING_ARB) = %s\n",
          (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));

   glEnable(GL_FRAGMENT_PROGRAM_ARB);

   glClearColor(.3, .3, .3, 0);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
	exit(1);
      default:
	return;
    }

    glutPostRedisplay();
}

static void Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT); 

   glBegin(GL_TRIANGLES);
   glColor3f(0,0,1); 
   glVertex3f( 0.9, -0.9, -30.0);
   glColor3f(1,0,0); 
   glVertex3f( 0.9,  0.9, -30.0);
   glColor3f(0,1,0); 
   glVertex3f(-0.9,  0.0, -30.0);
   glEnd();

   glFlush();
}


int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);

    glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

    type = GLUT_RGB | GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("First Tri") == GL_FALSE) {
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
