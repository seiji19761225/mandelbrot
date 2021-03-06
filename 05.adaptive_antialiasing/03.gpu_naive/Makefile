#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# makefile
# $Id: Makefile,v 1.1.1.3 2020/07/30 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
include ../../make.inc

.PHONY: default clean clobber

UTILS	= ../../utils
VPATH	= $(UTILS)

CFLAGS	+= -I$(UTILS)
LIBS	+= -L$(UTILS) $(LIBOCL) -lpixmap -lm
OBJS	= cl_util.o
BIN	= mandelbrot.exe
#------------------------------------------------------------------------
# config.
include config.mk
#........................................................................
PFLAGS	+= -DOPENCL_DEVICE=CL_DEVICE_TYPE_$(shell echo $(DEVICE) | tr 'a-z' 'A-Z')
PFLAGS	+= -DUSE_$(shell echo $(SAMPLE) | tr 'a-z' 'A-Z')

ifeq ($(EQVCLR),strict)
PFLAGS  += -DUSE_SAME_COLOR
endif

ifeq ($(SAMPLE),halton)
OBJS	+= lds.o
endif

ifeq ($(SAMPLE),hammersley)
OBJS	+= lds.o
endif

ifeq ($(SAMPLE),mt19937)
LIBS	+= -lmt19937
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
