/*
 * pixmap_destroy.c
 * (c)2010,2015 Seiji Nishimura
 * $Id: pixmap_destroy.c,v 1.1.1.1 2015/11/27 00:00:00 seiji Exp seiji $
 */

#include <stdlib.h>
#include <string.h>
#include "pixmap_internal.h"

//======================================================================
void pixmap_destroy(pixmap_t *pixmap)
{				// destroy a pixmap.
    free  (pixmap->data);
    memset(pixmap, 0x00, sizeof(pixmap_t));

    return;
}
