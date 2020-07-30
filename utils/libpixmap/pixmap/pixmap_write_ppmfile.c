/*
 * pixmap_write_ppmfile.c
 * (c)2010,2015 Seiji Nishimura
 * $Id: pixmap_write_ppmfile.c,v 1.1.1.1 2015/11/27 00:00:00 seiji Exp seiji $
 */

#include <stdio.h>
#include <stdlib.h>
#include "pixmap_internal.h"

// LF is '\n' on UNIX/LINUX.
#define LF	0x0a

//======================================================================
int pixmap_write_ppmfile(pixmap_t *pixmap, const char *fname)
{				// write out a pixmap as a raw PPM image file.
    FILE  *fp;
    size_t wh;

    if ((fp = fopen(fname, "wb")) == NULL)
	return EXIT_FAILURE;

    wh = (size_t) pixmap->width * pixmap->height;

    fprintf(fp, "P6%c%d %d%c255%c", LF,  pixmap->width,  pixmap->height, LF, LF);

    if (ferror(fp) ||
	fwrite(pixmap->data, sizeof(pixel_t), wh, fp) != wh) {
	fclose(fp);
	return EXIT_FAILURE;
    }

    fclose(fp);

    return EXIT_SUCCESS;
}
