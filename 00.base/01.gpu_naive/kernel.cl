/*
 * kernel.cl: Non-vectorized OpenCL kernel
 * (c)2017-2018 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.2 2018/09/09 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define BAILOUT	4.0

__kernel void mandelbrot_GPU
	(__global uchar *pixmap  , int width, int height,
	 __global uchar *colormap, int iter_max, double c_r, double c_i, double radius)
{
    int    i;
    int    x    = get_global_id(0),
	   y    = get_global_id(1);
    double work = 2.0 * radius / min(width, height),
	   p_r  = c_r + work * (x - width  / 2),
	   p_i  = c_i + work * (height / 2 - y);

    c_r  = p_r;
    c_i  = p_i;
    work = 2.0 * c_r * c_i;

    for (i = 1; i < iter_max && (c_r *= c_r) +
				(c_i *= c_i) < BAILOUT; i++) {
	c_r += p_r - c_i ;
	c_i  = p_i + work;
	work = 2.0 * c_r * c_i;
    }

#if        SIZEOF_PIXEL_T == 3
    uchar3  pixel = vload3(i % iter_max, colormap);
    vstore3(pixel, x + y * width, pixmap);
#else	// SIZEOF_PIXEL_T == 4
    uchar4  pixel = vload4(i % iter_max, colormap);
    vstore4(pixel, x + y * width, pixmap);
#endif

    return;
}
