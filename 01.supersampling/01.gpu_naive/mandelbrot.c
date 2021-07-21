/*
 * mandelbrot.c
 * (c)2010-2018 Seiji Nishimura
 * $Id: mandelbrot.c,v 1.1.1.5 2021/07/21 00:00:00 seiji Exp seiji $
 */

#include <pixmap.h>
#include <palette.h>
#include <cl_util.h>

#define KERNEL	"./kernel.cl"

// prototype
void colormap_init(pixel_t *, int);
void draw_image   (cl_obj_t *, pixmap_t *, pixel_t *, int, int, double, double, double);

//======================================================================
int main(int argc, char **argv)
{
    cl_obj_t obj;
    char    *options =
#if        SIZEOF_PIXEL_T == 3
		"-DSIZEOF_PIXEL_T=3";
#else	// SIZEOF_PIXEL_T == 4
		"-DSIZEOF_PIXEL_T=4";
#endif

    pixmap_t image;
    pixel_t  colormap[ITER_MAX];

    cl_init(&obj, NULL, OPENCL_DEVICE, 0, KERNEL, options);

    pixmap_create(&image, WIDTH, HEIGHT);
    colormap_init(colormap, ITER_MAX);

    draw_image(&obj, &image, colormap, ITER_MAX, AALEV, CENTER_R, CENTER_I, RADIUS);

    pixmap_write_ppmfile(&image, "output.ppm");
    pixmap_destroy(&image);

    cl_fin(&obj);

    return 0;
}

//----------------------------------------------------------------------
void colormap_init(pixel_t *colormap, int iter_max)
{
    const int colormap_mask = COLORMAP_CYCLE - 1;

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
void draw_image(cl_obj_t *obj, pixmap_t *image, pixel_t *colormap,
	int iter_max, int sampling, double c_r, double c_i, double radius)
{
    int              width, height;
    cl_program       program = cl_query_program(obj);
    cl_command_queue queue   = cl_query_queue  (obj);
    cl_context       context = cl_query_context(obj);
    cl_kernel        kernel  = NULL;
    cl_mem           dev_pixmap, dev_colormap;
    size_t           global_size[2];

    pixmap_get_size(image, &width, &height);

    // memory allocation on GPU
    dev_pixmap   = clCreateBuffer(context, CL_MEM_READ_WRITE,
			width * height * sizeof(pixel_t), NULL, NULL);
    dev_colormap = clCreateBuffer(context, CL_MEM_READ_WRITE,
			iter_max       * sizeof(pixel_t), NULL, NULL);

    // CPU->GPU memory copy
    clEnqueueWriteBuffer(queue, dev_colormap, CL_TRUE, 0,
			iter_max       * sizeof(pixel_t), colormap, 0, NULL, NULL);

    // load kernel function
    kernel = clCreateKernel(program, "antialiasing_GPU", NULL);

    // set up kernel args
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &dev_pixmap  );
    clSetKernelArg(kernel, 1, sizeof(int   ), &width       );
    clSetKernelArg(kernel, 2, sizeof(int   ), &height      );
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &dev_colormap);
    clSetKernelArg(kernel, 4, sizeof(int   ), &iter_max    );
    clSetKernelArg(kernel, 5, sizeof(int   ), &sampling    );
    clSetKernelArg(kernel, 6, sizeof(double), &c_r         );
    clSetKernelArg(kernel, 7, sizeof(double), &c_i         );
    clSetKernelArg(kernel, 8, sizeof(double), &radius      );

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
    clReleaseMemObject(dev_pixmap  );
    clReleaseMemObject(dev_colormap);

    // unload kernel function
    clReleaseKernel(kernel);

    return;
}
