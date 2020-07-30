/*
 * mandelbrot.c: edge detection
 * (c)2010-2018 Seiji Nishimura
 * $Id: mandelbrot.c,v 1.1.1.3 2018/09/11 00:00:00 seiji Exp seiji $
 */

#include <math.h>
#include <pixmap.h>
#include <palette.h>
#include <stdbool.h>

#define ROUND(x)	((int) round(x))
#define MIN(x,y)	(((x)<(y))?(x):(y))
#define MAX(x,y)	(((x)>(y))?(x):(y))

// prototypes
void colormap_init   (pixel_t  *, int);
void draw_image      (pixmap_t *, pixmap_t *, pixel_t *, int, double, double, double);
void rough_sketch    (pixmap_t *,             pixel_t *, int, double, double, double);
int  mandelbrot      (int, double, double);
bool detect_edge     (pixmap_t *, pixel_t  *, int, int);
bool equivalent_color(pixel_t, pixel_t);

//======================================================================
int main(int argc, char **argv)
{
    pixmap_t image, sketch;
    pixel_t  colormap[ITER_MAX];

    pixmap_create(&image , WIDTH, HEIGHT);
    pixmap_create(&sketch, WIDTH, HEIGHT);
    colormap_init(colormap, ITER_MAX);

    draw_image(&image, &sketch, colormap, ITER_MAX, CENTER_R, CENTER_I, RADIUS);

    pixmap_write_ppmfile(&image, "output.pbm");
    pixmap_destroy(&sketch);
    pixmap_destroy(&image );

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
void draw_image(pixmap_t *image, pixmap_t *sketch, pixel_t *colormap,
		int iter_max, double c_r, double c_i, double radius)
{				// draw edge image.
    const pixel_t black = pixel_set_rgb(0x00, 0x00, 0x00),
		  white = pixel_set_rgb(0xff, 0xff, 0xff);

    rough_sketch(sketch, colormap, iter_max, c_r, c_i, radius);

#pragma omp parallel for schedule(static,1)
    for (int xy = 0;  xy < WIDTH * HEIGHT; xy++) {
	const int x = xy % WIDTH,
		  y = xy / WIDTH;
	bool edge;
	pixel_t pixel;
	edge  = detect_edge(sketch, &pixel, x, y);
#ifdef USE_MONOCHROME
	pixel = edge ? white : black;
#else
	pixel = edge ? white : pixel;
#endif
	pixmap_put_pixel(image, pixel, x, y);
    }

    return;
}

//----------------------------------------------------------------------
void rough_sketch(pixmap_t *sketch, pixel_t *colormap,
		int iter_max, double c_r, double c_i, double radius)
{
    int iter_mask = iter_max - 1;
    int width, height;
    double d;

    pixmap_get_size(sketch, &width, &height);

    d = 2.0 * radius / MIN(width, height);

#pragma omp parallel for schedule(static,1)
    for (int xy = 0; xy < WIDTH * HEIGHT; xy++) {
	int    x   = xy % WIDTH,
	       y   = xy / WIDTH;
	double p_r = c_r + d * (x - width  / 2),
	       p_i = c_i + d * (height / 2 - y);
	int   iter = mandelbrot(iter_max, p_r, p_i);
	pixmap_put_pixel(sketch, colormap[iter & iter_mask], x, y);
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

//----------------------------------------------------------------------
bool detect_edge(pixmap_t *pixmap, pixel_t *pixel, int x, int y)
{
    int width, height;

    pixmap_get_size (pixmap, &width, &height);
    pixmap_get_pixel(pixmap, pixel, x, y);

    for (int j = MAX(0, y - 1); j <= MIN(height - 1, y + 1); j++)
	for (int i = MAX(0, x - 1); i <= MIN(width - 1, x + 1); i++)
	    if (i != x || j != y) {
		pixel_t p;
		pixmap_get_pixel(pixmap, &p, i, j);
		if (!equivalent_color(*pixel, p))
		    return true;
	    }

    return false;
}

//----------------------------------------------------------------------
bool equivalent_color(pixel_t p, pixel_t q)
#ifdef USE_SAME_COLOR
{
    return pixel_get_r(p) == pixel_get_r(q) &&
	   pixel_get_g(p) == pixel_get_g(q) &&
	   pixel_get_b(p) == pixel_get_b(q);
}
#else				//......................................
{
    int dr, dg, db;

    dr = pixel_get_r(p) - pixel_get_r(q);
    dg = pixel_get_g(p) - pixel_get_g(q);
    db = pixel_get_b(p) - pixel_get_b(q);

    return (fabs(0.299 * dr + 0.587 * dg + 0.114 * db) < 1.5  ||
	    fabs(0.213 * dr + 0.715 * dg + 0.072 * db) < 1.5) &&
	    fabs(0.596 * dr - 0.274 * dg - 0.322 * db) < 4.2;
}
#endif
