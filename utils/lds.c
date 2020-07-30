/*
 * lds.c: LDS (Low Discrepancy Sequence) for QMC (Quasi Monte-Carlo)
 * (c)2010,2015 Seiji Nishimura
 * $Id: lds.c,v 1.1.1.1 2015/02/26 00:00:00 seiji Exp seiji $
 */

#define _LDS_INTERNAL_

#include "lds.h"

// bit size of integer == log2(UINT_MAX)
#define LOG2_UINT_MAX	(sizeof(unsigned int)*8)

//======================================================================
double lds_vdc(unsigned int n, unsigned int radix)
{				// return n-th element of van der Corput seq.
    int i;
    unsigned int rn;
    double vdc, dstack[LOG2_UINT_MAX];

    if (radix == 2) {
	for (i = 0, rn = 1; n != 0x00; n >>= 0x01)
	    dstack[i++] = ((double) (n & 0x01)) / (rn <<= 0x01);
    } else {
	for (i = 0, rn = 1; n != 0x00; n /= radix)
	    dstack[i++] = ((double) (n % radix)) / (rn *= radix);
    }

    vdc = 0.0;
    while (--i >= 0)		// sum from the lowest
	vdc += dstack[i];

    return vdc;
}
