/*
 * GL_EXT_pixel_buffer_object test
 * 
 * Brian Paul
 * 11 March 2004
 */

#define GL_GLEXT_PROTOTYPES

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#include "../util/readtex.c"  /* a hack, I know */

#define IMAGE_FILE "../images/girl.rgb"

static int ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLubyte *Image = NULL;

static int APosX, APosY;  /* simple drawpixels */
static int BPosX, BPosY;  /* read/draw pixels */
static int CPosX, CPosY;  /* copypixels */

static GLboolean DrawFront = GL_FALSE;
static GLboolean ScaleAndBias = GL_FALSE;
static GLboolean Benchmark = GL_FALSE;

static GLuint DrawPBO, TempPBO;


static GLenum ReadFormat = GL_RGBA;
static GLenum ReadType = GL_UNSIGNED_BYTE;




static void
Reset( void )
{
   APosX = 5;     APosY = 20;
   BPosX = APosX + ImgWidth + 5;   BPosY = 20;
   CPosX = BPosX + ImgWidth + 5;   CPosY = 20;
}


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
SetupPixelTransfer(GLboolean invert)
{
   if (invert) {
      glPixelTransferf(GL_RED_SCALE, -1.0);
      glPixelTransferf(GL_RED_BIAS, 1.0);
      glPixelTransferf(GL_GREEN_SCALE, -1.0);
      glPixelTransferf(GL_GREEN_BIAS, 1.0);
      glPixelTransferf(GL_BLUE_SCALE, -1.0);
      glPixelTransferf(GL_BLUE_BIAS, 1.0);
   }
   else {
      glPixelTransferf(GL_RED_SCALE, 1.0);
      glPixelTransferf(GL_RED_BIAS, 0.0);
      glPixelTransferf(GL_GREEN_SCALE, 1.0);
      glPixelTransferf(GL_GREEN_BIAS, 0.0);
      glPixelTransferf(GL_BLUE_SCALE, 1.0);
      glPixelTransferf(GL_BLUE_BIAS, 0.0);
   }
}


static void
Display( void )
{
   glClearColor(.3, .3, .3, 1);
   glClear( GL_COLOR_BUFFER_BIT );

   /** Unbind UNPACK pixel buffer before calling glBitmap */
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);

   glRasterPos2i(5, ImgHeight+25);
   PrintString("f = toggle front/back  s = toggle scale/bias  b = benchmark");

   glRasterPos2i(5, ImgHeight+40);
   PrintString("GL_EXT_pixel_buffer_object test");

   /* draw original image */
   glRasterPos2i(APosX, 5);
   PrintString("Original");
   glRasterPos2i(APosX, APosY);
   glEnable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);
   /*** Draw from the DrawPBO */
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, DrawPBO);
   glDrawPixels(ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, 0);
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);

   /* do readpixels, drawpixels */
   glRasterPos2i(BPosX, 5);
   PrintString("Read/DrawPixels");
   SetupPixelTransfer(ScaleAndBias);
   /*** read into the Temp PBO */
   glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, TempPBO);
   if (Benchmark) {
      GLint reads = 0;
      GLint endTime;
      GLint startTime = glutGet(GLUT_ELAPSED_TIME);
      GLdouble seconds, pixelsPerSecond;
      printf("Benchmarking...\n");
      do {
         glReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                      ReadFormat, ReadType, 0);
         reads++;
         endTime = glutGet(GLUT_ELAPSED_TIME);
      } while (endTime - startTime < 4000);   /* 4 seconds */
      seconds = (double) (endTime - startTime) / 1000.0;
      pixelsPerSecond = reads * ImgWidth * ImgHeight / seconds;
      printf("Result:  %d reads in %f seconds = %f pixels/sec\n",
             reads, seconds, pixelsPerSecond);
      Benchmark = GL_FALSE;
   }
   else {
      glReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                   ReadFormat, ReadType, 0);
   }
   glRasterPos2i(BPosX, BPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);
   /*** draw from the Temp PBO */
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, TempPBO);
   glDrawPixels(ImgWidth, ImgHeight, ReadFormat, ReadType, 0);
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);

   /* do copypixels */
   glRasterPos2i(CPosX, 5);
   PrintString("CopyPixels");
   glRasterPos2i(CPosX, CPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(ScaleAndBias);
   glCopyPixels(APosX, APosY, ImgWidth, ImgHeight, GL_COLOR);

   if (!DrawFront)
      glutSwapBuffers();
   else
      glFinish();
}


static void
Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0.0, width, 0.0, height, -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void
Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'b':
         Benchmark = GL_TRUE;
         break;
      case 's':
         ScaleAndBias = !ScaleAndBias;
         break;
      case 'f':
         DrawFront = !DrawFront;
         if (DrawFront) {
            glDrawBuffer(GL_FRONT);
            glReadBuffer(GL_FRONT);
         }
         else {
            glDrawBuffer(GL_BACK);
            glReadBuffer(GL_BACK);
         }
         printf("glDrawBuffer(%s)\n", DrawFront ? "GL_FRONT" : "GL_BACK");
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init( GLboolean ciMode )
{
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   if (!glutExtensionSupported("GL_EXT_pixel_buffer_object")) {
      printf("Sorry, this demo requires GL_EXT_pixel_buffer_object\n");
      exit(0);
   }

   Image = LoadRGBImage( IMAGE_FILE, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }

   if (ciMode) {
      /* Convert RGB image to grayscale */
      GLubyte *indexImage = (GLubyte *) malloc( ImgWidth * ImgHeight );
      GLint i;
      for (i=0; i<ImgWidth*ImgHeight; i++) {
         int gray = Image[i*3] + Image[i*3+1] + Image[i*3+2];
         indexImage[i] = gray / 3;
      }
      free(Image);
      Image = indexImage;
      ImgFormat = GL_COLOR_INDEX;

      for (i=0;i<255;i++) {
         float g = i / 255.0;
         glutSetColor(i, g, g, g);
      }
   }

   printf("Loaded %d by %d image\n", ImgWidth, ImgHeight );

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, ImgWidth);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ROW_LENGTH, ImgWidth);

   Reset();

   /* put image into DrawPBO */
   glGenBuffersARB(1, &DrawPBO);
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, DrawPBO);
   glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_EXT,
                   ImgWidth * ImgHeight * 4, Image, GL_STATIC_DRAW);

   /* Setup TempPBO - used for glReadPixels & glDrawPixels */
   glGenBuffersARB(1, &TempPBO);
   glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, TempPBO);
   glBufferDataARB(GL_PIXEL_PACK_BUFFER_EXT,
                   ImgWidth * ImgHeight * 4, NULL, GL_DYNAMIC_COPY);

}


int
main( int argc, char *argv[] )
{
   GLboolean ciMode = GL_FALSE;
   if (argc > 1 && strcmp(argv[1], "-ci")==0) {
      ciMode = GL_TRUE;
   }
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 750, 250 );
   if (ciMode)
      glutInitDisplayMode( GLUT_INDEX | GLUT_DOUBLE );
   else
      glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   Init(ciMode);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutMainLoop();
   return 0;
}
