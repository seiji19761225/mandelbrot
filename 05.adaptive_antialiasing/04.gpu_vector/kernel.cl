/*
 * kernel.cl: Vectorized OpenCL kernel
 * (c)2017-2018,2021 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.5 2021/07/20 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define VLEN	4
#define BAILOUT	4.0

// for adaptive anti-aliasing
#define MIN_SAMPLES	(0x01<<4)
#define MAX_SAMPLES	(0x01<<16)

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

    p_r = c_r + d * convert_double4(x - width  / 2 + (int4) (0, 1, 2, 3));
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
	 __global uchar *colormap, int iter_max, double c_r, double c_i, double radius, __global double *dx, __global double *dy)
{
#if 1
    int x = get_global_id(0),
	y = get_global_id(1);
#else
    int x = get_group_id(0) + get_num_groups(0) * get_local_id(0),
	y = get_group_id(1) + get_num_groups(1) * get_local_id(1);
#endif
    uchar4 pixel;

    if (detect_edge(sketch, &pixel, x, y, width, height)) {	// over-sampling for edge
	uchar4 average  = pixel;
	int4   sum      = 0;
	int    m = 0, n = MIN_SAMPLES;
	double d = 2.0 * radius / min(width, height);
	c_r += d * (x - width  / 2),
	c_i += d * (height / 2 - y);
	do {
	    int16 sum4 = 0;
	    for (int k = m; k < n; k += VLEN) {
		double4 p_r = c_r + d * vload4(k >> 2, dx),
			p_i = c_i - d * vload4(k >> 2, dy);
		int4   iter = mandelbrot(iter_max, p_r, p_i);
		sum4 += convert_int16(colormap_lookup(colormap, iter % iter_max));
	    }
	    sum    += sum4.s0123 + sum4.s4567 + sum4.s89ab + sum4.scdef;
	    pixel   = average;
	    average = convert_uchar4_sat((sum + (n >> 1)) / n);
	} while (!equivalent_color(average, pixel) &&
			(n = (m = n) << 0x01) <= MAX_SAMPLES);
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
#if 1
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
#else				//......................................
{
    bool edge = false;

    *pixel = pixmap_get_pixel(pixmap, x, y, width);

    for (int j = max(0, y - 1); j <= min(height - 1, y + 1); j++)
	for (int i = max(0, x - 1); i <= min(width - 1, x + 1); i++)
	    if (i != x || j != y) {
		uchar4 p = pixmap_get_pixel(pixmap, i, j, width);
		if (!equivalent_color(*pixel, p))
		    edge = true;
	    }

    return edge;
}
#endif

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
    uchar dr, dg, db;

    dr = abs_diff(p.s1, q.s1);
    dg = abs_diff(p.s2, q.s2);
    db = abs_diff(p.s3, q.s3);

    return ((299 * dr + 587 * dg + 114 * db) < 1500  ||
	    (213 * dr + 715 * dg +  72 * db) < 1500) &&
	 abs(596 * dr - 274 * dg - 322 * db) < 4200;
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
