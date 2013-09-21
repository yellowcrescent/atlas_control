#####################################################################
#
# Atlas Project - Data Acquisition Daemon (DAQ)
# 
# Global Makefile
# Copyright (c) 2012 J. Hipps/Nichiha USA, Inc.
#
#####################################################################

## Program names
CC=gcc
TARGET=atlas_daq_test

## Compilation & Programming parameters
INCL_DIR = ./
CFLAGS = -O2 -std=c99 `mysql_config --cflags` -I$(INCL_DIR)
LINKFLAGS = -O2 `mysql_config --libs`

## Object list
OBJS = atlas_daq.o mgmt_callbacks.o mgmt_fifo.o alarms.o drivers/melsec_mc/melsec.o drivers/melsec_mc/read_mcconfig.o drivers/eip/eip.o
ARS = ../tuxeip/tuxeip/libtuxeip.a

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(LINKFLAGS) -o $@ $(OBJS) $(ARS)

clean :
	rm -f *.o
	rm -f drivers/*/*.o

all : $(TARGET)

