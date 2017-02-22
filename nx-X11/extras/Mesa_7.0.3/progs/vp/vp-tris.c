/* Test glGenProgramsNV(), glIsProgramNV(), glLoadProgramNV() */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

static const char *filename = NULL;
static GLuint nr_steps = 4;

static void usage( char *name )
{
   fprintf( stderr, "usage: %s [ options ] shader_filename\n", name );
   fprintf( stderr, "\n" );
   fprintf( stderr, "options:\n" );
   fprintf( stderr, "    -f     flat shaded\n" );
   fprintf( stderr, "    -nNr  subdivision steps\n" );
}


static void args(int argc, char *argv[])
{
   GLint i;

   for (i = 1; i < argc; i++) {
      if (strncmp(argv[i], "-n", 2) == 0) {
	 nr_steps = atoi((argv[i]) + 2);
      }
      else if (strcmp(argv[i], "-f") == 0) {
	 glShadeModel(GL_FLAT);
      }
      else if (i == argc - 1) {
	 filename = argv[i];
      }
      else {
	 usage(argv[0]);
	 exit(1);
      }
   }

   if (!filename) {
      usage(argv[0]);
      exit(1);
   }
}



static void Init( void )
{
   GLint errno;
   GLuint prognum;
   char buf[4096];
   GLuint sz;
   FILE *f;

   if ((f = fopen(filename, "r")) == NULL) {
      fprintf(stderr, "couldn't open %s\n", filename);
      exit(1);
   }

   sz = fread(buf, 1, sizeof(buf), f);
   if (!feof(f)) {
      fprintf(stderr, "file too long\n");
      exit(1);
   }

   fprintf(stderr, "%.*s\n", sz, buf);
      
   glGenProgramsARB(1, &prognum);

   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		      sz, (const GLubyte *) buf);

   errno = glGetError();
   printf("glGetError = %d\n", errno);
   if (errno != GL_NO_ERROR)
   {
      GLint errorpos;

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
      printf("errorpos: %d\n", errorpos);
      printf("%s\n", (char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   }
   assert(glIsProgramARB(prognum));
}


union vert {
   struct {
      GLfloat color[3];
      GLfloat pos[3];
   } v;
   GLfloat f[6];
};

static void make_midpoint( union vert *out,
			   const union vert *v0,
			   const union vert *v1)
{
   int i;
   for (i = 0; i < 6; i++)
      out->f[i] = v0->f[i] + .5 * (v1->f[i] - v0->f[i]);
}

static void subdiv( union vert *v0, 
		    union vert *v1,
		    union vert *v2,
		    GLuint depth )
{
   if (depth == 0) {
      glColor3fv(v0->v.color);
      glVertex3fv(v0->v.pos);
      glColor3fv(v1->v.color); 
      glVertex3fv(v1->v.pos);
      glColor3fv(v2->v.color); 
      glVertex3fv(v2->v.pos);
   }
   else {
      union vert m[3];

      make_midpoint(&m[0], v0, v1);
      make_midpoint(&m[1], v1, v2);
      make_midpoint(&m[2], v2, v0);
      
      subdiv(&m[0], &m[2], v0, depth-1);
      subdiv(&m[1], &m[0], v1, depth-1);
      subdiv(&m[2], &m[1], v2, depth-1);
      subdiv(&m[0], &m[1], &m[2], depth-1);
   }
}

/** Assignment */
#define ASSIGN_3V( V, V0, V1, V2 )  \
do {                                \
    V[0] = V0;                      \
    V[1] = V1;                      \
    V[2] = V2;                      \
} while(0)

static void Display( void )
{
   glClearColor(0.3, 0.3, 0.3, 1);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glEnable(GL_VERTEX_PROGRAM_NV);

   glBegin(GL_TRIANGLES);


   {
      union vert v[3];

      ASSIGN_3V(v[0].v.color, 0,0,1); 
      ASSIGN_3V(v[0].v.pos,  0.9, -0.9, 0.0);
      ASSIGN_3V(v[1].v.color, 1,0,0); 
      ASSIGN_3V(v[1].v.pos, 0.9, 0.9, 0.0);
      ASSIGN_3V(v[2].v.color, 0,1,0); 
      ASSIGN_3V(v[2].v.pos, -0.9, 0, 0.0);

      subdiv(&v[0], &v[1], &v[2], nr_steps);
   }

   glEnd();


   glFlush(); 
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   /*glTranslatef( 0.0, 0.0, -15.0 );*/
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}




int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 250, 250 );
   glutInitDisplayMode( GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   args( argc, argv );
   Init();
   glutMainLoop();
   return 0;
}
