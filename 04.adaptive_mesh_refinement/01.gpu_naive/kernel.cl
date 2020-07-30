/*
 * kernel.cl: Non-vectorized OpenCL kernel
 * (c)2017-2018 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.2 2018/09/09 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define BAILOUT	4.0

// for AMR
#define MIN_GRID	(0x01<<2)
#define MAX_GRID	(0x01<<8)

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
	int4   sum     = convert_int4(pixel);
	double d = 2.0 * radius / min(width, height);
	c_r += d * (x - width  / 2),
	c_i += d * (height / 2 - y);
	for (int ngrid = MIN_GRID; ngrid <= MAX_GRID; ngrid <<= 0x01) {
	    int    ng2 = ngrid * ngrid;
	    double e   = d     / ngrid;
	    for (int k = 1; k < ng2; k++) {	// pixel refinement with AMR
		const int m = k % ngrid,
			  n = k / ngrid;
		if (((m | n) & 0x01) ||	// skip redundant points: (m % 2) != 0 || (n % 2) != 0
		    ngrid == MIN_GRID) {
		    double p_r = c_r + e * m,
			   p_i = c_i - e * n;
		    int   iter = mandelbrot(iter_max, p_r, p_i);
#if        SIZEOF_PIXEL_T == 3
		    sum += convert_int4((uchar4) ((uchar) 0x00, vload3(iter % iter_max, colormap)));
#else	// SIZEOF_PIXEL_T == 4
		    sum += convert_int4(vload4(iter % iter_max, colormap));
#endif
		}
	    }
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
