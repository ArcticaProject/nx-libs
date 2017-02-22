/* Test glGenProgramsNV(), glIsProgramNV(), glLoadProgramNV() */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

static float Zrot = 0.0;


static void Display( void )
{
   glClearColor(0.3, 0.3, 0.3, 1);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glEnable(GL_VERTEX_PROGRAM_NV);

   glLoadIdentity();
   glRotatef(Zrot, 0, 0, 1);

   glPushMatrix();

   glVertexAttrib3fARB(3, 1, 0.5, 0.25);
   glBegin(GL_TRIANGLES);
#if 1
   glVertexAttrib3fARB(3, 1.0, 0.0, 0.0);
   glVertexAttrib2fARB(0, -0.5, -0.5);
   glVertexAttrib3fARB(3, 0.0, 1.0, 0.0);
   glVertexAttrib2fARB(0, 0.5, -0.5);
   glVertexAttrib3fARB(3, 0.0, 0.0, 1.0);
   glVertexAttrib2fARB(0, 0,  0.5);
#else
   glVertex2f( -1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 0,  1);
#endif
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   /*   glFrustum( -2.0, 2.0, -2.0, 2.0, 5.0, 25.0 );*/
   glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 2.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   /*glTranslatef( 0.0, 0.0, -15.0 );*/
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'z':
         Zrot -= 5.0;
         break;
      case 'Z':
         Zrot += 5.0;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   GLint errno;
   GLuint prognum;
   
   static const char *prog1 =
      "!!ARBvp1.0\n"
      "MOV  result.color, vertex.color;\n"

      "DP4  result.position.x, vertex.position, state.matrix.modelview.row[0];\n"
      "DP4  result.position.y, vertex.position, state.matrix.modelview.row[1];\n"
      "DP4  result.position.z, vertex.position, state.matrix.modelview.row[2];\n"
      "DP4  result.position.w, vertex.position, state.matrix.modelview.row[3];\n"
      "END\n";

   glGenProgramsARB(1, &prognum);

   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(prog1), (const GLubyte *) prog1);

   assert(glIsProgramARB(prognum));
   errno = glGetError();
   printf("glGetError = %d\n", errno);
   if (errno != GL_NO_ERROR)
   {
      GLint errorpos;

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
      printf("errorpos: %d\n", errorpos);
      printf("%s\n", (char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   }
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 250, 250 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
