#####################################################################
#
# Atlas Project - Data Acquisition Daemon (DAQ)
# 
# Global Makefile
# Copyright (c) 2012-2013 J. Hipps
#
# http://atlas.neoretro.net/
#
#	author:			Jacob Hipps <jacob@neoretro.net>
#	title:			Global Makefile
#	src:
#		lang:		makefile
#		bin:		make
#	srcdoc:
#		segment:	build_system
#		path:		global_makefile
#		base:		atlas.neoretro.net/srcdoc/
#		schema:		base/segment/path
#		cdate:		07 Aug 2012
#		mdate:		04 Nov 2013
#	options:
#		invocation:	"make [target]"
#		targets:
#			null:		Build atlas_daq and protocol drivers
#			clean: >	Clean (remove) all intermediate files
#					generated from the build process
#
#####################################################################

## Program names
CC=gcc
TARGET=atlas_daq_test

## Import env vars, if set
##
#ifdef($(ATLAS_BASEDIR)) 
#BASEDIR=$(ATLAS_BASEDIR)
#else
#	ifndef($(TUXEIP_PATH))
#		BASEDIR=$(PWD)
#	endif
#endif
BASEDIR=$(PWD)

## Compilation & Programming parameters
TUXEIP_PATH = $(BASEDIR)/../tuxeip
INCL_DIR = ./
CFLAGS = -O2 -std=c99 `mysql_config --cflags` -I$(INCL_DIR)
LINKFLAGS = -O2 `mysql_config --libs`

## Object list
TUXEIP = $(TUXEIP_PATH)/tuxeip/libtuxeip.a
OBJS = atlas_daq.o mgmt_callbacks.o mgmt_fifo.o alarms.o drivers/melsec_mc/melsec.o drivers/melsec_mc/read_mcconfig.o drivers/eip/eip.o
ARS = $(TUXEIP)


.c.o:
	@/bin/echo -en '[CC] [ ' $< ' ] '
	$(CC) -c $(CFLAGS) $< -o $@

$(TUXEIP):
	@/bin/echo -e '*** Building TuxEip library...'
	@/bin/echo -en '[ '$(TUXEIP)' ]\n'
	@sleep 1
	@/bin/echo -en  '\t>> Building in '$(TUXEIP_PATH)'...\n'
	cd "$(TUXEIP_PATH)/tuxeip" && pwd && cmake . && make
	
$(TARGET): $(TUXEIP) $(OBJS)
	@/bin/echo -en '[LD] [ '$(TARGET)' ] '
	$(CC) $(LINKFLAGS) $(OBJS) $(ARS) -o $@

clean : clean_core

clean_all : clean_core clean_tuxeip clean_drivers

clean_core :
	rm -f *.o
	rm -f drivers/*/*.o

clean_tuxeip :
	cd $(TUXEIP_PATH)
	make clean
	cd $(BASEDIR)	

all : $(TARGET)

