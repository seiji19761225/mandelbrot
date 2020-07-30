/*
 * kernel.cl: Non-vectorized OpenCL kernel
 * (c)2017-2018 Seiji Nishimura
 * $Id: kernel.cl,v 1.1.1.2 2018/09/09 00:00:00 seiji Exp seiji $
 */

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define BAILOUT	4.0

inline int mandelbrot(int, double, double);

//----------------------------------------------------------------------
__kernel void antialiasing_GPU
	(__global uchar *pixmap  , int width   , int height  ,
	 __global uchar *colormap, int iter_max, int sampling, double c_r, double c_i, double radius)
{
    int    x = get_global_id(0),
	   y = get_global_id(1);
    int    n = sampling * sampling;
    double d = 2.0 * radius / min(width, height);

#if        SIZEOF_PIXEL_T == 3
    int3 sum = 0;
#else	// SIZEOF_PIXEL_T == 4
    int4 sum = 0;
#endif

    c_r += d * (x - width  / 2);
    c_i += d * (height / 2 - y);
    d   /= sampling;

    for (int j = 0; j < sampling; j++)
	for (int i = 0; i < sampling; i++) {
	    double p_r  = c_r + d * i,
		   p_i  = c_i - d * j;
	    int    iter = mandelbrot(iter_max, p_r, p_i);
#if        SIZEOF_PIXEL_T == 3
	    sum += convert_int3(vload3(iter % iter_max, colormap));
#else	// SIZEOF_PIXEL_T == 4
	    sum += convert_int4(vload4(iter % iter_max, colormap));
#endif
	}

    sum = (sum + (n >> 1)) / n;

#if        SIZEOF_PIXEL_T == 3
    vstore3(convert_uchar3_sat(sum), x + y * width, pixmap);
#else	// SIZEOF_PIXEL_T == 4
    vstore4(convert_uchar4_sat(sum), x + y * width, pixmap);
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
