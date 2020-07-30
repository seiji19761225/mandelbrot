/*
 * pixmap.h: pixmap library lite-edition
 * (c)2010,2015 Seiji Nishimura
 * $Id: pixmap.h,v 1.1.1.3 2015/11/27 00:00:00 seiji Exp seiji $
 */

#ifndef _PIXMAP_H_
#define _PIXMAP_H_

#ifdef _PIXMAP_INTERNAL_
#define PIXMAP_API
#else
#define PIXMAP_API extern
#endif

#define SIZEOF_PIXEL_T	3	// == sizeof(pixel_t)

#define pixel_set_rgb(r,g,b)	((pixel_t) {(r),(g),(b)})
#define pixel_get_r(p)		((int) (p).r)
#define pixel_get_g(p)		((int) (p).g)
#define pixel_get_b(p)		((int) (p).b)

typedef struct {		// pixel
    unsigned char r, g, b;
} pixel_t;

typedef struct {		// pixmap
    int width, height;
    pixel_t *data;
} pixmap_t;

#ifdef __cplusplus
extern "C" {
#endif

// prototype
PIXMAP_API void pixmap_create       (pixmap_t *, int  , int  );
PIXMAP_API void pixmap_destroy      (pixmap_t *);
PIXMAP_API void pixmap_put_pixel    (pixmap_t *, pixel_t  , int, int);
PIXMAP_API void pixmap_get_pixel    (pixmap_t *, pixel_t *, int, int);
PIXMAP_API void pixmap_get_size     (pixmap_t *, int *, int *);
PIXMAP_API int  pixmap_load_ppmfile (pixmap_t *, const char *);
PIXMAP_API int  pixmap_write_ppmfile(pixmap_t *, const char *);

#ifdef __cplusplus
}
#endif
#undef PIXMAP_API
#endif
