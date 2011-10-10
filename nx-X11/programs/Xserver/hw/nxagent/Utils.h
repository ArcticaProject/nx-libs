/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#ifndef __Utils_H__
#define __Utils_H__

/*
 * Number of bits in fixed point operations.
 */

#define PRECISION  16

/*
 * "1" ratio means "don't scale".
 */

#define DONT_SCALE  (1 << PRECISION)

#define nxagentScale(i, ratio) (((i) * (ratio)) >> (PRECISION))

#ifndef MIN
#define MIN(A, B) ( (A) < (B) ? (A) : (B) )
#endif

#endif /* __Utils_H__ */
