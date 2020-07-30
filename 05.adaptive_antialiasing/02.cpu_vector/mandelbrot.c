/*
 * mandelbrot.c: adaptive anti-aliasing [MPI+OpenMP+Vector Version]
 * (c)2010-2018 Seiji Nishimura
 * $Id: mandelbrot.c,v 1.1.1.8 2018/09/11 00:00:00 seiji Exp seiji $
 */

#ifdef USE_MPI
#include <mpi.h>
#endif

#ifdef BENCHMARK_TEST
#include <wtime.h>
#endif

#if   defined(USE_HALTON) || defined(USE_HAMMERSLEY)
#include <lds.h>
#elif defined(USE_MT19937)
#include <mt19937.h>
#endif

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pixmap.h>
#include <palette.h>
#include <stdbool.h>

// for adaptive anti-aliasing
#define MIN_SAMPLES	(0x01<<4)
#define MAX_SAMPLES	(0x01<<16)

#define ROUND(x)	((int) round(x))
#define MIN(x,y)	(((x)<(y))?(x):(y))
#define MAX(x,y)	(((x)>(y))?(x):(y))

// uniform RNG for [0:1)
#if   defined(USE_RAND)
#define SRAND(s)	srand(s)
#define DRAND()		((double) rand()/(RAND_MAX+1.0))
#elif defined(USE_MT19937)
#define SRAND(s)	init_genrand(s)
#define DRAND()		genrand_real2()
#endif

#if   defined(USE_FP128_REAL)
typedef __float128  real_t;
#elif defined(USE_FP80_REAL)
typedef long double real_t;
#else
typedef      double real_t;
#endif

// some compilers do not support vectorization for bool data type.
#ifdef USE_BOOL
typedef bool bool_t;
#define FALSE	false
#define TRUE	true
#else
typedef int  bool_t;
#define FALSE	0
#define TRUE	1
#endif

// prototypes
void   colormap_init   (pixel_t *, int);
void   jitter_init     (double *, double *);
void   draw_image      (pixmap_t *, pixmap_t *, pixel_t *, int,
			double, double, double, double *, double *, int, int);
void   rough_sketch    (pixmap_t *,             pixel_t *, int, double, double, double, int, int);
void   pixmap_reduction(pixmap_t *, int, int);
#ifdef VECTOR_LENGTH
void   mandelbrot      (int, int, int * restrict, real_t * restrict, real_t * restrict);
#else
#ifdef USE_OMP_SIMD
#pragma omp declare simd
#endif
int    mandelbrot      (int, real_t, real_t);
#endif
bool_t detect_edge     (pixmap_t *, pixel_t *, int, int);
bool_t equivalent_color(pixel_t, pixel_t);

//======================================================================
int main(int argc, char **argv)
{
    int nprocs = 1, myrank = 0;
    pixmap_t image, sketch;
    pixel_t  colormap[ITER_MAX];
    double   dx[MAX_SAMPLES],
	     dy[MAX_SAMPLES];
#ifdef BENCHMARK_TEST
    double ts, te, tm_init, tm_comp, tm_fin;
#endif

#ifdef USE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
#endif

#ifdef BENCHMARK_TEST
    if (myrank == 0)
	printf("*** Mandelbrot [%dx%d] #PE=%d ***\n", WIDTH, HEIGHT, nprocs);
    ts      = wtime(true);
#endif

    // pixmap data allocation, all pixels are zero-cleared.
    pixmap_create(&image , WIDTH, HEIGHT);
    pixmap_create(&sketch, WIDTH, HEIGHT);
    colormap_init(colormap, ITER_MAX);
    jitter_init(dx, dy);

#ifdef BENCHMARK_TEST
    te      = wtime(true);
    tm_init = te - ts;
    ts      = te;
#endif

    draw_image(&image, &sketch, colormap,
		ITER_MAX, CENTER_R, CENTER_I, RADIUS, dx, dy, nprocs, myrank);

#ifdef BENCHMARK_TEST
    te      = wtime(true);
    tm_comp = te - ts;
    ts      = te;
#endif

    if (myrank == 0)
	pixmap_write_ppmfile(&image, "output.ppm");

    // pixmap data deallocation
    pixmap_destroy(&sketch);
    pixmap_destroy(&image );

#ifdef BENCHMARK_TEST
    te      = wtime(true);
    tm_fin  = te - ts;
    if (myrank == 0)
	printf("Init/Comp/Fin=%.3f/%.3f/%.3f[sec.]\n",
				tm_init, tm_comp, tm_fin);
#endif

#ifdef USE_MPI
    MPI_Finalize();
#endif

    return EXIT_SUCCESS;
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
void jitter_init(double *dx, double *dy)
#if   defined(USE_HALTON)
{
    for (int k = 0; k < MAX_SAMPLES; k++) {
	dx[k] = lds_vdc(k, 2);
	dy[k] = lds_vdc(k, 3);
    }

    return;
}
#elif defined(USE_HAMMERSLEY)	//......................................
{
    for (int j = 0, k = 1; k <= MAX_SAMPLES; k = (j = k) << 0x01)
	for (int i = j; i < k; i++) {
	    dx[i] = (double) ((j == 0) ? i : (2 * (i - j) + 1)) / k;
	    dy[i] = lds_vdc(MAX_SAMPLES * dx[i], 3);
	}

    return;
}
#else				//......................................
{
#ifdef SEED
    SRAND(SEED);
#else
    SRAND((int) time(NULL));
#endif

    for (int k = 0; k < MAX_SAMPLES; k++) {
	dx[k] = DRAND();
	dy[k] = DRAND();
    }

    return;
}
#endif

//----------------------------------------------------------------------
void draw_image(pixmap_t *image, pixmap_t *sketch, pixel_t *colormap, int iter_max,
	double c_r, double c_i, double radius, double *dx, double *dy, int nprocs, int myrank)
{				// adaptive anti-aliasing
    int iter_mask = iter_max - 1;
    int width, height;
    double d;
#ifdef BENCHMARK_TEST
    double ts, te;
#endif

    rough_sketch(sketch, colormap, iter_max, c_r, c_i, radius, nprocs, myrank);

#ifdef BENCHMARK_TEST
    ts = wtime(true);
#endif

    pixmap_get_size(image, &width, &height);

    d = 2.0 * radius / MIN(width, height);

#pragma omp parallel for schedule(static,1)
    for (int xy = myrank; xy < width * height; xy += nprocs) {
	int x = xy % width,
	    y = xy / width;
	pixel_t pixel;
	if (detect_edge(sketch, &pixel, x, y)) {
	    pixel_t average  = pixel;
	    int m     = 1, n = MIN_SAMPLES,
		sum_r = pixel_get_r(pixel),
		sum_g = pixel_get_g(pixel),
		sum_b = pixel_get_b(pixel);
	    do {
		pixel = average;
#ifdef VECTOR_LENGTH
		for (int k = m; k < n; k += VECTOR_LENGTH) {	// pixel refinement with QMC/MC integration
		    int   vlen = MIN(VECTOR_LENGTH, n - k);
		    int   iter[VECTOR_LENGTH];
		    real_t p_r[VECTOR_LENGTH], p_i[VECTOR_LENGTH];
		    for (int j = 0; j < vlen; j++) {
			p_r[j] = c_r + d * ((x + dx[k + j]) - width  / 2);
			p_i[j] = c_i + d * (height / 2 - (y + dy[k + j]));
		    }
		    mandelbrot(vlen, iter_max, iter, p_r, p_i);
		    for (int j = 0; j < vlen; j++) {
			sum_r += pixel_get_r(colormap[iter[j] & iter_mask]);
			sum_g += pixel_get_g(colormap[iter[j] & iter_mask]);
			sum_b += pixel_get_b(colormap[iter[j] & iter_mask]);
		    }
		}
#else				//......................................
#ifdef USE_OMP_SIMD
#pragma omp simd reduction(+:sum_r,sum_g,sum_b)
#endif
		for (int k = m; k < n; k++) {	// pixel refinement with QMC/MC integration
		    double p_r = c_r + d * ((x + dx[k]) - width  / 2),
			   p_i = c_i + d * (height / 2 - (y + dy[k]));
		    int   iter = mandelbrot(iter_max, p_r, p_i);
		    sum_r += pixel_get_r(colormap[iter & iter_mask]);
		    sum_g += pixel_get_g(colormap[iter & iter_mask]);
		    sum_b += pixel_get_b(colormap[iter & iter_mask]);
		}
#endif
		average = pixel_set_rgb(ROUND((double) sum_r / n),
					ROUND((double) sum_g / n),
					ROUND((double) sum_b / n));
	    } while (!equivalent_color(average, pixel) &&
			(n = (m = n) << 0x01) <= MAX_SAMPLES);
	    pixel = average;
	}
	pixmap_put_pixel(image, pixel, x, y);
    }

#ifdef BENCHMARK_TEST
    te = wtime(true);
    if (myrank == 0)
#ifdef USE_MPI
	printf("Rendering=%10.3f[sec.],  ", te - ts);
#else
	printf("Rendering=%10.3f[sec.]\n" , te - ts);
#endif
#endif

    pixmap_reduction(image, nprocs, myrank);

    return;
}

//----------------------------------------------------------------------
void rough_sketch(pixmap_t *sketch, pixel_t *colormap, int iter_max,
		double c_r, double c_i, double radius, int nprocs, int myrank)
#ifdef VECTOR_LENGTH
{				// rough sketch image
    int iter_mask = iter_max - 1;
    int width, height;
    double d;
#ifdef BENCHMARK_TEST
    double ts, te;

    ts = wtime(true);
#endif

    pixmap_get_size(sketch, &width, &height);

    d = 2.0 * radius / MIN(width, height);

#pragma omp parallel for schedule(static,1)
    for (int xy  = myrank * VECTOR_LENGTH; width * height > xy ;
	     xy += nprocs * VECTOR_LENGTH) {
	int   vlen = MIN(VECTOR_LENGTH, width * height - xy);
	int   iter[VECTOR_LENGTH];
	real_t p_r[VECTOR_LENGTH], p_i[VECTOR_LENGTH];
	for (int j = 0; j < vlen; j++) {
	    int x = (j + xy) % width,
		y = (j + xy) / width;
	    p_r[j] = c_r + d * (x - width  / 2);
	    p_i[j] = c_i + d * (height / 2 - y);
	}
	mandelbrot(vlen, iter_max, iter, p_r, p_i);
	for (int j = 0; j < vlen; j++) {
	    int x = (j + xy) % width,
		y = (j + xy) / width;
	    pixmap_put_pixel(sketch, colormap[iter[j] & iter_mask], x, y);
	}
    }

#ifdef BENCHMARK_TEST
    te = wtime(true);
    if (myrank == 0)
#ifdef USE_MPI
	printf("Sketch   =%10.3f[sec.],  ", te - ts);
#else
	printf("Sketch   =%10.3f[sec.]\n" , te - ts);
#endif
#endif

    pixmap_reduction(sketch, nprocs, myrank);

    return;
}
#else				//......................................
{				// rough sketch image
    int iter_mask = iter_max - 1;
    int width, height;
    double d;
#ifdef BENCHMARK_TEST
    double ts, te;

    ts = wtime(true);
#endif

    pixmap_get_size(sketch, &width, &height);

    d = 2.0 * radius / MIN(width, height);

#pragma omp parallel for schedule(static,1)
    for (int xy = myrank; xy < width * height; xy += nprocs) {
	int    x   = xy % width,
	       y   = xy / width;
	double p_r = c_r + d * (x - width  / 2),
	       p_i = c_i + d * (height / 2 - y);
	int   iter = mandelbrot(iter_max, p_r, p_i);
	pixmap_put_pixel(sketch, colormap[iter & iter_mask], x, y);
    }

#ifdef BENCHMARK_TEST
    te = wtime(true);
    if (myrank == 0)
#ifdef USE_MPI
	printf("Sketch   =%10.3f[sec.],  ", te - ts);
#else
	printf("Sketch   =%10.3f[sec.]\n" , te - ts);
#endif
#endif

    pixmap_reduction(sketch, nprocs, myrank);

    return;
}
#endif

//----------------------------------------------------------------------
void pixmap_reduction(pixmap_t *pixmap, int nprocs, int myrank)
#ifdef USE_MPI
{				// reduce pixmap image distributed among PEs.
    int width, height;
#ifdef BENCHMARK_TEST
    double ts, te;

    ts = wtime(true);
#endif

    pixmap_get_size(pixmap, &width, height);

    MPI_Allreduce(MPI_IN_PLACE, pixmap->data, width * height * SIZEOF_PIXEL_T,
		  MPI_BYTE, MPI_BOR, MPI_COMM_WORLD);

#ifdef BENCHMARK_TEST
    te = wtime(true);
    if (myrank == 0)
	printf("Reduction=%.3f[sec.]\n", te - ts);
#endif

    return;
}
#else				//......................................
{				// dummy function
    return;
}
#endif

//----------------------------------------------------------------------
#ifdef VECTOR_LENGTH
void mandelbrot(int vlen, int iter_max, int * restrict iter,
		real_t * restrict p_r , real_t * restrict p_i)
#if 1
{				// kernel function (vector version)
    real_t z_r [VECTOR_LENGTH], z_i [VECTOR_LENGTH],
	   work[VECTOR_LENGTH], nrm2[VECTOR_LENGTH];

    for (int j  = 0  ; j < vlen; j++) {		// initialization
	iter[j] = 1;
	work[j] = 2.0     * p_r[j] * p_i[j];
	nrm2[j] = (z_r[j] = p_r[j] * p_r[j]) +
		  (z_i[j] = p_i[j] * p_i[j]);
    }

    for (int i = 2; i <= iter_max; i++) {	// main iteration
	bool_t to_be_continued = FALSE;
#ifdef USE_OMP_SIMD
#pragma omp simd reduction(|:to_be_continued)
#else
#pragma vector always
#pragma ivdep
#endif
	for (int j = 0; j < vlen; j++)
	    if (nrm2[j] < 4.0) {
		z_r [j] +=  p_r[j] -  z_i [j];
		z_i [j]  =  p_i[j] +  work[j];
		work[j]  =  z_r[j] *  z_i [j]  * 2.0;
		nrm2[j]  = (z_r[j] *= z_r [j]) +
			   (z_i[j] *= z_i [j]);
		iter[j]  = i;
		to_be_continued = TRUE;
	    }
	if (!to_be_continued)
	    break;
    }

    return;
}
#else				//......................................
{				// kernel function (vector version)
    bool_t mask[VECTOR_LENGTH];	// mask vector
    real_t z_r [VECTOR_LENGTH], z_i [VECTOR_LENGTH],
	   work[VECTOR_LENGTH];

    for (int j  = 0; j < vlen; j++) {		// initialization
	iter[j] = 1;
	work[j] = 2.0      * p_r[j] * p_i[j];
	mask[j] = ((z_r[j] = p_r[j] * p_r[j]) +
		   (z_i[j] = p_i[j] * p_i[j]) < 4.0);
    }

    for (int i = 2; i <= iter_max; i++) {	// main iteration
	bool_t to_be_continued = FALSE;
#ifdef USE_OMP_SIMD
#pragma omp simd reduction(|:to_be_continued)
#else
#pragma vector always
#pragma ivdep
#endif
	for (int j = 0; j < vlen; j++)
	    if (mask[j]) {
		z_r [j] +=   p_r[j] -  z_i [j];
		z_i [j]  =   p_i[j] +  work[j];
		work[j]  =   z_r[j] *  z_i [j]  * 2.0 ;
		mask[j]  = ((z_r[j] *= z_r [j]) +
			    (z_i[j] *= z_i [j]) < 4.0);
		iter[j]  = i;
		to_be_continued = TRUE;
	    }
	if (!to_be_continued)
	    break;
    }

    return;
}
#endif
#else				//......................................
int mandelbrot(int iter_max, real_t p_r, real_t p_i)
{				// kernel function (scalar version)
    int i;
    real_t z_r, z_i, work;

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
#endif

//----------------------------------------------------------------------
bool_t detect_edge(pixmap_t *pixmap, pixel_t *pixel, int x, int y)
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
		    return TRUE;
	    }

    return FALSE;
}

//----------------------------------------------------------------------
bool_t equivalent_color(pixel_t p, pixel_t q)
#ifdef USE_SAME_COLOR
{
    return pixel_get_r(p) == pixel_get_r(q) &&
	   pixel_get_g(p) == pixel_get_g(q) &&
	   pixel_get_b(p) == pixel_get_b(q);
}
#else				//......................................
{
    int dr, dg, db;

    dr = abs(pixel_get_r(p) - pixel_get_r(q));
    dg = abs(pixel_get_g(p) - pixel_get_g(q));
    db = abs(pixel_get_b(p) - pixel_get_b(q));

    return (fabs(0.299 * dr + 0.587 * dg + 0.114 * db) < 1.5  ||
	    fabs(0.213 * dr + 0.715 * dg + 0.072 * db) < 1.5) &&
	    fabs(0.596 * dr - 0.274 * dg - 0.322 * db) < 4.2;
}
#endif
