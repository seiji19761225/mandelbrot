/*
 * pixmap_put_pixel.c
 * (c)2010,2015 Seiji Nishimura
 * $Id: pixmap_put_pixel.c,v 1.1.1.1 2015/11/27 00:00:00 seiji Exp seiji $
 */

#include <stddef.h>
#include "pixmap_internal.h"

//======================================================================
void pixmap_put_pixel(pixmap_t *pixmap, pixel_t pixel, int x, int y)
{				// put a pixel at (x, y).
    if (x >= 0 && x < pixmap->width &&
	y >= 0 && y < pixmap->height)
	pixmap->data[(size_t) y * pixmap->width + x] = pixel;

    return;
}
