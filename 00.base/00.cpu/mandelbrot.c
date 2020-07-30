/*
 * mandelbrot.c
 * (c)2010-2018 Seiji Nishimura
 * $Id: mandelbrot.c,v 1.1.1.3 2018/09/11 00:00:00 seiji Exp seiji $
 */

#include <pixmap.h>
#include <palette.h>

#define MIN(x,y)	(((x)<(y))?(x):(y))

// prototype
void colormap_init(pixel_t *, int);
void draw_image   (pixmap_t *, pixel_t *, int, double, double, double);
int  mandelbrot   (int, double, double);

//======================================================================
int main(int argc, char **argv)
{
    pixmap_t image;
    pixel_t  colormap[ITER_MAX];

    pixmap_create(&image, WIDTH, HEIGHT);
    colormap_init(colormap, ITER_MAX);

    draw_image(&image, colormap, ITER_MAX, CENTER_R, CENTER_I, RADIUS);

    pixmap_write_ppmfile(&image, "output.ppm");
    pixmap_destroy(&image);

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
void draw_image(pixmap_t *image, pixel_t *colormap, int iter_max,
				double c_r, double c_i, double radius)
{
    int iter_mask = iter_max - 1;
    int width, height;
    double d;

    pixmap_get_size(image, &width, &height);

    d = 2.0 * radius / MIN(width, height);

#pragma omp parallel for schedule(static,1) collapse(2)
    for (int j = 0; j < height; j++) {
	for (int i = 0; i < width; i++) {
	    double p_r = c_r + d * (i - width  / 2),
		   p_i = c_i + d * (height / 2 - j);
	    int   iter = mandelbrot(iter_max, p_r, p_i);
	    pixmap_put_pixel(image, colormap[iter & iter_mask], i, j);
	}
    }

    return;
}

//----------------------------------------------------------------------
int mandelbrot(int iter_max, double p_r, double p_i)
{				// kernel function (scalar version)
    int i;
    double z_r, z_i, work;

    z_r  = p_r;
    z_i  = p_i;
    work = 2.0 * z_r * z_i;

    for (i = 1; i < iter_max && (z_r *= z_r) +
				(z_i *= z_i) < 4.0; i++) {
	z_r += p_r - z_i ;
	z_i  = p_i + work;
	work = 2.0 * z_r * z_i;
    }

    return i;
}
