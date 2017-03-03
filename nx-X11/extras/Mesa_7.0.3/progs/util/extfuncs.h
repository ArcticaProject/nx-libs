/**
 * Utility for getting OpenGL extension function pointers
 * Meant to be #included.
 */

/* OpenGL 2.0 */
static PFNGLATTACHSHADERPROC glAttachShader_func = NULL;
static PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation_func = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader_func = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram_func = NULL;
static PFNGLCREATESHADERPROC glCreateShader_func = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram_func = NULL;
static PFNGLDELETESHADERPROC glDeleteShader_func = NULL;
static PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib_func = NULL;
static PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders_func = NULL;
static PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation_func = NULL;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_func = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_func = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv_func = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv_func = NULL;
static PFNGLGETSHADERSOURCEPROC glGetShaderSource_func = NULL;
static PFNGLGETUNIFORMFVPROC glGetUniformfv_func = NULL;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_func = NULL;
static PFNGLISPROGRAMPROC glIsProgram_func = NULL;
static PFNGLISSHADERPROC glIsShader_func = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram_func = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource_func = NULL;
static PFNGLUNIFORM1IPROC glUniform1i_func = NULL;
static PFNGLUNIFORM1FVPROC glUniform1fv_func = NULL;
static PFNGLUNIFORM2FVPROC glUniform2fv_func = NULL;
static PFNGLUNIFORM3FVPROC glUniform3fv_func = NULL;
static PFNGLUNIFORM4FVPROC glUniform4fv_func = NULL;
static PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv_func = NULL;
static PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv_func = NULL;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv_func = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram_func = NULL;
static PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f_func = NULL;
static PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f_func = NULL;
static PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f_func = NULL;
static PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f_func = NULL;

/* OpenGL 2.1 */
static PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv_func = NULL;
static PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv_func = NULL;
static PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv_func = NULL;
static PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv_func = NULL;
static PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv_func = NULL;
static PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv_func = NULL;

/* GL_ARB_vertex/fragment_program */
static PFNGLBINDPROGRAMARBPROC glBindProgramARB_func = NULL;
static PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB_func = NULL;
static PFNGLGENPROGRAMSARBPROC glGenProgramsARB_func = NULL;
static PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB_func = NULL;
static PFNGLISPROGRAMARBPROC glIsProgramARB_func = NULL;
static PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB_func = NULL;
static PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB_func = NULL;
static PFNGLPROGRAMSTRINGARBPROC glProgramStringARB_func = NULL;
static PFNGLVERTEXATTRIB1FARBPROC glVertexAttrib1fARB_func = NULL;

/* GL_APPLE_vertex_array_object */
static PFNGLBINDVERTEXARRAYAPPLEPROC glBindVertexArrayAPPLE_func = NULL;
static PFNGLDELETEVERTEXARRAYSAPPLEPROC glDeleteVertexArraysAPPLE_func = NULL;
static PFNGLGENVERTEXARRAYSAPPLEPROC glGenVertexArraysAPPLE_func = NULL;
static PFNGLISVERTEXARRAYAPPLEPROC glIsVertexArrayAPPLE_func = NULL;

/* GL_EXT_stencil_two_side */
static PFNGLACTIVESTENCILFACEEXTPROC glActiveStencilFaceEXT_func = NULL;


static void
GetExtensionFuncs(void)
{
   /* OpenGL 2.0 */
   glAttachShader_func = (PFNGLATTACHSHADERPROC) glutGetProcAddress("glAttachShader");
   glBindAttribLocation_func = (PFNGLBINDATTRIBLOCATIONPROC) glutGetProcAddress("glBindAttribLocation");
   glCompileShader_func = (PFNGLCOMPILESHADERPROC) glutGetProcAddress("glCompileShader");
   glCreateProgram_func = (PFNGLCREATEPROGRAMPROC) glutGetProcAddress("glCreateProgram");
   glCreateShader_func = (PFNGLCREATESHADERPROC) glutGetProcAddress("glCreateShader");
   glDeleteProgram_func = (PFNGLDELETEPROGRAMPROC) glutGetProcAddress("glDeleteProgram");
   glDeleteShader_func = (PFNGLDELETESHADERPROC) glutGetProcAddress("glDeleteShader");
   glGetActiveAttrib_func = (PFNGLGETACTIVEATTRIBPROC) glutGetProcAddress("glGetActiveAttrib");
   glGetAttachedShaders_func = (PFNGLGETATTACHEDSHADERSPROC) glutGetProcAddress("glGetAttachedShaders");
   glGetAttribLocation_func = (PFNGLGETATTRIBLOCATIONPROC) glutGetProcAddress("glGetAttribLocation");
   glGetProgramInfoLog_func = (PFNGLGETPROGRAMINFOLOGPROC) glutGetProcAddress("glGetProgramInfoLog");
   glGetShaderInfoLog_func = (PFNGLGETSHADERINFOLOGPROC) glutGetProcAddress("glGetShaderInfoLog");
   glGetProgramiv_func = (PFNGLGETPROGRAMIVPROC) glutGetProcAddress("glGetProgramiv");
   glGetShaderiv_func = (PFNGLGETSHADERIVPROC) glutGetProcAddress("glGetShaderiv");
   glGetShaderSource_func = (PFNGLGETSHADERSOURCEPROC) glutGetProcAddress("glGetShaderSource");
   glGetUniformLocation_func = (PFNGLGETUNIFORMLOCATIONPROC) glutGetProcAddress("glGetUniformLocation");
   glGetUniformfv_func = (PFNGLGETUNIFORMFVPROC) glutGetProcAddress("glGetUniformfv");
   glIsProgram_func = (PFNGLISPROGRAMPROC) glutGetProcAddress("glIsProgram");
   glIsShader_func = (PFNGLISSHADERPROC) glutGetProcAddress("glIsShader");
   glLinkProgram_func = (PFNGLLINKPROGRAMPROC) glutGetProcAddress("glLinkProgram");
   glShaderSource_func = (PFNGLSHADERSOURCEPROC) glutGetProcAddress("glShaderSource");
   glUniform1i_func = (PFNGLUNIFORM1IPROC) glutGetProcAddress("glUniform1i");
   glUniform1fv_func = (PFNGLUNIFORM1FVPROC) glutGetProcAddress("glUniform1fv");
   glUniform2fv_func = (PFNGLUNIFORM2FVPROC) glutGetProcAddress("glUniform2fv");
   glUniform3fv_func = (PFNGLUNIFORM3FVPROC) glutGetProcAddress("glUniform3fv");
   glUniform4fv_func = (PFNGLUNIFORM3FVPROC) glutGetProcAddress("glUniform4fv");
   glUniformMatrix2fv_func = (PFNGLUNIFORMMATRIX2FVPROC) glutGetProcAddress("glUniformMatrix2fv");
   glUniformMatrix3fv_func = (PFNGLUNIFORMMATRIX3FVPROC) glutGetProcAddress("glUniformMatrix3fv");
   glUniformMatrix4fv_func = (PFNGLUNIFORMMATRIX4FVPROC) glutGetProcAddress("glUniformMatrix4fv");
   glUseProgram_func = (PFNGLUSEPROGRAMPROC) glutGetProcAddress("glUseProgram");
   glVertexAttrib1f_func = (PFNGLVERTEXATTRIB1FPROC) glutGetProcAddress("glVertexAttrib1f");
   glVertexAttrib2f_func = (PFNGLVERTEXATTRIB2FPROC) glutGetProcAddress("glVertexAttrib2f");
   glVertexAttrib3f_func = (PFNGLVERTEXATTRIB3FPROC) glutGetProcAddress("glVertexAttrib3f");
   glVertexAttrib4f_func = (PFNGLVERTEXATTRIB4FPROC) glutGetProcAddress("glVertexAttrib4f");

   /* OpenGL 2.1 */
   glUniformMatrix2x3fv_func = (PFNGLUNIFORMMATRIX2X3FVPROC) glutGetProcAddress("glUniformMatrix2x3fv");
   glUniformMatrix3x2fv_func = (PFNGLUNIFORMMATRIX3X2FVPROC) glutGetProcAddress("glUniformMatrix3x2fv");
   glUniformMatrix2x4fv_func = (PFNGLUNIFORMMATRIX2X4FVPROC) glutGetProcAddress("glUniformMatrix2x4fv");
   glUniformMatrix4x2fv_func = (PFNGLUNIFORMMATRIX4X2FVPROC) glutGetProcAddress("glUniformMatrix4x2fv");
   glUniformMatrix3x4fv_func = (PFNGLUNIFORMMATRIX3X4FVPROC) glutGetProcAddress("glUniformMatrix3x4fv");
   glUniformMatrix4x3fv_func = (PFNGLUNIFORMMATRIX4X3FVPROC) glutGetProcAddress("glUniformMatrix4x3fv");

   /* GL_ARB_vertex/fragment_program */
   glBindProgramARB_func = (PFNGLBINDPROGRAMARBPROC) glutGetProcAddress("glBindProgramARB");
   glDeleteProgramsARB_func = (PFNGLDELETEPROGRAMSARBPROC) glutGetProcAddress("glDeleteProgramsARB");
   glGenProgramsARB_func = (PFNGLGENPROGRAMSARBPROC) glutGetProcAddress("glGenProgramsARB");
   glGetProgramLocalParameterdvARB_func = (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) glutGetProcAddress("glGetProgramLocalParameterdvARB");
   glIsProgramARB_func = (PFNGLISPROGRAMARBPROC) glutGetProcAddress("glIsProgramARB");
   glProgramLocalParameter4dARB_func = (PFNGLPROGRAMLOCALPARAMETER4DARBPROC) glutGetProcAddress("glProgramLocalParameter4dARB");
   glProgramLocalParameter4fvARB_func = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) glutGetProcAddress("glProgramLocalParameter4fvARB");
   glProgramStringARB_func = (PFNGLPROGRAMSTRINGARBPROC) glutGetProcAddress("glProgramStringARB");
   glVertexAttrib1fARB_func = (PFNGLVERTEXATTRIB1FARBPROC) glutGetProcAddress("glVertexAttrib1fARB");

   /* GL_APPLE_vertex_array_object */
   glBindVertexArrayAPPLE_func = (PFNGLBINDVERTEXARRAYAPPLEPROC) glutGetProcAddress("glBindVertexArrayAPPLE");
   glDeleteVertexArraysAPPLE_func = (PFNGLDELETEVERTEXARRAYSAPPLEPROC) glutGetProcAddress("glDeleteVertexArraysAPPLE");
   glGenVertexArraysAPPLE_func = (PFNGLGENVERTEXARRAYSAPPLEPROC) glutGetProcAddress("glGenVertexArraysAPPLE");
   glIsVertexArrayAPPLE_func = (PFNGLISVERTEXARRAYAPPLEPROC) glutGetProcAddress("glIsVertexArrayAPPLE");

   /* GL_EXT_stencil_two_side */
   glActiveStencilFaceEXT_func = (PFNGLACTIVESTENCILFACEEXTPROC) glutGetProcAddress("glActiveStencilFaceEXT");
}

