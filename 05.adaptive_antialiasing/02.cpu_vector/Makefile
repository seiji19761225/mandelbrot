#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# makefile
# $Id: Makefile,v 1.1.1.6 2018/09/11 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
include ../../make.inc

.PHONY: default clean clobber

UTILS	= ../../utils
VPATH	= $(UTILS)

CFLAGS	+= -I$(UTILS)
LIBS	+= -L$(UTILS) -lpixmap -lm
OBJS	=
BIN	= mandelbrot.exe
#------------------------------------------------------------------------
# config.
include config.mk
#------------------------------------------------------------------------
PFLAGS	+= -DUSE_$(shell echo $(REAL_T) | tr 'a-z' 'A-Z')_REAL
PFLAGS	+= -DUSE_$(shell echo $(SAMPLE) | tr 'a-z' 'A-Z')

ifeq ($(BOOL_T),bool)
PFLAGS	+= -DUSE_BOOL
endif

ifeq ($(DATA),benchmark)
PFLAGS	+= -DBENCHMARK_TEST
OBJS	+= wtime.o
endif

ifeq ($(EQVCLR),strict)
PFLAGS	+= -DUSE_SAME_COLOR
endif

ifeq ($(shell echo $$((${VECTOR}>=2))),1)
PFLAGS	+= -DVECTOR_LENGTH=$(VECTOR)
else
ifeq ($(shell echo $$((${VECTOR}==1))),1)
PFLAGS	+= -DUSE_OMP_SIMD
endif
endif

ifdef MPICC
CC	= $(MPICC)
PFLAGS	+= -DUSE_MPI
endif

ifeq ($(SAMPLE),halton)
OBJS	+= lds.o
endif

ifeq ($(SAMPLE),hammersley)
OBJS	+= lds.o
endif

ifeq ($(SAMPLE),mt19937)
OBJS	+= mt19937.o
endif
#------------------------------------------------------------------------
include input/$(DATA).dat
#........................................................................
PFLAGS	+= -DWIDTH="$(WIDTH)"
PFLAGS	+= -DHEIGHT="$(HEIGHT)"
PFLAGS	+= -DCENTER_R="$(CENTER_R)"
PFLAGS	+= -DCENTER_I="$(CENTER_I)"
PFLAGS	+= -DRADIUS="$(RADIUS)"
PFLAGS	+= -DITER_MAX="$(ITER_MAX)"
PFLAGS	+= -DCOLORMAP_TYPE="$(COLORMAP_TYPE)"
PFLAGS	+= -DCOLORMAP_CYCLE="$(COLORMAP_CYCLE)"
ifeq ($(COLORMAP_ORDER),reverse)
PFLAGS	+= -DREVERSE_COLORMAP
endif
ifdef SEED
PFLAGS	+= -DSEED="$(SEED)"
endif
#------------------------------------------------------------------------
default: $(BIN)

$(BIN): mandelbrot.c $(OBJS)
	$(CC) $(CFLAGS) $(PFLAGS) -o $@ $^ $(LIBS)

clean clobber:
	@rm -f $(BIN) *.o *~ core*
	@rm -f *.ppm *.pgm *.pbm *.gz
