#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# $Id: Makefile,v 1.1.1.1 2017/07/27 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
.PHONY: clean clobber default

include ../make.inc

DIRS	 = $(strip $(shell ls [0-9][0-9].cpu*/Makefile | sed -e "s/Makefile//g"))
ifdef LIBOCL
DIRS	+= $(strip $(shell ls [0-9][0-9].gpu*/Makefile | sed -e "s/Makefile//g"))
endif
#---------------------------------------------------------------------
default clean clobber:
	@for dir in $(DIRS); do make -C $$dir $@; done
