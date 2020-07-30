#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# $Id: Makefile,v 1.1.1.4 2017/07/27 00:00:00 seiji Exp seiji $
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
.PHONY: clean clobber default setup

DIRS	= $(strip utils $(shell ls [0-9][0-9].*/Makefile | sed -e "s/Makefile//g"))
#---------------------------------------------------------------------
default clean:
	@for dir in $(DIRS); do make -C $$dir $@; done

setup:
	@make -C utils
clobber: clean
	@rm -f *.tgz
#---------------------------------------------------------------------
-include addon.mk
