/*
 * kernel.cl: Vectorized OpenCL kernel
 * (c)2017-2018 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.2 2018/09/09 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define VLEN	4
#define BAILOUT	4.0

inline uchar16 colormap_lookup(__global uchar *, int4);

//----------------------------------------------------------------------
__kernel void mandelbrot_GPU
	(__global uchar *pixmap  , int width, int height,
	 __global uchar *colormap, int iter_max, double c_r, double c_i, double radius)
{
    int     x = get_global_id(0) * VLEN,
	    y = get_global_id(1);
    int4    i, mask;
    double  d = 2.0 * radius / min(width, height);
    double4 p_r, p_i, z_r, z_i, work;

    if (x > width - VLEN)	// to deal with vector remainder
	x = width - VLEN;

    p_r  = c_r + d * convert_double4(x + (int4) (0, 1, 2, 3) - width / 2);
    p_i  = c_i + d * convert_double (height / 2 - y);
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

    uchar16  pixel = colormap_lookup(colormap, i % iter_max);
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
