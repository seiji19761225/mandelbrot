/*
 * pixmap_load_ppmfile.c
 * (c)2010,2015 Seiji Nishimura
 * $Id: pixmap_load_ppmfile.c,v 1.1.1.1 2015/11/27 00:00:00 seiji Exp seiji $
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "pixmap_internal.h"

// magic number of raw PPM image
#define RAW_PPM	(('P'<<8)|'6')

// prototype of internal procedure
static int _pixmap_get_int_from_ppm_header(FILE *);

//======================================================================
int pixmap_load_ppmfile(pixmap_t *pixmap, const char *fname)
{				// load a raw PPM image and create the pixmap data.
    FILE  *fp;
    size_t wh;
    int magic_number, width, height, maxval;

    if ((fp = fopen(fname, "rb")) == NULL)
	return EXIT_FAILURE;

    magic_number  = fgetc(fp) << 8;
    magic_number |= fgetc(fp);

    if (magic_number != RAW_PPM ||
	(width  = _pixmap_get_int_from_ppm_header(fp)) <= 0 ||
	(height = _pixmap_get_int_from_ppm_header(fp)) <= 0 ||
	(maxval = _pixmap_get_int_from_ppm_header(fp)) != 255) {
	fclose(fp);
	return EXIT_FAILURE;
    }

    wh = (size_t) width * height;

    pixmap_create(pixmap, width, height);

    if (fread(pixmap->data, sizeof(pixel_t), wh, fp) != wh) {
	pixmap_destroy(pixmap);
	fclose(fp);
	return EXIT_FAILURE;
    }

    fclose(fp);

    return EXIT_SUCCESS;
}

//......................................................................
static int _pixmap_get_int_from_ppm_header(FILE *fp)
{
    int c, v = 0;

    while (isspace(c = fgetc(fp)) || c == '#')
	if (c == '#')	// skip comment line.
	    while ('\n' != (c = fgetc(fp)) &&
		   '\r' !=  c)
		if (c == EOF)
		    return -1;

    if (!isdigit(c))
	return -1;

    do {
	v = 10 * v + c - '0';
    } while (isdigit(c = fgetc(fp)));

    return v;
}
