#####################################################################
#
# Atlas Project - Mitsubishi MELSEC GX Developer File Parser
# 
# Global Makefile
# Copyright (c) 2013 J. Hipps/Neo-Retro Group LLC.
# @author-mail: tetrisfrog@gmail.com
# @group-url:   http://neoretro.net/
# @prog-url:    http://neoretro.net/progs/jhipps/gxdev_read
#
#####################################################################

## Program names
CC=gcc
TARGET=gxdev_read

## Compilation & Programming parameters
INCL_DIR = ./
CFLAGS = -O2 -std=c99 -I$(INCL_DIR)
LINKFLAGS = -O2

## Object list
OBJS = gxdev_read.o

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(LINKFLAGS) -o $@ $(OBJS) $(ARS)

clean :
	rm -f *.o

all : $(TARGET)

