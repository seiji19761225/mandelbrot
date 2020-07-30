/*
 * wtime.c: wall clock timer
 * (c)2011-2016 Seiji Nishimura
 * $Id: wtime.c,v 1.1.1.6 2016/07/04 00:00:00 seiji Exp seiji $
 */

#include "wtime_internal.h"

#ifdef USE_MPI
#include <mpi.h>
#endif
#include <time.h>

//======================================================================
double wtime(bool sync)
{				// wall clock timer
    struct timespec ts;

#ifdef USE_MPI
    if (sync)			// need to synchronize all PEs.
	MPI_Barrier(MPI_COMM_WORLD);
#endif

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
	return -1.0;		// negative value means error.

    return (double) ts.tv_sec  +
	   (double) ts.tv_nsec * 1.E-9;
}
