/*
 * mandelbrot_sycl.cpp: Simple Mandelbrot Set Rendering Program in SYCL
 * (c)2019 Seiji Nishimura <seiji1976@gmail.com>
 * $Id: mandelbrot_sycl.cpp,v 1.1.1.5 2020/07/30 00:00:00 seiji Exp seiji $
 */

#ifdef USE_MPI
#include <mpi.h>
#endif

#include <new>
#include <cmath>
#include <random>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <CL/sycl.hpp>

namespace sycl = cl::sycl;

typedef double real_t;

// kernel name
class RoughSketch;
class DrawImage  ;

// parameters
#define C_R		(-0.74323348754012)
#define C_I		( 0.13121889397412)
#define RADIUS		(1.E-7)
#define WIDTH		1920
#define HEIGHT		1080
#define COLORMAP_CYCLE	(0x01<<9)
#define OUTPUT_FNAME	"output.ppm"

// constants
#define D		(2.0*RADIUS/std::min(WIDTH,HEIGHT))
#define MIN_SAMPLING	(0x01<<4)
#define MAX_SAMPLING	(0x01<<16)
#define MAX_ITER	(0x01<<16)

// prototypes
void init_colormap  (uint8_t *);
void init_jitter    (float   *, float   *);
void draw_image     (uint8_t *, uint8_t *, uint8_t *, float   *, float *, int    , int);
void rough_sketch   (sycl::queue &, sycl::buffer<uint8_t> &, sycl::buffer<uint8_t> &, int, int);
template<typename T>
bool detect_edge    (T       &, uint8_t *, uint8_t *, uint8_t *, int    , int    );
bool same_color     (uint8_t  , uint8_t  , uint8_t  , uint8_t  , uint8_t, uint8_t);
inline
int  mandelbrot     (real_t   , real_t  );
void write_out_image(uint8_t *, const char *);

//======================================================================
int main(int argc, char **argv)
{
    uint8_t *colormap = nullptr, *sketch = nullptr, *image = nullptr;
    float   *dx       = nullptr, *dy     = nullptr;
    int      nprocs   = 1      ,  myrank = 0      ;

#ifdef USE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
#endif

    try {
	colormap = new uint8_t[3 * (COLORMAP_CYCLE + 1)];
	sketch   = new uint8_t[3 * WIDTH * HEIGHT      ];
	image    = new uint8_t[3 * WIDTH * HEIGHT      ];
	dx       = new float  [MAX_SAMPLING            ];
	dy       = new float  [MAX_SAMPLING            ];
    } catch (std::bad_alloc e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
    }

#ifdef USE_MPI
    std::memset(sketch, 0x00, 3 * WIDTH * HEIGHT * sizeof(uint8_t));
    std::memset(image , 0x00, 3 * WIDTH * HEIGHT * sizeof(uint8_t));
#endif

    if (myrank == 0) {
	init_colormap(colormap);
	init_jitter  (dx, dy  );
    }

#ifdef USE_MPI
    MPI_Bcast(colormap, 3 * (COLORMAP_CYCLE + 1) * sizeof(uint8_t), MPI_BYTE, 0, MPI_COMM_WORLD);
    MPI_Bcast(dx      , MAX_SAMPLING             * sizeof(float  ), MPI_BYTE, 0, MPI_COMM_WORLD);
    MPI_Bcast(dy      , MAX_SAMPLING             * sizeof(float  ), MPI_BYTE, 0, MPI_COMM_WORLD);
#endif

    draw_image(image, sketch, colormap, dx, dy, nprocs, myrank);

    if (myrank == 0) {
	write_out_image(image, OUTPUT_FNAME);
    }

    delete[] dx;
    delete[] dy;
    delete[] image;
    delete[] sketch;
    delete[] colormap;

#ifdef USE_MPI
    MPI_Finalize();
#endif

    return EXIT_SUCCESS;
}

//----------------------------------------------------------------------
void init_colormap(uint8_t *colormap)
{				// initialize colormap table.
    for (int i = 0; i < COLORMAP_CYCLE; i++) {
	uint8_t r = ((int) (127.0 * std::cos((2.0 * M_PI * i) / (COLORMAP_CYCLE   )) + 0.5)) + 128,
		g = ((int) (127.0 * std::sin((2.0 * M_PI * i) / (COLORMAP_CYCLE   )) + 0.5)) + 128,
		b = ((int) (127.0 * std::sin((2.0 * M_PI * i) / (COLORMAP_CYCLE>>1)) + 0.5)) + 128;
	colormap[3 * i + 0] = r;
	colormap[3 * i + 1] = g;
	colormap[3 * i + 2] = b;
    }

    colormap[3 * COLORMAP_CYCLE + 0] =
    colormap[3 * COLORMAP_CYCLE + 1] =
    colormap[3 * COLORMAP_CYCLE + 2] = 0;

    return;
}

//----------------------------------------------------------------------
void init_jitter(float *dx, float *dy)
{				// initialize jittering table.
    std::mt19937 mt_engine(std::random_device{}());
    std::uniform_real_distribution<float> frand(0.0, 1.0);

    dx[0] = dy[0] = 0.0f;

    for (int i = 1; i < MAX_SAMPLING; i++) {
	dx[i] = frand(mt_engine);
	dy[i] = frand(mt_engine);
    }

    return;
}

//----------------------------------------------------------------------
void draw_image(uint8_t *ptr_image, uint8_t *ptr_sketch, uint8_t *ptr_colormap,
		float *ptr_dx, float *ptr_dy, int nprocs, int myrank)
{				// draw anti-aliased image.
    sycl::queue queue(sycl::default_selector{});
    {
	sycl::buffer<uint8_t> buf_image   (ptr_image   , sycl::range<1>(3 * WIDTH * HEIGHT      ));
	sycl::buffer<uint8_t> buf_sketch  (ptr_sketch  , sycl::range<1>(3 * WIDTH * HEIGHT      ));
	sycl::buffer<uint8_t> buf_colormap(ptr_colormap, sycl::range<1>(3 * (COLORMAP_CYCLE + 1)));
	sycl::buffer<float  > buf_dx      (ptr_dx      , sycl::range<1>(MAX_SAMPLING            ));
	sycl::buffer<float  > buf_dy      (ptr_dy      , sycl::range<1>(MAX_SAMPLING            ));

	rough_sketch(queue, buf_sketch, buf_colormap, nprocs, myrank);

	queue.submit([&] (sycl::handler &cgh) {
	    int  nthreads = (WIDTH * HEIGHT + nprocs - 1) / nprocs;
	    auto image    = buf_image.get_access   <sycl::access::mode::read_write>(cgh);
	    auto sketch   = buf_sketch.get_access  <sycl::access::mode::read      >(cgh);
	    auto colormap = buf_colormap.get_access<sycl::access::mode::read      >(cgh);
	    auto dx       = buf_dx.get_access      <sycl::access::mode::read      >(cgh);
	    auto dy       = buf_dy.get_access      <sycl::access::mode::read      >(cgh);

	    cgh.parallel_for<DrawImage>(sycl::range<1>(nthreads), [=] (sycl::item<1> item) {
		int l = item.get_id(0) * nprocs + myrank;
		if (l < WIDTH * HEIGHT) {
		    int i = l % WIDTH,
			j = l / WIDTH;
		    uint8_t r , g , b ,
			    rr, gg, bb;
		    if (detect_edge(sketch, &r, &g, &b, i, j)) {	// Monte Carlo Integration
			int n     = 1, m     = MIN_SAMPLING,
			    sum_r = r, sum_g = g, sum_b = b;
			do {
			    rr = r;
			    gg = g;
			    bb = b;
			    for (int k = n; k < m; k++) {
				real_t p_r = C_R + D * (i + dx[k] - WIDTH  / 2),
				       p_i = C_I - D * (j + dy[k] - HEIGHT / 2);
				int  index = mandelbrot(p_r, p_i);
				sum_r += colormap[3 * index + 0];
				sum_g += colormap[3 * index + 1];
				sum_b += colormap[3 * index + 2]; 
			    }
			    r = (uint8_t) ((sum_r + (m>>1)) / m);
			    g = (uint8_t) ((sum_g + (m>>1)) / m);
			    b = (uint8_t) ((sum_b + (m>>1)) / m);
			} while (!same_color(r, g, b, rr, gg, bb) &&
					(m = (n = m) << 1) <= MAX_SAMPLING);
		    }
		    image[3 * l + 0] = r;
		    image[3 * l + 1] = g;
		    image[3 * l + 2] = b;
		}
	    });
	});
    }

#ifdef USE_MPI
    MPI_Allreduce(MPI_IN_PLACE, ptr_image, 3 * WIDTH * HEIGHT * sizeof(uint8_t),
		  MPI_BYTE, MPI_BOR, MPI_COMM_WORLD);
#endif

    return;
}

//----------------------------------------------------------------------
void rough_sketch(sycl::queue &queue, sycl::buffer<uint8_t> &buf_sketch,
				      sycl::buffer<uint8_t> &buf_colormap, int nprocs, int myrank)
{				// draw rough sketch image.
    queue.submit([&] (sycl::handler &cgh) {
	int  nthreads = (WIDTH * HEIGHT + nprocs - 1) / nprocs;
	auto sketch   = buf_sketch.get_access  <sycl::access::mode::read_write>(cgh);
	auto colormap = buf_colormap.get_access<sycl::access::mode::read      >(cgh);

	cgh.parallel_for<RoughSketch>(sycl::range<1>(nthreads), [=] (sycl::item<1> item) {
	    int l = item.get_id(0) * nprocs + myrank;
	    if (l < WIDTH * HEIGHT) {
		int      i = l % WIDTH,
			 j = l / WIDTH;
		real_t p_r = C_R + D * (i - WIDTH  / 2),
		       p_i = C_I - D * (j - HEIGHT / 2);
		int  index = mandelbrot(p_r, p_i);
		uint8_t  r = colormap[3 * index + 0],
			 g = colormap[3 * index + 1],
			 b = colormap[3 * index + 2];
		sketch[3 * l + 0] = r;
		sketch[3 * l + 1] = g;
		sketch[3 * l + 2] = b;
	    }
	});
    });
//  queue.wait_and_throw();

#ifdef USE_MPI
    // constructing host accessor
    auto sketch = buf_sketch.get_access<sycl::access::mode::read_write>();

    MPI_Allreduce(MPI_IN_PLACE, sketch.get_pointer(), sketch.get_size(),
		  MPI_BYTE, MPI_BOR, MPI_COMM_WORLD);
#endif

    return;
}

//----------------------------------------------------------------------
template<typename T>
bool detect_edge(T &image, uint8_t *r, uint8_t *g, uint8_t *b, int x, int y)
{				// detect whether (x,y) is edge or not.
    *r = image[3 * (x + y * WIDTH) + 0];
    *g = image[3 * (x + y * WIDTH) + 1];
    *b = image[3 * (x + y * WIDTH) + 2];

    for (int j = std::max(0, y-1); j <= std::min(HEIGHT-1, y+1); j++) {
	for (int i = std::max(0, x-1); i <= std::min(WIDTH-1, x+1); i++) {
	    int rr = image[3 * (i + j * WIDTH) + 0],
		gg = image[3 * (i + j * WIDTH) + 1],
		bb = image[3 * (i + j * WIDTH) + 2];
	    if (!same_color(*r, *g, *b, rr, gg, bb)) {
		return true;
	    }
	}
    }

    return false;
}

//----------------------------------------------------------------------
bool same_color(uint8_t r1, uint8_t g1, uint8_t b1,
		uint8_t r2, uint8_t g2, uint8_t b2)
#if 0
{				// detect whether (r1,g1,b1) and (r2,g2,b2) are same or not.
    return r1 == r2 &&
	   g1 == g2 &&
	   b1 == b2;
}
#else				//......................................
{				// detect whether (r1,g1,b1) and (r2,g2,b2) are same or not.
    return 3 * std::abs((int) r1 - r2) +
	   6 * std::abs((int) g1 - g2) +
	   1 * std::abs((int) b1 - b2) < 15;
}
#endif

//----------------------------------------------------------------------
inline
int mandelbrot(real_t p_r, real_t p_i)
{
    int i;
    real_t z_r, z_i, work;

    z_r  = p_r;
    z_i  = p_i;
    work = 2.0 * z_r * z_i;

    for (i = 1; i < MAX_ITER && (z_r *= z_r) +
				(z_i *= z_i) < 4.0; i++) {
	z_r += p_r - z_i ;
	z_i  = p_i + work;
	work = 2.0 * z_r * z_i;
    }

    // convert #iter to index of the colormap
    if (i &= MAX_ITER       - 1) {
	i &= COLORMAP_CYCLE - 1;
    } else {	// i == MAX_ITER
	i  = COLORMAP_CYCLE    ;
    }

    return i;
}

//----------------------------------------------------------------------
void write_out_image(uint8_t *image, const char *fname)
{				// write out image as a PPM file.
    std::ofstream fout(fname);

    if (!fout.good()) {
	std::cerr << "Error: cannot open " << fname << "." << std::endl;
	std::exit(EXIT_FAILURE);
    }

    fout << "P6"  << std::endl;
    fout << WIDTH << " " << HEIGHT << std::endl;
    fout << "255" << std::endl;
    fout.write((const char *) image, 3 * WIDTH * HEIGHT * sizeof(uint8_t));

    if (fout.fail()) {
	std::cerr << "Error: cannot write out " << fname << "." << std::endl;
	std::exit(EXIT_FAILURE);
    }

    fout.close();

    return;
}
