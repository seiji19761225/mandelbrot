#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# makefile
# $Id: Makefile,v 1.1.1.3 2017/07/27 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
.PHONY: default clean clobber

LIB	= libpixmap.a
#.......................................................................
default: $(LIB)
$(LIB):
	@make -C libpixmap/
	@ln -s   libpixmap/$@
	@ln -s   libpixmap/pixmap.h
	@ln -s   libpixmap/palette.h

clean clobber:
	@make -C libpixmap/ $@
	@rm -f $(LIB) pixmap.h palette.h
	@rm -f *.o *~ core*
