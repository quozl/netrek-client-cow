#
# =========================================================================
# COW on Win32 Makefile
# This makefile  is written for Watcom C++ 10.0, but it should
# be fairly easy to convert to other compilers, mostly by changing
# this top section of the makefile, and the link line, below
#
# !IFNDEF's added for MSVC - Microsoft Visual C++

# For No Debugging Info invoke with   make nodebug=1
#
# Note: creating the environment variable NODEBUG is an
#       alternate method to setting this option via the make command 
line.

# Change this when you compile!
CWHO = greynite@vnet.net

!IFNDEF MSVC

# Basic defs
cc     = wcc386
rc     = wrc
hc     = whc
link   = wlink
implib = wlib
compileandlink = wcl386

!ELSE

# Microsoft Visual C++ defs
# hc == help compiler?
cc     = cl
rc     = rc
hc     = hcw
link   = link
implib = lib
compileandlink = cl

!ENDIF

# Compile Flags - must be specified after $(cc)
#
# Compiler flags (all other defaults are OK for NT):
#   -d2      - generate debugging information
#   -bt=nt   - compile for NT
#   -c       - compile only
#   -od      - no optimization
#   -otexan  - full optimization for speed
#   -zq      - shut up, already (no banner message)


!IFNDEF MSVC

!IFDEF nodebug
cflags = -zq -otexan -zp4 -5 -fp3 -dWIN32
!ELSE
cflags = -zq -d2 -od -dWIN32 
!ENDIF

!ELSE

# MS Visual C++ (v4.0) options
# /nologo   - no banner
# /MT       - Multi-threaded app
# /Ox       - max optimization
# /Od       - disable optimizations
# /G4       - 486 optimized (386 running Win32? Uh, no)
# /c        - compile only
# /Zi       - debugging info
# /Fr[file] - generate browse info

!IFDEF nodebug
cflags = /nologo /MT -DWIN32 /Ox /G4
ccompileonly = /c
!ELSE
cflags = /nologo /MT -DWIN32 /Od /G4 /Zi
ccompileonly = /c
!ENDIF

!ENDIF

# resource compiler
rcvars = -DWIN32 

########################################
# Link flags
#

!IFNDEF MSVC

!IFDEF NODEBUG
ldebug = 
!ELSE
ldebug = d all 
!ENDIF

!ELSE

!IFDEF NODEBUG
ldebug = /INCREMENTAL:NO
!ELSE
ldebug = /INCREMENTAL:NO /DEBUG
!ENDIF

!ENDIF
########################################

# declarations common to all linker options

!IFNDEF MSVC

lflags = op st=8192 op symf 
# subsystem dependant flags
conlflags = $(lflags) SYS nt 
guilflags = $(lflags) SYS nt_win
# The space is neccesary!!! -SAC
lout      = "name "
#GNU multi-byte precision library - for RSA
GMPLIB = .\addon\lib\libgmp.lib

!ELSE

lflags = /STACK:8192
#Dunno here...
conlflags = $(lflags) /NODEFAULTLIB /SUBSYSTEM:CONSOLE
guilflags = $(lflags) /SUBSYSTEM:CONSOLE
lout      = "/OUT:"
GMPLIB = libmp.lib libgmp.lib libcmt.lib user32.lib gdi32.lib\
         wsock32.lib kernel32.lib winmm.lib mpr.lib

!ENDIF


# 
---------------------------------------------------------------------------
# - Start of makefile proper

INCS = -I.\win32\h
ARCH = Intel/Win32

# DEFS  = -DRSA  -DDEBUG
DEFS = -DRSA -DDEBUG

ROBJ            = check.obj colors.obj data.obj death.obj defaults.obj dmessage.obj\
                  enter.obj findslot.obj getname.obj getship.obj helpwin.obj inform.obj\
                  interface.obj newwin.obj option.obj planetlist.obj macrowin.obj\
                  map.obj playerlist.obj ranklist.obj reserved.obj sintab.obj\
                  smessage.obj socket.obj stats.obj util.obj war.obj warning.obj\
                  udpopt.obj ping.obj pingstats.obj rotate.obj lagmeter.obj parsemeta.obj\
                  netstat.obj netstatopt.obj spopt.obj dashboard.obj dashboard3.obj\
                  short.obj distress.obj senddist.obj defwin.obj tools.obj sound.obj\
                  docwin.obj cflags.obj beeplite.obj feature.obj\
                  string_util.obj local.obj censor.obj cowmain.obj 

RSRC            = check.c colors.c data.c death.c defaults.c dmessage.c\
                  enter.c findslot.c getname.c getship.c helpwin.c inform.c\
                  input.c interface.c newwin.c option.c planetlist.c\
                  macrowin.c map.c playerlist.c ranklist.c redraw.c\
                  smessage.c parsemeta.c socket.c stats.c util.c war.c\
                  warning.c udpopt.c sintab.c ping.c pingstats.c rotate.c\
                  lagmeter.c netstat.c netstatopt.c spopt.c dashboard.c dashboard3.c\
                  short.c distress.c senddist.c defwin.c tools.c sound.c\
                  docwin.c cflags.c beeplite.c feature.c reserved.c\
                  string_util.c local.c censor.c cowmain.c

INPUTOBJ        = input.obj redraw.obj 
MAINOBJ         = main.obj
MAINSRC         = main.c

RSASRC = 
RSAOBJ = 

WIN32_SRCS = winmain.c mswindow.c winsndlib.c
WIN32_OBJS = winmain.obj mswindow.obj winsndlib.obj

RANDOMOBJ = random.obj

INCLUDES        = struct.h packets.h defs.h copyright.h bitmaps.h data.h\
                  oldbitmaps.h tngbitmaps.h hullbitmaps.h rabbitbitmaps.h\
                  sound.h audio.h litebitmaps.h

all: netrek.exe

cflags.c: mkcflags.exe config.h mkfilent.mak
		mkcflags "$(cc) $(cflags)" "$(ARCH)"  > cflags.c
		echo char cwho[]="$(CWHO)"; >> cflags.c

mkcflags.exe: mkcflags.c patchlevel.h version.h config.h mkfilent.mak
		$(compileandlink) $(cflags) $(DEFS) $(INCS) mkcflags.c 


OBJS = $(ROBJ) $(MAINOBJ) $(RSAOBJ) $(INPUTOBJ) $(WIN32_OBJS) 
$(RANDOMOBJ)

.c.obj:
        $(cc) $(cflags) $(ccompileonly) $(cdebug) $(DEFS) $(INCS) $<

!IFNDEF MSVC
RESOURCES = netrek.rc
!ELSE
RESOURCES = netrek.res
$(RESOURCES): netrek.rc
        rc netrek.rc
!ENDIF

netrek.mif: mkfilent.mak
		@echo $(conlflags) $(ldebug) $(lout)netrek.exe > $@
!IFDEF MSVC
		@echo netrek.res >> $@
!ENDIF
		@echo $(GMPLIB) >> $@
		for %i in ($(OBJS)) do echo %i >> $@

netrek.exe:  $(OBJS) $(RESOURCES) warn.cur trek.cur main.ico 
netrek.mif
		$(link) @netrek.mif
		copy netrek.exe c:\games\netrek
!IFNDEF MSVC
        $(rc) netrek.rc netrek.exe
!ENDIF
