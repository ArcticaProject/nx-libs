/**
 * Test texturing with GL shading language.
 *
 * Copyright (C) 2007  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GL/glut.h"
#include "readtex.h"
#include "extfuncs.h"

static const char *Demo = "texdemo1";

static const char *ReflectVertFile = "reflect.vert.txt";
static const char *CubeFragFile = "cubemap.frag.txt";

static const char *SimpleVertFile = "simple.vert.txt";
static const char *SimpleTexFragFile = "shadowtex.frag.txt";

static const char *GroundImage = "../images/tile.rgb";

static GLuint Program1, Program2;

static GLfloat TexXrot = 0, TexYrot = 0;
static GLfloat Xrot = 20.0, Yrot = 20.0, Zrot = 0.0;
static GLfloat EyeDist = 10;
static GLboolean Anim = GL_TRUE;


struct uniform_info {
   const char *name;
   GLuint size;
   GLint location;
   GLenum type;  /**< GL_FLOAT or GL_INT */
   GLfloat value[4];
};

static struct uniform_info ReflectUniforms[] = {
   { "cubeTex",  1, -1, GL_INT, { 0, 0, 0, 0 } },
   { "lightPos", 3, -1, GL_FLOAT, { 10, 10, 20, 0 } },
   { NULL, 0, 0, 0, { 0, 0, 0, 0 } }
};

static struct uniform_info SimpleUniforms[] = {
   { "tex2d",    1, -1, GL_INT,   { 1, 0, 0, 0 } },
   { "lightPos", 3, -1, GL_FLOAT, { 10, 10, 20, 0 } },
   { NULL, 0, 0, 0, { 0, 0, 0, 0 } }
};


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error %s (0x%x) at line %d\n",
             gluErrorString(err), (int) err, line);
   }
}


static void
DrawGround(GLfloat size)
{
   glPushMatrix();
   glRotatef(90, 1, 0, 0);
   glNormal3f(0, 0, 1);
   glBegin(GL_POLYGON);
   glTexCoord2f(-2, -2);  glVertex2f(-size, -size);
   glTexCoord2f( 2, -2);  glVertex2f( size, -size);
   glTexCoord2f( 2,  2);  glVertex2f( size,  size);
   glTexCoord2f(-2,  2);  glVertex2f(-size,  size);
   glEnd();
   glPopMatrix();
}


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_TEXTURE_2D);

   glPushMatrix(); /* modelview matrix */
      glTranslatef(0.0, 0.0, -EyeDist);
      glRotatef(Xrot, 1, 0, 0);
      glRotatef(Yrot, 0, 1, 0);
      glRotatef(Zrot, 0, 0, 1);

      /* sphere w/ reflection map */
      glPushMatrix();
         glTranslatef(0, 1, 0);
         glUseProgram_func(Program1);

         /* setup texture matrix */
         glActiveTexture(GL_TEXTURE0);
         glMatrixMode(GL_TEXTURE);
         glLoadIdentity();
         glRotatef(-TexYrot, 0, 1, 0);
         glRotatef(-TexXrot, 1, 0, 0);

         glEnable(GL_TEXTURE_GEN_S);
         glEnable(GL_TEXTURE_GEN_T);
         glEnable(GL_TEXTURE_GEN_R);
         glutSolidSphere(2.0, 20, 20);

         glLoadIdentity(); /* texture matrix */
         glMatrixMode(GL_MODELVIEW);
      glPopMatrix();

      /* ground */
      glUseProgram_func(Program2);
      glTranslatef(0, -1.0, 0);
      DrawGround(5);

   glPopMatrix();

   glutSwapBuffers();
}


static void
idle(void)
{
   GLfloat t = 0.05 * glutGet(GLUT_ELAPSED_TIME);
   TexYrot = t;
   glutPostRedisplay();
}


static void
key(unsigned char k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
   case ' ':
   case 'a':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'z':
      EyeDist -= 0.5;
      if (EyeDist < 6.0)
         EyeDist = 6.0;
      break;
   case 'Z':
      EyeDist += 0.5;
      if (EyeDist > 90.0)
         EyeDist = 90;
      break;
   case 27:
      exit(0);
   }
   glutPostRedisplay();
}


static void
specialkey(int key, int x, int y)
{
   GLfloat step = 2.0;
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_UP:
      Xrot += step;
      break;
   case GLUT_KEY_DOWN:
      Xrot -= step;
      break;
   case GLUT_KEY_LEFT:
      Yrot -= step;
      break;
   case GLUT_KEY_RIGHT:
      Yrot += step;
      break;
   }
   glutPostRedisplay();
}


/* new window size or exposure */
static void
Reshape(int width, int height)
{
   GLfloat ar = (float) width / (float) height;
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-2.0*ar, 2.0*ar, -2.0, 2.0, 4.0, 100.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
InitCheckers(void)
{
#define CUBE_TEX_SIZE 64
   GLubyte image[CUBE_TEX_SIZE][CUBE_TEX_SIZE][3];
   static const GLubyte colors[6][3] = {
      { 255,   0,   0 },	/* face 0 - red */
      {   0, 255, 255 },	/* face 1 - cyan */
      {   0, 255,   0 },	/* face 2 - green */
      { 255,   0, 255 },	/* face 3 - purple */
      {   0,   0, 255 },	/* face 4 - blue */
      { 255, 255,   0 }		/* face 5 - yellow */
   };
   static const GLenum targets[6] = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
   };

   GLint i, j, f;

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   /* make colored checkerboard cube faces */
   for (f = 0; f < 6; f++) {
      for (i = 0; i < CUBE_TEX_SIZE; i++) {
         for (j = 0; j < CUBE_TEX_SIZE; j++) {
            if ((i/4 + j/4) & 1) {
               image[i][j][0] = colors[f][0];
               image[i][j][1] = colors[f][1];
               image[i][j][2] = colors[f][2];
            }
            else {
               image[i][j][0] = 255;
               image[i][j][1] = 255;
               image[i][j][2] = 255;
            }
         }
      }

      glTexImage2D(targets[f], 0, GL_RGB, CUBE_TEX_SIZE, CUBE_TEX_SIZE, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, image);
   }
}


static void
LoadFace(GLenum target, const char *filename,
         GLboolean flipTB, GLboolean flipLR)
{
   GLint w, h;
   GLenum format;
   GLubyte *img = LoadRGBImage(filename, &w, &h, &format);
   if (!img) {
      printf("Error: couldn't load texture image %s\n", filename);
      exit(1);
   }
   assert(format == GL_RGB);

   /* <sigh> the way the texture cube mapping works, we have to flip
    * images to make things look right.
    */
   if (flipTB) {
      const int stride = 3 * w;
      GLubyte temp[3*1024];
      int i;
      for (i = 0; i < h / 2; i++) {
         memcpy(temp, img + i * stride, stride);
         memcpy(img + i * stride, img + (h - i - 1) * stride, stride);
         memcpy(img + (h - i - 1) * stride, temp, stride);
      }
   }
   if (flipLR) {
      const int stride = 3 * w;
      GLubyte temp[3];
      GLubyte *row;
      int i, j;
      for (i = 0; i < h; i++) {
         row = img + i * stride;
         for (j = 0; j < w / 2; j++) {
            int k = w - j - 1;
            temp[0] = row[j*3+0];
            temp[1] = row[j*3+1];
            temp[2] = row[j*3+2];
            row[j*3+0] = row[k*3+0];
            row[j*3+1] = row[k*3+1];
            row[j*3+2] = row[k*3+2];
            row[k*3+0] = temp[0];
            row[k*3+1] = temp[1];
            row[k*3+2] = temp[2];
         }
      }
   }

   gluBuild2DMipmaps(target, GL_RGB, w, h, format, GL_UNSIGNED_BYTE, img);
   free(img);
}


static void
LoadEnvmaps(void)
{
   LoadFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, "right.rgb", GL_TRUE, GL_FALSE);
   LoadFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, "left.rgb", GL_TRUE, GL_FALSE);
   LoadFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, "top.rgb", GL_FALSE, GL_TRUE);
   LoadFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, "bottom.rgb", GL_FALSE, GL_TRUE);
   LoadFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, "front.rgb", GL_TRUE, GL_FALSE);
   LoadFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, "back.rgb", GL_TRUE, GL_FALSE);
}


static void
InitTextures(GLboolean useImageFiles)
{
   GLenum filter;

   /*
    * Env map
    */
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_CUBE_MAP, 1);
   if (useImageFiles) {
      LoadEnvmaps();
      filter = GL_LINEAR;
   }
   else {
      InitCheckers();
      filter = GL_NEAREST;
   }
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, filter);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   /*
    * Ground texture
    */
   {
      GLint imgWidth, imgHeight;
      GLenum imgFormat;
      GLubyte *image = NULL;

      image = LoadRGBImage(GroundImage, &imgWidth, &imgHeight, &imgFormat);
      if (!image) {
         printf("Couldn't read %s\n", GroundImage);
         exit(0);
      }

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, 2);
      gluBuild2DMipmaps(GL_TEXTURE_2D, 3, imgWidth, imgHeight,
                        imgFormat, GL_UNSIGNED_BYTE, image);
      free(image);
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   }
}


static void
LoadAndCompileShader(GLuint shader, const char *text)
{
   GLint stat;

   glShaderSource_func(shader, 1, (const GLchar **) &text, NULL);

   glCompileShader_func(shader);

   glGetShaderiv_func(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog_func(shader, 1000, &len, log);
      fprintf(stderr, "%s: problem compiling shader: %s\n", Demo, log);
      exit(1);
   }
   else {
      printf("Shader compiled OK\n");
   }
}


/**
 * Read a shader from a file.
 */
static void
ReadShader(GLuint shader, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "%s: Unable to open shader file %s\n", Demo, filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   printf("%s: read %d bytes from shader file %s\n", Demo, n, filename);
   if (n > 0) {
      buffer[n] = 0;
      LoadAndCompileShader(shader, buffer);
   }

   fclose(f);
   free(buffer);
}


static void
CheckLink(GLuint prog)
{
   GLint stat;
   glGetProgramiv_func(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog_func(prog, 1000, &len, log);
      fprintf(stderr, "Linker error:\n%s\n", log);
   }
   else {
      fprintf(stderr, "Link success!\n");
   }
}


static GLuint
CreateProgram(const char *vertProgFile, const char *fragProgFile,
              struct uniform_info *uniforms)
{
   GLuint fragShader = 0, vertShader = 0, program = 0;
   GLint i;

   program = glCreateProgram_func();
   if (vertProgFile) {
      vertShader = glCreateShader_func(GL_VERTEX_SHADER);
      ReadShader(vertShader, vertProgFile);
      glAttachShader_func(program, vertShader);
   }

   if (fragProgFile) {
      fragShader = glCreateShader_func(GL_FRAGMENT_SHADER);
      ReadShader(fragShader, fragProgFile);
      glAttachShader_func(program, fragShader);
   }

   glLinkProgram_func(program);
   CheckLink(program);

   glUseProgram_func(program);

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));

   CheckError(__LINE__);
   for (i = 0; uniforms[i].name; i++) {
      uniforms[i].location
         = glGetUniformLocation_func(program, uniforms[i].name);
      printf("Uniform %s location: %d\n", uniforms[i].name,
             uniforms[i].location);

      switch (uniforms[i].size) {
      case 1:
         if (uniforms[i].type == GL_INT)
            glUniform1i_func(uniforms[i].location,
                             (GLint) uniforms[i].value[0]);
         else
            glUniform1fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 2:
         glUniform2fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 3:
         glUniform3fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 4:
         glUniform4fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      default:
         abort();
      }
   }

   CheckError(__LINE__);

   return program;
}


static void
InitPrograms(void)
{
   Program1 = CreateProgram(ReflectVertFile, CubeFragFile, ReflectUniforms);
   Program2 = CreateProgram(SimpleVertFile, SimpleTexFragFile, SimpleUniforms);
}


static void
Init(GLboolean useImageFiles)
{
   const char *version = (const char *) glGetString(GL_VERSION);

   if (version[0] != '2' || version[1] != '.') {
      printf("Warning: this program expects OpenGL 2.0\n");
      /*exit(1);*/
   }
   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   GetExtensionFuncs();

   InitTextures(useImageFiles);
   InitPrograms();

   glEnable(GL_DEPTH_TEST);

   glClearColor(.6, .6, .9, 0);
   glColor3f(1.0, 1.0, 1.0);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(500, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   glutCreateWindow(Demo);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(key);
   glutSpecialFunc(specialkey);
   glutDisplayFunc(draw);
   if (Anim)
      glutIdleFunc(idle);
   if (argc > 1 && strcmp(argv[1] , "-i") == 0)
      Init(1);
   else
      Init(0);
   glutMainLoop();
   return 0;
}
