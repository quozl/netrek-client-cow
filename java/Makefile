#!make
################################################################################
# Copyright (c) 1996 Netscape Communications. All rights reserved.
################################################################################
#
# Template plugin makefile
#
# Platform: Linux 1.2 (ELF)
#
# The output of the make process will be nptemplate.so
# Install this file either in
#	/usr/lib/netscape/plugins/
#	(or)
#	~/.netscape/plugins/
#	(or) in any convenient directory and point environment variable
#	     NPX_PLUGIN_PATH to point to the directory. It is advisable
#	     that the plugins (.so) are the only files in that directory.
#
# This makefile contains some of our defines for the compiler:
#
# XP_UNIX	This needs to get defined for npapi.h on unix platforms.
# PLUGIN_TRACE	Enable this define to get debug prints whenever the plugin
#		api gets control.
#		
# WARNING: Motif libraries are built static into the navigator and cannot
#          be accessed from the plugin.
#          

SDK=/home/info/PluginSDK30b5

# PLUGIN_DEFINES= -DXP_UNIX -DPLUGIN_TRACE -DDEBUG
PLUGIN_DEFINES= -DPLUGIN_TRACE -DXP_UNIX -DDEBUG -I$(SDK)/include -I./_gen

CC= gcc
OPTIMIZER= -g
CFLAGS=  $(OPTIMIZER) $(PLUGIN_DEFINES) -I. -I/usr/include
LDFLAGS= -L.. -lcow 

SRC= UnixShell.c stubs.c COW.c COWImpl.c
OBJ= plugin.o  COWImpl.o stubs.o

SHAREDTARGET=npcow.so

default all: $(SHAREDTARGET) 

$(SHAREDTARGET): $(OBJ)
	$(CC) -shared -o $(SHAREDTARGET) $(OBJ) $(LDFLAGS)

plugin.o: plugin.c
	$(CC) -c $(CFLAGS) plugin.c

stubs.o: stubs.c
	$(CC) -c $(CFLAGS) stubs.c

COW.o: COW.c
	$(CC) -c $(CFLAGS) COW.c

COWImpl.o: COWImpl.c
	$(CC) -c $(CFLAGS) -I.. COWImpl.c

clean:
	-$(RM) $(OBJ) $(SHAREDTARGET)
