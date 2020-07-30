#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# makefile
# $Id: Makefile,v 1.1.1.7 2017/07/27 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
include ../../make.inc

.PHONY: default clean clobber

CFLAGS	+= -I.
PFLAGS	+=

DIR_PM	= pixmap
OBJ_PM	= \
pixmap_create.o		pixmap_destroy.o       pixmap_get_pixel.o  \
pixmap_get_size.o	pixmap_load_ppmfile.o  pixmap_put_pixel.o  \
pixmap_write_ppmfile.o
DIR_PLT	= palette
OBJ_PLT	= palette.o
OBJS	= $(OBJ_PM) $(OBJ_PLT)
VPATH	= $(DIR_PM) $(DIR_PLT)
LIB	= libpixmap.a
#.......................................................................
default: $(LIB)
#	@ls -l $^
$(LIB):  $(OBJS)
	$(AR) $@ $^
ifdef RANLIB
	$(RANLIB) $@
endif

clean clobber:
	@rm -f $(LIB) *.o *~ core*
#.......................................................................
# dependencies
$(OBJ_PLT): palette_internal.h  palette.h  ds9_colors.h  idl_colors.h
$(OBJ_PM):  pixmap_internal.h
$(OBJS):    pixmap.h
