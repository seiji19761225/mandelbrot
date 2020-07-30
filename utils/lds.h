/*
 * lds.h: LDS (Low Discrepancy Sequence) for QMC (Quasi Monte-Carlo)
 * (c)2010 Seiji Nishimura
 * $Id: lds.h,v 1.1.1.1 2015/02/26 00:00:00 seiji Exp seiji $
 */

#ifndef __LDS_H__
#define __LDS_H__

#ifdef _LDS_INTERNAL_
#define LDS_API
#else
#define LDS_API	extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

LDS_API double lds_vdc(unsigned int, unsigned int);

#ifdef __cplusplus
}
#endif

#undef LDS_API
#endif
