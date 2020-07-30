/*
 * pixmap_create.c
 * (c)2010,2015 Seiji Nishimura
 * $Id: pixmap_create.c,v 1.1.1.1 2015/11/27 00:00:00 seiji Exp seiji $
 */

#include <stdio.h>
#include <stdlib.h>
#include "pixmap_internal.h"

#define CALLOC(n,t)	((t *) calloc((n),sizeof(t)))

//======================================================================
void pixmap_create(pixmap_t *pixmap, int width, int height)
{				// create a pixmap.
    if ((pixmap->data =
		CALLOC((size_t) width * height, pixel_t)) == NULL) {
	perror(__func__);
	exit(EXIT_FAILURE);
    }

    pixmap->width  = width ;
    pixmap->height = height;

    return;
}
