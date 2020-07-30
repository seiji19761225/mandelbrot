/*
 * kernel.cl: Vectorized OpenCL kernel
 * (c)2017-2018 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.2 2018/09/09 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define VLEN	4
#define BAILOUT	4.0

// for AMR
#define MIN_GRID	(0x01<<2)
#define MAX_GRID	(0x01<<8)

inline int4    mandelbrot      (int, double4, double4);
inline bool    detect_edge     (__global uchar *, uchar4 *, int, int, int, int);
inline bool    equivalent_color(uchar4, uchar4);
inline uchar4  pixmap_get_pixel(__global uchar *, int, int, int);
inline uchar16 colormap_lookup (__global uchar *, int4);

//----------------------------------------------------------------------
__kernel void rough_sketch_GPU
	(__global uchar *pixmap  , int width, int height,
	 __global uchar *colormap, int iter_max, double c_r, double c_i, double radius)
{
    int     x = get_global_id(0) * VLEN,
	    y = get_global_id(1);
    double  d = 2.0 * radius / min(width, height);
    double4 p_r, p_i;

    if (x > width - VLEN)	// to deal with vector remainder
	x = width - VLEN;

    p_r = c_r + d * convert_double4(x + (int4) (0, 1, 2, 3) - width / 2);
    p_i = c_i + d * convert_double (height / 2 - y);

    int4     iter  = mandelbrot(iter_max, p_r, p_i);

    uchar16  pixel = colormap_lookup(colormap, iter % iter_max);
#if        SIZEOF_PIXEL_T == 3
    vstore3 (pixel.s123, x + 0 + y * width, pixmap);
    vstore3 (pixel.s567, x + 1 + y * width, pixmap);
    vstore3 (pixel.s9ab, x + 2 + y * width, pixmap);
    vstore3 (pixel.sdef, x + 3 + y * width, pixmap);
#else	// SIZEOF_PIXEL_T == 4
    vstore16(pixel, 0, pixmap + SIZEOF_PIXEL_T * (x + y * width));
#endif

    return;
}

//----------------------------------------------------------------------
__kernel void antialiasing_GPU
	(__global uchar *pixmap  , __global uchar *sketch, int width, int height,
	 __global uchar *colormap, int iter_max, double c_r, double c_i, double radius)
{
#if 0
    int x = get_global_id(0),
	y = get_global_id(1);
#else
    int x = get_group_id(0) + get_num_groups(0) * get_local_id(0),
	y = get_group_id(1) + get_num_groups(1) * get_local_id(1);
#endif
    uchar4 pixel;

    if (detect_edge(sketch, &pixel, x, y, width, height)) {	// over-sampling for edge
	uchar4 average = pixel;
	int4   sum     = 0;
	double d = 2.0 * radius / min(width, height);
	c_r += d * (x - width  / 2),
	c_i += d * (height / 2 - y);
	for (int ngrid = MIN_GRID; ngrid <= MAX_GRID; ngrid <<= 0x01) {
	    int16 sum4 = 0;
	    int   ng2  = ngrid * ngrid;
	    double e   = d     / ngrid;
	    for (int jj = 0; jj < 2; jj++)
		for (int ii = 0; ii < 2; ii++)
		    if (((ii | jj) & 0x01) ||	// skip redundant points: (ii % 2) != 0 || (jj % 2) != 0
			ngrid == MIN_GRID)
			for (int k = 0; k < ng2 / 4; k += VLEN) {
			    int4    m    = 2 * ((k + (int4) (0, 1, 2, 3)) % (ngrid / 2)) + ii,
				    n    = 2 * ((k + (int4) (0, 1, 2, 3)) / (ngrid / 2)) + jj;
			    double4 p_r  = c_r + e * convert_double4(m),
				    p_i  = c_i - e * convert_double4(n);
			    int4    iter = mandelbrot(iter_max, p_r, p_i);
			    sum4 += convert_int16(colormap_lookup(colormap, iter % iter_max));
			}
	    sum    += sum4.s0123 + sum4.s4567 + sum4.s89ab + sum4.scdef;
	    pixel   = average;
	    average = convert_uchar4_sat((sum + (ng2 >> 1)) / ng2);
	    if (equivalent_color(average, pixel))
		break;
	}
	pixel = average;
    }

#if        SIZEOF_PIXEL_T == 3
    vstore3(pixel.s123, x + y * width, pixmap);
#else	// SIZEOF_PIXEL_T == 4
    vstore4(pixel     , x + y * width, pixmap);
#endif

    return;
}

//----------------------------------------------------------------------
inline int4 mandelbrot(int iter_max, double4 p_r, double4 p_i)
{
    int4    i, mask;
    double4 z_r, z_i, work;

    work = 2.0 * p_r * p_i;
    z_r  = p_r * p_r;
    z_i  = p_i * p_i;
    mask = convert_int4(isless(z_r + z_i, BAILOUT));	// (z_r + z_i < BAILOUT) ? -1 : 0;
    i    = -mask;

    for (int k = 1; k < iter_max && any(mask); k++) {
	z_r += p_r - z_i ;
	z_i  = p_i + work;
	work = 2.0 * z_r * z_i;
	z_r *= z_r;
	z_i *= z_i;
	mask = convert_int4(isless(z_r + z_i, BAILOUT));
	i   -= mask;
    }

    return i;
}

//----------------------------------------------------------------------
inline bool detect_edge(__global uchar *pixmap, uchar4 *pixel, int x, int y, int width, int height)
{
    *pixel = pixmap_get_pixel(pixmap, x, y, width);

    for (int j = max(0, y - 1); j <= min(height - 1, y + 1); j++)
	for (int i = max(0, x - 1); i <= min(width - 1, x + 1); i++)
	    if (i != x || j != y) {
		uchar4 p = pixmap_get_pixel(pixmap, i, j, width);
		if (!equivalent_color(*pixel, p))
		    return true;
	    }

    return false;
}

//----------------------------------------------------------------------
inline bool equivalent_color(uchar4 p, uchar4 q)
#ifdef USE_SAME_COLOR
{
    return p.s1 == q.s1 &&
	   p.s2 == q.s2 &&
	   p.s3 == q.s3;
}
#else				//......................................
{
    return 3 * abs(p.s1 - q.s1) +
	   6 * abs(p.s2 - q.s2) +
	   1 * abs(p.s3 - q.s3) < 15;
}
#endif

//----------------------------------------------------------------------
inline uchar4 pixmap_get_pixel(__global uchar *pixmap, int x, int y, int stride)
#if        SIZEOF_PIXEL_T == 3
{
    return (uchar4) ((uchar) 0x00, vload3(x + y * stride, pixmap));
}
#else	// SIZEOF_PIXEL_T == 4 .........................................
{
    return vload4(x + y * stride, pixmap);
}
#endif

//----------------------------------------------------------------------
inline uchar16 colormap_lookup(__global uchar *colormap, int4 index)
#if        SIZEOF_PIXEL_T == 3
{
    return (uchar16) ((uchar) 0x00, vload3(index.s0, colormap),
		      (uchar) 0x00, vload3(index.s1, colormap),
		      (uchar) 0x00, vload3(index.s2, colormap),
		      (uchar) 0x00, vload3(index.s3, colormap));
}
#else	// SIZEOF_PIXEL_T == 4 .........................................
{
    return (uchar16) (vload4(index.s0, colormap),
		      vload4(index.s1, colormap),
		      vload4(index.s2, colormap),
		      vload4(index.s3, colormap));
}
#endif
