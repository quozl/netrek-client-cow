!
!   DESCRIP.MMS
!
!   Make file for OpenVMS for use with MMS or MMK.
!
!   Build COW for OpenVMS with DEC TCP/IP Services for OpenVMS
!
!   Written by James Cameron, (cameron@stl.dec.com), Digital Equipment
!   Corporation, CSS Sydney Engineering, on 16-Jan-1998.
!

cflags = /prefix=all/decc/nowarnings/define=VMS
arch = OpenVMS
cwho = "cameron@stl.dec.com"

.first
	copy config.h_vms config.h
	open/write null null.h
	close null

objects = main, newwin, stats, pingstats, getname, cflags, defaults, socket,-
check, x11window, dmessage, reserved, enter, helpwin, macrowin, findslot,-
colors, input, vmsutils, getship, util, distress, ping, short, inform, redraw,-
beeplite, lagmeter, smessage, parsemeta, netstatopt, netstat, option,-
planetlist, playerlist, senddist, ranklist, rotate, interface, defwin, docwin,-
spopt, strdup, udpopt, death, war, warning, dashboard, tools, data, -
sintab, vmsio, feature, censor, cowmain, local, dashboard3, map, string_util

cow.exe : cow.olb($(objects)), cow.opt
	$(link)$(linkflags) cow.opt/options

cflags.c : mkcflags.exe
	mkcflags = "$sys$disk:[]mkcflags"
	define/user_mode sys$output cflags.c
	mkcflags "$(CC) $(CFLAGS)" "$(ARCH)"
	open/append cflags cflags.c
	write cflags "char cwho[]="$(CWHO)";"
	close cflags

mkcflags.exe : mkcflags.obj cow.olb(vmsutils)
	link mkcflags, cow/library

mkcflags.obj : mkcflags.c config.h version.h patchlevel.h

!
! Manually prepared header dependencies; OpenVMS/VAX does not have makedepend,
! but this can be derived from makedepend running on a system that does have it.!

audio.obj : config.h audio.h

beeplite.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

cflags.obj : config.h

check.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h

colors.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h

dashboard.obj : dashboard.c config.h copyright.h Wlib.h defs.h struct.h data.h

data.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

death.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

defaults.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h

defwin.obj : config.h Wlib.h defs.h struct.h data.h

distress.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

dmessage.obj : config.h copyright.h Wlib.h defs.h struct.h data.h -
version.h patchlevel.h

docwin.obj : config.h copyright.h Wlib.h defs.h struct.h data.h -
packets.h

enter.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

feature.obj : config.h copyright.h Wlib.h defs.h struct.h data.h -
packets.h

findslot.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h

getname.obj : config.h copyright2.h vmsutils.h Wlib.h defs.h -
struct.h data.h

getship.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

helpwin.obj : config.h Wlib.h defs.h struct.h data.h

inform.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

input.obj : config.h copyright.h vmsutils.h Wlib.h defs.h struct.h -
data.h packets.h

interface.obj : config.h copyright.h Wlib.h defs.h struct.h -
data.h packets.h

lagmeter.obj : config.h copyright.h Wlib.h defs.h struct.h data.h -
packets.h

macrowin.obj : config.h Wlib.h defs.h struct.h data.h

main.obj : config.h copyright.h Wlib.h defs.h struct.h data.h packets.h -
version.h patchlevel.h

mkcflags.obj : config.h version.h patchlevel.h

mkkey.obj : config.h defs.h

name.obj : version.h patchlevel.h

netstat.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h -
packets.h

netstatopt.obj : config.h copyright.h Wlib.h defs.h struct.h -
data.h packets.h

newwin.obj : config.h copyright.h vmsutils.h Wlib.h defs.h struct.h -
data.h bitmaps.h moobitmaps.h rabbitbitmaps.h tngbitmaps.h hullbitmaps.h -
oldbitmaps.h packets.h

option.obj : config.h copyright.h Wlib.h defs.h struct.h data.h -
packets.h

parsemeta.obj : config.h copyright.h Wlib.h defs.h struct.h data.h -
vmsutils.h

ping.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h packets.h

pingstats.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

planetlist.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

playerlist.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

randomize.obj : config.h

ranklist.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h

redraw.obj : config.h copyright.h Wlib.h defs.h struct.h data.h -
packets.h

reserved.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h -
packets.h

rotate.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h -
packets.h

senddist.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

short.obj : config.h Wlib.h defs.h struct.h data.h packets.h wtext.h

sintab.obj : config.h copyright.h

smessage.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

socket.obj : config.h copyright2.h Wlib.h defs.h struct.h data.h -
packets.h wtext.h vmsutils.h

sound.obj : config.h copyright.h Wlib.h defs.h struct.h data.h audio.h

spopt.obj : config.h copyright.h Wlib.h defs.h struct.h data.h packets.h

stats.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

tools.obj : config.h Wlib.h defs.h struct.h data.h

udpopt.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

util.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

vmsutils.obj : vmsutils.h

war.obj : config.h copyright.h Wlib.h defs.h struct.h data.h packets.h

warning.obj : config.h copyright.h Wlib.h defs.h struct.h data.h

x11window.obj : config.h vmsutils.h Wlib.h defs.h struct.h data.h -
teams.bitmap mapcursor.bitmap localcursor.bitmap

