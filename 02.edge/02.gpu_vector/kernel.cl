/*
 * kernel.cl: Vectorized OpenCL kernel
 * (c)2017-2018 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.2 2018/09/09 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define VLEN	4
#define BAILOUT	4.0

inline int4    mandelbrot      (int, double4, double4);
inline int4    detect_edge     (__global uchar *, uchar16 *, int, int, int, int);
inline int     equivalent_color(uchar4, uchar4);
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
__kernel void draw_edge_GPU
	(__global uchar *pixmap, __global uchar *sketch, int width, int height)
{
    int     x     = get_global_id(0) * VLEN,
	    y     = get_global_id(1);
    uchar4  black = (uchar4) (0x00, 0x00, 0x00, 0x00),
	    white = (uchar4) (0x00, 0xff, 0xff, 0xff);
    uchar16 pixel;

    if (x > width - VLEN)	// to deal with vector remainder
	x = width - VLEN;

    int4 edge = detect_edge(sketch, &pixel, x, y, width, height);

#ifdef USE_MONOCHROME
    pixel = (uchar16) ((edge.s0) ? white : black, (edge.s1) ? white : black,
		       (edge.s2) ? white : black, (edge.s3) ? white : black);
#else
    pixel = (uchar16) ((edge.s0) ? white : pixel.s0123, (edge.s1) ? white : pixel.s4567,
		       (edge.s2) ? white : pixel.s89ab, (edge.s3) ? white : pixel.scdef);
#endif

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
inline int4 detect_edge(__global uchar *pixmap, uchar16 *pixel, int x, int y, int width, int height)
{
    int4 edge = 0;

    uchar4 p0 = (*pixel).s0123 = pixmap_get_pixel(pixmap, x + 0, y, width);
    uchar4 p1 = (*pixel).s4567 = pixmap_get_pixel(pixmap, x + 1, y, width);
    uchar4 p2 = (*pixel).s89ab = pixmap_get_pixel(pixmap, x + 2, y, width);
    uchar4 p3 = (*pixel).scdef = pixmap_get_pixel(pixmap, x + 3, y, width);

    for (int j = max(0, y - 1); j <= min(height - 1, y + 1); j++)
	for (int i = x - 1; i <= x + 1; i++)
	    if (i != x || j != y) {
		int4 ii  = clamp(i + (int4) (0, 1, 2, 3), 0, width - 1);
		edge.s0 |= !equivalent_color(p0, pixmap_get_pixel(pixmap, ii.s0, j, width));
		edge.s1 |= !equivalent_color(p1, pixmap_get_pixel(pixmap, ii.s1, j, width));
		edge.s2 |= !equivalent_color(p2, pixmap_get_pixel(pixmap, ii.s2, j, width));
		edge.s3 |= !equivalent_color(p3, pixmap_get_pixel(pixmap, ii.s3, j, width));
		if (all(edge))
		    return edge;
	    }

    return edge;
}

//----------------------------------------------------------------------
inline int equivalent_color(uchar4 p, uchar4 q)
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
