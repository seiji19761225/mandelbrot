/*
 * palette.c
 * (c)2010,2015 Seiji Nishimura, Original code was written by Chisato Yamauchi.
 * $Id: palette.c,v 1.1.1.2 2015/11/27 00:00:00 seiji Exp seiji $
 */

#include "pixmap.h"
#include "palette_internal.h"

#define DS9_BEGIN	0
#define DS9_NUM		17
#define IDL1_BEGIN	( DS9_BEGIN+ DS9_NUM)
#define IDL1_NUM	16
#define IDL2_BEGIN	(IDL1_BEGIN+IDL1_NUM)
#define IDL2_NUM	26

#define IS_DS9(id)	((id)>= DS9_BEGIN&&(id)<( DS9_BEGIN+ DS9_NUM))
#define IS_IDL1(id)	((id)>=IDL1_BEGIN&&(id)<(IDL1_BEGIN+IDL1_NUM))
#define IS_IDL2(id)	((id)>=IDL2_BEGIN&&(id)<(IDL2_BEGIN+IDL2_NUM))

#define RINT(x)			((int) (x))
#define RANGE(x,min,max)	(((x)>(max))?(max):((x)<(min))?(min):(x))

/*======================================================================*/
pixel_t palette(int id, double dmin, double dmax, double data)
{				/* color palette function               */
    int cn, rr = 0x00, gg = 0x00, bb = 0x00;
    double nd, d_max, d_min;

    d_max = (dmax > dmin) ? dmax : dmin;
    d_min = (dmax < dmin) ? dmax : dmin;

    data = RANGE(data, d_min, d_max);
    data = ((dmax < dmin) ? d_max - data : data - d_min) / (d_max - d_min);

    if (IS_DS9(id)) {
	if (DS9_GREY <= id && id <= DS9_BLUE) {
	    cn = RINT(RANGE(256.0 * data, 0.0, 255.0));
	    if (id == DS9_GREY) {
		rr = gg = bb = cn;
	    } else if (id == DS9_RED) {
		rr = cn;
	    } else if (id == DS9_GREEN) {
		gg = cn;
	    } else if (id == DS9_BLUE) {
		bb = cn;
	    }
	} else if ((DS9_A   <= id && id <= DS9_HE     ) ||
		   (DS9_SLS <= id && id <= DS9_RAINBOW)) {
	    if (DS9_SLS - DS9_A <= (id -= DS9_A))
		id -= DS9_SLS - DS9_HE - 1;
	    if ((cn = RINT(nd = RANGE(613.0 * data, 0.0, 612.0))) == 612 ||
		(Ds9_cl1[id].flags & F_DIRECT)) {
		rr = Ds9_cl1[id].c[cn].r;
		gg = Ds9_cl1[id].c[cn].g;
		bb = Ds9_cl1[id].c[cn].b;
	    } else {
		rr = RINT((nd - cn) * Ds9_cl1[id].c[cn + 1].r +
			  (cn - nd + 1) * Ds9_cl1[id].c[cn].r);
		gg = RINT((nd - cn) * Ds9_cl1[id].c[cn + 1].g +
			  (cn - nd + 1) * Ds9_cl1[id].c[cn].g);
		bb = RINT((nd - cn) * Ds9_cl1[id].c[cn + 1].b +
			  (cn - nd + 1) * Ds9_cl1[id].c[cn].b);
	    }
	} else if (id == DS9_I8) {
	    const int c_r[8] = { 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff },
		      c_g[8] = { 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff },
		      c_b[8] = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff };
	    rr = c_r[cn = RINT(RANGE(8.0 * data, 0.0, 7.0))];
	    gg = c_g[cn];
	    bb = c_b[cn];
	} else if (id == DS9_AIPS0) {
	    const int c_r[9] = { 0x31, 0x79, 0x00, 0x5f, 0x00, 0x00, 0xff, 0xff, 0xff },
		      c_g[9] = { 0x31, 0x00, 0x00, 0xa7, 0x97, 0xf6, 0xff, 0xb0, 0x00 },
		      c_b[9] = { 0x31, 0x9b, 0xc8, 0xeb, 0x00, 0x00, 0x00, 0x00, 0x00 };
	    rr = c_r[cn = RINT(RANGE(9.0 * data, 0.0, 8.0))];
	    gg = c_g[cn];
	    bb = c_b[cn];
	} else if (id == DS9_STANDARD) {
	    if ((nd = RANGE(3.0 * data, 0.0, 3.0)) < 1.0) {
		rr = gg = RINT(0x04d * nd);
		bb =      RINT(0x0fd * nd + 1.0);
	    } else if (nd < 2.0) {
		rr = bb = RINT(0x04d * (nd - 1.0));
		gg =      RINT(0x04c + (nd - 1.0) * (0x0ff - 0x04c));
	    } else {
		rr =      RINT(0x04c + (nd - 2.0) * (0x0fe - 0x04c));
		gg = bb = RINT(0x04c * (nd - 2.0));
	    }
	} else if (id == DS9_STAIRCASE) {
	    const int c_r[15] = {
		0x0f, 0x1e, 0x2d, 0x3d, 0x4c, 0x0f, 0x1e, 0x2d,
		0x3d, 0x4c, 0x33, 0x66, 0x99, 0xcc, 0xff
	    },	      c_g[15] = {
		0x0f, 0x1e, 0x2d, 0x3d, 0x4c, 0x33, 0x66, 0x99,
		0xcc, 0xff, 0x0f, 0x1e, 0x2d, 0x3d, 0x4c
	    },	      c_b[15] = {
		0x33, 0x66, 0x99, 0xcc, 0xff, 0x0f, 0x1e, 0x2d,
		0x3d, 0x4c, 0x0f, 0x1e, 0x2d, 0x3d, 0x4c
	    };
	    rr = c_r[cn = RINT(RANGE(15.0 * data, 0.0, 14.0))];
	    gg = c_g[cn];
	    bb = c_b[cn];
	} else if (id == DS9_COLOR) {
	    const int c_r[16] = {
		0x00, 0x2e, 0x5f, 0x8e, 0xbf, 0xee, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x4e, 0x7f, 0x9f, 0xee, 0xbf
	    },	      c_g[16] = {
		0x00, 0x2e, 0x5f, 0x8e, 0xbf, 0xee, 0x2e, 0x5f,
		0x7f, 0xbf, 0xee, 0x9f, 0x7f, 0x4e, 0x00, 0x00
	    },	      c_b[16] = {
		0x00, 0x2e, 0x5f, 0x8e, 0xbf, 0xee, 0xee, 0xbf,
		0x7f, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4e
	    };
	    rr = c_r[cn = RINT(RANGE(16.0 * data, 0.0, 15.0))];
	    gg = c_g[cn];
	    bb = c_b[cn];
	}
    } else if (IS_IDL1(id) || IS_IDL2(id)) {
	struct idl_color *pt;
	pt  = (struct idl_color *) ((IDL2_BEGIN <= id) ? Idl_cl2    : Idl_cl1   );
	id -=                      ((IDL2_BEGIN <= id) ? IDL2_BEGIN : IDL1_BEGIN);
	if ((cn = RINT(nd = RANGE(255.0 * data, 0.0, 254.0))) == 254 ||
	    (pt[id].flags & F_DIRECT)) {
	    rr = pt[id].c[cn].r;
	    gg = pt[id].c[cn].g;
	    bb = pt[id].c[cn].b;
	} else {
	    rr = RINT((nd - cn) * pt[id].c[cn + 1].r +
		      (cn - nd + 1) * pt[id].c[cn].r);
	    gg = RINT((nd - cn) * pt[id].c[cn + 1].g +
		      (cn - nd + 1) * pt[id].c[cn].g);
	    bb = RINT((nd - cn) * pt[id].c[cn + 1].b +
		      (cn - nd + 1) * pt[id].c[cn].b);
	}
    }

    return pixel_set_rgb(rr, gg, bb);
}
