/*
 * kernel.cl: Non-vectorized OpenCL kernel
 * (c)2017-2018,2021 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.4 2021/07/20 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define BAILOUT	4.0

// for adaptive anti-aliasing
#define MIN_SAMPLES	(0x01<<4)
#define MAX_SAMPLES	(0x01<<16)

inline int    mandelbrot      (int, double, double);
inline bool   detect_edge     (__global uchar *, uchar4 *, int, int, int, int);
inline bool   equivalent_color(uchar4, uchar4);
inline uchar4 pixmap_get_pixel(__global uchar *, int, int, int);

//----------------------------------------------------------------------
__kernel void rough_sketch_GPU
	(__global uchar *pixmap  , int width, int height,
	 __global uchar *colormap, int iter_max, double c_r, double c_i, double radius)
{
    int    iter;
    int    x   = get_global_id(0),
	   y   = get_global_id(1);
    double d   = 2.0 * radius / min(width, height),
	   p_r = c_r + d * (x - width  / 2),
	   p_i = c_i + d * (height / 2 - y);

    iter = mandelbrot(iter_max, p_r, p_i);

#if        SIZEOF_PIXEL_T == 3
    uchar3  pixel = vload3(iter % iter_max, colormap);
    vstore3(pixel, x + y * width, pixmap);
#else	// SIZEOF_PIXEL_T == 4
    uchar4  pixel = vload4(iter % iter_max, colormap);
    vstore4(pixel, x + y * width, pixmap);
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
	int4   sum      = convert_int4(pixel);
	int    m = 1, n = MIN_SAMPLES;
	double d = 2.0 * radius / min(width, height);
	c_r += d * (x - width  / 2),
	c_i += d * (height / 2 - y);
	do {
	    for (int k = m; k < n; k++) {	// pixel refinement with MC integration
		double p_r = c_r + d * dx[k],
		       p_i = c_i - d * dy[k];
		int   iter = mandelbrot(iter_max, p_r, p_i);
#if        SIZEOF_PIXEL_T == 3
		sum += convert_int4((uchar4) ((uchar) 0x00, vload3(iter % iter_max, colormap)));
#else	// SIZEOF_PIXEL_T == 4
		sum += convert_int4(vload4(iter % iter_max, colormap));
#endif
	    }
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
inline int mandelbrot(int iter_max, double p_r, double p_i)
{
    int i;
    double z_r, z_i, work;

    z_r  = p_r;
    z_i  = p_i;
    work = 2.0 * z_r * z_i;

    for (i = 1; i < iter_max && (z_r *= z_r) +
				(z_i *= z_i) < BAILOUT; i++) {
	z_r += p_r - z_i ;
	z_i  = p_i + work;
	work = 2.0 * z_r * z_i;
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
