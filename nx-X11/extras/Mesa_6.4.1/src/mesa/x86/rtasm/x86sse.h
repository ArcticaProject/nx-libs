
#ifndef _X86SSE_H_
#define _X86SSE_H_

#if defined(USE_X86_ASM)

#include "glheader.h"

/* It is up to the caller to ensure that instructions issued are
 * suitable for the host cpu.  There are no checks made in this module
 * for mmx/sse/sse2 support on the cpu.
 */
struct x86_reg {
   GLuint file:3;
   GLuint idx:3;
   GLuint mod:2;		/* mod_REG if this is just a register */
   GLint  disp:24;		/* only +/- 23bits of offset - should be enough... */
};

struct x86_function {
   GLubyte *store;
   GLubyte *csr;
   GLuint stack_offset;
   GLint need_emms;
   const char *fn;
};

enum x86_reg_file {
   file_REG32,
   file_MMX,
   file_XMM,
   file_x87
};

/* Values for mod field of modr/m byte
 */
enum x86_reg_mod {
   mod_INDIRECT,
   mod_DISP8,
   mod_DISP32,
   mod_REG
};

enum x86_reg_name {
   reg_AX,
   reg_CX,
   reg_DX,
   reg_BX,
   reg_SP,
   reg_BP,
   reg_SI,
   reg_DI
};


enum x86_cc {
   cc_O,			/* overflow */
   cc_NO,			/* not overflow */
   cc_NAE,			/* not above or equal / carry */
   cc_AE,			/* above or equal / not carry */
   cc_E,			/* equal / zero */
   cc_NE			/* not equal / not zero */
};

enum sse_cc {
   cc_Equal,
   cc_LessThan,
   cc_LessThanEqual,
   cc_Unordered,
   cc_NotEqual,
   cc_NotLessThan,
   cc_NotLessThanEqual,
   cc_Ordered
};

#define cc_Z  cc_E
#define cc_NZ cc_NE

/* Begin/end/retreive function creation:
 */


void x86_init_func( struct x86_function *p );
void x86_release_func( struct x86_function *p );
void (*x86_get_func( struct x86_function *p ))( void );



/* Create and manipulate registers and regmem values:
 */
struct x86_reg x86_make_reg( enum x86_reg_file file,
			     enum x86_reg_name idx );

struct x86_reg x86_make_disp( struct x86_reg reg,
			      GLint disp );

struct x86_reg x86_deref( struct x86_reg reg );

struct x86_reg x86_get_base_reg( struct x86_reg reg );


/* Labels, jumps and fixup:
 */
GLubyte *x86_get_label( struct x86_function *p );

void x86_jcc( struct x86_function *p,
	      enum x86_cc cc,
	      GLubyte *label );

GLubyte *x86_jcc_forward( struct x86_function *p,
			  enum x86_cc cc );

void x86_fixup_fwd_jump( struct x86_function *p,
			 GLubyte *fixup );


/* Macro for sse_shufps() and sse2_pshufd():
 */
#define SHUF(_x,_y,_z,_w)       (((_x)<<0) | ((_y)<<2) | ((_z)<<4) | ((_w)<<6))
#define SHUF_NOOP               RSW(0,1,2,3)
#define GET_SHUF(swz, idx)      (((swz) >> ((idx)*2)) & 0x3)

void mmx_emms( struct x86_function *p );
void mmx_movd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void mmx_movq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void mmx_packssdw( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void mmx_packuswb( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse2_cvtps2dq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_movd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_packssdw( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_packsswb( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_packuswb( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_pshufd( struct x86_function *p, struct x86_reg dest, struct x86_reg arg0, GLubyte shuf );
void sse2_rcpss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse_addps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_addss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_cvtps2pi( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_divss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_andps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_cmpps( struct x86_function *p, struct x86_reg dst, struct x86_reg src, GLubyte cc );
void sse_maxps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_minps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movaps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movhlps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movhps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movlhps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movlps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movups( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_mulps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_subps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_rsqrtss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_shufps( struct x86_function *p, struct x86_reg dest, struct x86_reg arg0, GLubyte shuf );

void x86_cmp( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_dec( struct x86_function *p, struct x86_reg reg );
void x86_inc( struct x86_function *p, struct x86_reg reg );
void x86_lea( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_mov( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_pop( struct x86_function *p, struct x86_reg reg );
void x86_push( struct x86_function *p, struct x86_reg reg );
void x86_ret( struct x86_function *p );
void x86_test( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_xor( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_sahf( struct x86_function *p );

void x87_f2xm1( struct x86_function *p );
void x87_fabs( struct x86_function *p );
void x87_fadd( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_faddp( struct x86_function *p, struct x86_reg dst );
void x87_fchs( struct x86_function *p );
void x87_fclex( struct x86_function *p );
void x87_fcom( struct x86_function *p, struct x86_reg dst );
void x87_fcomp( struct x86_function *p, struct x86_reg dst );
void x87_fcos( struct x86_function *p );
void x87_fdiv( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fdivp( struct x86_function *p, struct x86_reg dst );
void x87_fdivr( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fdivrp( struct x86_function *p, struct x86_reg dst );
void x87_fist( struct x86_function *p, struct x86_reg dst );
void x87_fistp( struct x86_function *p, struct x86_reg dst );
void x87_fld( struct x86_function *p, struct x86_reg arg );
void x87_fld1( struct x86_function *p );
void x87_fldcw( struct x86_function *p, struct x86_reg arg );
void x87_fldl2e( struct x86_function *p );
void x87_fldln2( struct x86_function *p );
void x87_fldz( struct x86_function *p );
void x87_fmul( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fmulp( struct x86_function *p, struct x86_reg dst );
void x87_fnclex( struct x86_function *p );
void x87_fprndint( struct x86_function *p );
void x87_fscale( struct x86_function *p );
void x87_fsin( struct x86_function *p );
void x87_fsincos( struct x86_function *p );
void x87_fsqrt( struct x86_function *p );
void x87_fst( struct x86_function *p, struct x86_reg dst );
void x87_fstp( struct x86_function *p, struct x86_reg dst );
void x87_fsub( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fsubp( struct x86_function *p, struct x86_reg dst );
void x87_fsubr( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fsubrp( struct x86_function *p, struct x86_reg dst );
void x87_fxch( struct x86_function *p, struct x86_reg dst );
void x87_fxtract( struct x86_function *p );
void x87_fyl2x( struct x86_function *p );
void x87_fyl2xp1( struct x86_function *p );
void x87_fwait( struct x86_function *p );
void x87_fnstsw( struct x86_function *p, struct x86_reg dst );
void x87_fucompp( struct x86_function *p );
void x87_fucomp( struct x86_function *p, struct x86_reg arg );
void x87_fucom( struct x86_function *p, struct x86_reg arg );



/* Retreive a reference to one of the function arguments, taking into
 * account any push/pop activity.  Note - doesn't track explict
 * manipulation of ESP by other instructions.
 */
struct x86_reg x86_fn_arg( struct x86_function *p, GLuint arg );

#endif
#endif
