#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# make.inc
# $Id: make.inc,v 1.1.1.3 2021/07/21 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
.SUFFIXES: .c .o

#MPICC	= mpicc
#CC	= gcc
CC	= clang
CFLAGS	= -O3 -fopenmp

PFLAGS	= -DCL_TARGET_OPENCL_VERSION=120
LIBS	= -DDEBUG
LIBOCL	= -lOpenCL
#------------------------------------------------------------------------
AR	= ar scr
#RANLIB	= ranlib
#------------------------------------------------------------------------
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c -o $@ $<
