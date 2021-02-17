/* ecc-dup-jj.c

   Copyright (C) 2013 Niels Möller

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see http://www.gnu.org/licenses/.
*/

/* Development of Nettle's ECC support was funded by the .SE Internet Fund. */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "ecc.h"
#include "ecc-internal.h"

/* NOTE: Behaviour for corner cases:

   + p = 0  ==>  r = 0, correct!
*/
void
ecc_dup_jj (const struct ecc_curve *ecc,
	    mp_limb_t *r, const mp_limb_t *p,
	    mp_limb_t *scratch)
{
  /* Formulas (from djb,
     http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#doubling-dbl-2001-b):

     Computation			Operation	Live variables
     delta = z^2			sqr		delta
     gamma = y^2			sqr		delta, gamma
     z' = (y+z)^2-gamma-delta		sqr		delta, gamma
     alpha = 3*(x-delta)*(x+delta)	mul		gamma, beta, alpha
     beta = x*gamma			mul		gamma, beta, alpha
     x' = alpha^2-8*beta		sqr		gamma, beta, alpha
     y' = alpha*(4*beta-x')-8*gamma^2	mul, sqr
  */

#define delta  scratch
#define gamma (scratch + ecc->p.size)
#define beta  (scratch + 2*ecc->p.size)
#define g2    (scratch + 3*ecc->p.size)
#define sum   (scratch + 4*ecc->p.size)
#define alpha  scratch /* Overlap delta */
  
#define xp p
#define yp (p + ecc->p.size)
#define zp (p + 2*ecc->p.size)
  
  /* delta */
  ecc_mod_sqr (&ecc->p, delta, zp);

  /* gamma */
  ecc_mod_sqr (&ecc->p, gamma, yp);

  /* z'. Can use beta area as scratch. */
  ecc_mod_add (&ecc->p, r + 2*ecc->p.size, yp, zp);
  ecc_mod_sqr (&ecc->p, beta, r + 2*ecc->p.size);
  ecc_mod_sub (&ecc->p, beta, beta, gamma);
  ecc_mod_sub (&ecc->p, r + 2*ecc->p.size, beta, delta);
  
  /* alpha. Can use beta area as scratch, and overwrite delta. */
  ecc_mod_add (&ecc->p, sum, xp, delta);
  ecc_mod_sub (&ecc->p, delta, xp, delta);
  ecc_mod_mul (&ecc->p, beta, sum, delta);
  ecc_mod_mul_1 (&ecc->p, alpha, beta, 3);

  /* beta */
  ecc_mod_mul (&ecc->p, beta, xp, gamma);

  /* Do gamma^2 and 4*beta early, to get them out of the way. We can
     then use the old area at gamma as scratch. */
  ecc_mod_sqr (&ecc->p, g2, gamma);
  ecc_mod_mul_1 (&ecc->p, sum, beta, 4);
  
  /* x' */
  ecc_mod_sqr (&ecc->p, gamma, alpha);   /* Overwrites gamma and beta */
  ecc_mod_submul_1 (&ecc->p, gamma, sum, 2);
  mpn_copyi (r, gamma, ecc->p.size);

  /* y' */
  ecc_mod_sub (&ecc->p, sum, sum, r);
  ecc_mod_mul (&ecc->p, gamma, sum, alpha);
  ecc_mod_submul_1 (&ecc->p, gamma, g2, 8);
  mpn_copyi (r + ecc->p.size, gamma, ecc->p.size);
}
