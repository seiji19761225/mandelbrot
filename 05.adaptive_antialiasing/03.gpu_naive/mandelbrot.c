/*
 * mandelbrot.c
 * (c)2010-2018 Seiji Nishimura
 * $Id: mandelbrot.c,v 1.1.1.5 2020/07/30 00:00:00 seiji Exp seiji $
 */

#include <pixmap.h>
#include <palette.h>
#include <cl_util.h>

#if   defined(USE_HALTON) || defined(USE_HAMMERSLEY)
#include <lds.h>
#elif defined(USE_MT19937)
#include <mt19937.h>
#endif

#define KERNEL	"./kernel.cl"

// for adaptive anti-aliasing
#define MIN_SAMPLES	(0x01<<4)
#define MAX_SAMPLES	(0x01<<16)

// uniform RNG for [0:1)
#if   defined(USE_RAND)
#define SRAND(s)	srand(s)
#define DRAND()		((double) rand()/(RAND_MAX+1.0))
#elif defined(USE_MT19937)
#define SRAND(s)	init_genrand(s)
#define DRAND()		genrand_real2()
#endif

// prototype
void colormap_init(pixel_t *, int);
void jitter_init  (double *, double *);
void draw_image   (cl_obj_t *, pixmap_t *, pixel_t *,
			int, double, double, double, double *, double *);

//======================================================================
int main(int argc, char **argv)
{
    cl_obj_t obj;
    char    *options =
#ifdef USE_SAME_COLOR
		"-DUSE_SAME_COLOR "
#endif
#if        SIZEOF_PIXEL_T == 3
		"-DSIZEOF_PIXEL_T=3";
#else	// SIZEOF_PIXEL_T == 4
		"-DSIZEOF_PIXEL_T=4";
#endif

    pixmap_t image;
    pixel_t  colormap[ITER_MAX];
    double   dx[MAX_SAMPLES],
	     dy[MAX_SAMPLES];

    // initialize OpenCL
    cl_init(&obj, OPENCL_DEVICE, KERNEL, options);

    pixmap_create(&image, WIDTH, HEIGHT);
    colormap_init(colormap, ITER_MAX);
    jitter_init(dx, dy);

    // draw image
    draw_image(&obj, &image, colormap, ITER_MAX, CENTER_R, CENTER_I, RADIUS, dx, dy);

    pixmap_write_ppmfile(&image, "output.ppm");
    pixmap_destroy(&image);

    // finalize OpenCL
    cl_fin(&obj);

    return 0;
}

//----------------------------------------------------------------------
void colormap_init(pixel_t *colormap, int iter_max)
{
    int colormap_mask = COLORMAP_CYCLE - 1;

    colormap[0] = pixel_set_rgb(0x00, 0x00, 0x00);

    for (int i = 1; i < iter_max; i++)
#ifdef REVERSE_COLORMAP
	colormap[i] = palette(COLORMAP_TYPE, 0x00, colormap_mask,
			      colormap_mask - (i & colormap_mask));
#else
	colormap[i] = palette(COLORMAP_TYPE, 0x00, colormap_mask,
					       i & colormap_mask );
#endif

    return;
}

//----------------------------------------------------------------------
void jitter_init(double *dx, double *dy)
#if   defined(USE_HALTON)
{
    for (int k = 0; k < MAX_SAMPLES; k++) {
	dx[k] = lds_vdc(k, 2);
	dy[k] = lds_vdc(k, 3);
    }

    return;
}
#elif defined(USE_HAMMERSLEY)	//......................................
{
    for (int j = 0, k = 1; k <= MAX_SAMPLES; k = (j = k) << 0x01)
	for (int i = j; i < k; i++) {
	    dx[i] = (double) ((j == 0) ? i : (2 * (i - j) + 1)) / k;
	    dy[i] = lds_vdc(MAX_SAMPLES * dx[i], 3);
	}

    return;
}
#else				//......................................
{
#ifdef SEED
    SRAND(SEED);
#else
    SRAND((int) time(NULL));
#endif

    for (int k = 0; k < MAX_SAMPLES; k++) {
	dx[k] = DRAND();
	dy[k] = DRAND();
    }

    return;
}
#endif

//----------------------------------------------------------------------
void draw_image(cl_obj_t *obj, pixmap_t *image, pixel_t *colormap,
	int iter_max, double c_r, double c_i, double radius, double *dx, double *dy)
{
    int              width, height;
    cl_program       program = cl_query_program(obj);
    cl_command_queue queue   = cl_query_queue  (obj);
    cl_context       context = cl_query_context(obj);
    cl_kernel        kernel  = NULL;
    cl_mem           dev_dx, dev_dy, dev_sketch, dev_pixmap, dev_colormap;
    size_t           global_size[2];
    void rough_sketch(cl_obj_t *, cl_mem, int, int, cl_mem, int, double, double, double);

    pixmap_get_size(image, &width, &height);

    // memory allocation on GPU
    dev_dx       = clCreateBuffer(context, CL_MEM_READ_WRITE,
			MAX_SAMPLES    * sizeof(double ), NULL, NULL);
    dev_dy       = clCreateBuffer(context, CL_MEM_READ_WRITE,
			MAX_SAMPLES    * sizeof(double ), NULL, NULL);
    dev_sketch   = clCreateBuffer(context, CL_MEM_READ_WRITE,
			width * height * sizeof(pixel_t), NULL, NULL);
    dev_pixmap   = clCreateBuffer(context, CL_MEM_READ_WRITE,
			width * height * sizeof(pixel_t), NULL, NULL);
    dev_colormap = clCreateBuffer(context, CL_MEM_READ_WRITE,
			iter_max       * sizeof(pixel_t), NULL, NULL);

    // CPU->GPU memory copy
    clEnqueueWriteBuffer(queue, dev_dx      , CL_TRUE, 0,
			MAX_SAMPLES    * sizeof(double ), dx      , 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, dev_dy      , CL_TRUE, 0,
			MAX_SAMPLES    * sizeof(double ), dy      , 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, dev_colormap, CL_TRUE, 0,
			iter_max       * sizeof(pixel_t), colormap, 0, NULL, NULL);

    // draw rough sketch image
    rough_sketch(obj, dev_sketch, width, height, dev_colormap, iter_max, c_r, c_i, radius);

    // load kernel function
    kernel = clCreateKernel(program, "antialiasing_GPU", NULL);

    // set up kernel args for antialiasing
    clSetKernelArg(kernel,  0, sizeof(cl_mem), &dev_pixmap  );
    clSetKernelArg(kernel,  1, sizeof(cl_mem), &dev_sketch  );
    clSetKernelArg(kernel,  2, sizeof(int   ), &width       );
    clSetKernelArg(kernel,  3, sizeof(int   ), &height      );
    clSetKernelArg(kernel,  4, sizeof(cl_mem), &dev_colormap);
    clSetKernelArg(kernel,  5, sizeof(int   ), &iter_max    );
    clSetKernelArg(kernel,  6, sizeof(double), &c_r         );
    clSetKernelArg(kernel,  7, sizeof(double), &c_i         );
    clSetKernelArg(kernel,  8, sizeof(double), &radius      );
    clSetKernelArg(kernel,  9, sizeof(cl_mem), &dev_dx      );
    clSetKernelArg(kernel, 10, sizeof(cl_mem), &dev_dy      );

    // set up threads
    global_size[0] = width ;
    global_size[1] = height;

    // calling kernel function
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);

    // GPU->CPU memory copy
    clEnqueueReadBuffer(queue, dev_pixmap, CL_TRUE, 0,
			width * height * sizeof(pixel_t), image->data, 0, NULL, NULL);

    clFlush (queue);
    clFinish(queue);

    // memory deallocation on GPU
    clReleaseMemObject(dev_dx      );
    clReleaseMemObject(dev_dy      );
    clReleaseMemObject(dev_sketch  );
    clReleaseMemObject(dev_pixmap  );
    clReleaseMemObject(dev_colormap);

    // unload kernel function
    clReleaseKernel(kernel);

    return;
}

//......................................................................
void rough_sketch(cl_obj_t *obj, cl_mem dev_sketch, int width, int height,
	cl_mem dev_colormap, int iter_max, double c_r, double c_i, double radius)
{
    cl_program       program = cl_query_program(obj);
    cl_command_queue queue   = cl_query_queue  (obj);
    cl_context       context = cl_query_context(obj);
    cl_kernel        kernel  = NULL;
    size_t           global_size[2];

    // load kernel function
    kernel = clCreateKernel(program, "rough_sketch_GPU", NULL);

    // set up kernel args for rough_sketch
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &dev_sketch  );
    clSetKernelArg(kernel, 1, sizeof(int   ), &width       );
    clSetKernelArg(kernel, 2, sizeof(int   ), &height      );
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &dev_colormap);
    clSetKernelArg(kernel, 4, sizeof(int   ), &iter_max    );
    clSetKernelArg(kernel, 5, sizeof(double), &c_r         );
    clSetKernelArg(kernel, 6, sizeof(double), &c_i         );
    clSetKernelArg(kernel, 7, sizeof(double), &radius      );

    // set up threads
    global_size[0] = width ;
    global_size[1] = height;

    // calling kernel function
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);

    clFlush (queue);
    clFinish(queue);

    // unload kernel function
    clReleaseKernel(kernel);

    return;
}
