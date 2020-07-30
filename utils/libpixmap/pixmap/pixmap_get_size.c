/*
 * pixmap_get_size.c
 * (c)2010,2015 Seiji Nishimura
 * $Id: pixmap_get_size.c,v 1.1.1.1 2015/11/27 00:00:00 seiji Exp seiji $
 */

#include "pixmap_internal.h"

//======================================================================
void pixmap_get_size(pixmap_t *pixmap, int *width, int *height)
{				// get the size information of a pixmap.
    *width  = pixmap->width ;
    *height = pixmap->height;

    return;
}
