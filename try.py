#!/usr/bin/python
"""
    pygame netrek
    Copyright (C) 2007  James Cameron (quozl@us.netrek.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

"""
import sys, socket, struct, pygame

verbose = 0

FED=0x1
ROM=0x2
KLI=0x4
ORI=0x8
GWIDTH=100000

SCOUT=0
DESTROYER=1
CRUISER=2
BATTLESHIP=3
ASSAULT=4
STARBASE=5
SGALAXY=6
ATT=7

def strnul(input):
    """ convert a NUL terminated string to a normal string
    """
    return input.split('\000')[0]

def scale(x, y):
    """ temporary coordinate scaling, galactic to screen
    """
    return (x/100, y/100)

def descale(x, y):
    """ temporary coordinate scaling, screen to galactic
    """
    return (x*100, y*100)

class IC:
    """ an image cache
    """
    def __init__(self):
        self.cache = {}

    def read(self, name):
        image = pygame.image.load(name)
        return pygame.Surface.convert_alpha(image)

    def get(self, name):
        if name not in self.cache:
            self.cache[name] = self.read(name)
        return self.cache[name]
        
ic = IC()
    
class Planet(pygame.sprite.Sprite):
    """ netrek planets
    """
    def __init__(self, n):
        pygame.sprite.Sprite.__init__(self)
        self.n = n
        self.x = self.old_x = -10000
        self.y = self.old_y = -10000
        self.name = self.old_name = ''
        self.image = ic.get("romulus-1.png")
        self.rect = self.image.get_rect()
        sprites.add(self)

    def update(self):
        if self.x != self.old_x or self.y != self.old_y:
            self.rect.center = scale(self.x, self.y)
            self.old_x = self.x
            self.old_y = self.y
        
    def sp_planet_loc(self, x, y, name):
        self.x = x
        self.y = y
        self.name = name
        # FIXME: render planet name on screen

    def sp_planet(self, owner, info, flags, armies):
        # FIXME: use args
        # FIXME: render planet owner, flags and armies on screen
        pass

class Ship(pygame.sprite.Sprite):
    """ netrek ships
    """
    def __init__(self, n):
        pygame.sprite.Sprite.__init__(self)
        self.n = n
        self.x = self.old_x = -10000
        self.y = self.old_y = -10000
        self.image = ic.get("netrek.png")
        self.rect = self.image.get_rect()

    def show(self):
        sprites.add(self)

    def hide(self):
        sprites.remove(self)

    def update(self):
        if self.x != self.old_x or self.y != self.old_y:
            self.rect.center = scale(self.x, self.y)
            self.old_x = self.x
            self.old_y = self.y
        # FIXME: render ship rotation, using pygame.transform.rotate
        
    def sp_player(self, dir, speed, x, y):
        self.dir = dir
        self.speed = speed
        self.x = x
        self.y = y

    def sp_pstatus(self, status):
        if status == 2:
            self.show()
        else:
            self.hide()
#define PFREE 0
#define POUTFIT 1
#define PALIVE 2
#define PEXPLODE 3
#define PDEAD 4
#define POBSERV 5

class Galaxy:
    def __init__(self):
        self.planets = {}
        self.ships = {}

    def planet(self, n):
        if not self.planets.has_key(n):
            planet = Planet(n)
            self.planets[n] = planet
        return self.planets[n]

    def ship(self, n):
        if not self.ships.has_key(n):
            self.ships[n] = Ship(n)
        return self.ships[n]

    def nearest_planet(self, x, y):
        x, y = descale(x, y)
        nearest = None
        minimum = GWIDTH**2
        for n, planet in self.planets.iteritems():
            distance = (planet.x - x)**2 + (planet.y - y)**2
            if distance < minimum:
                nearest = planet
                minimum = distance
        return nearest

galaxy = Galaxy()
me = None

def team_decode(input):
    """ convert a team mask to a list
    """
    x = []
    if input & FED: x.append('F')
    if input & ROM: x.append('R')
    if input & KLI: x.append('K')
    if input & ORI: x.append('O')
    return x

""" client originated packets
"""

class CP:
    def tabulate(self, number, format):
        global cp_table
        cp_table[number] = (struct.calcsize(format), format)

cp_table = {}

class CP_SOCKET(CP):
    def __init__(self):
        self.code = 27
        self.format = '!bbbxI'
        self.tabulate(self.code, self.format)

    def data(self):
        if verbose: print "CP_SOCKET"
        return struct.pack(self.format, self.code, 4, 10, 0)

cp_socket = CP_SOCKET()

class CP_BYE(CP):
    def __init__(self):
        self.code = 29
        self.format = '!bxxx'
        self.tabulate(self.code, self.format)

    def data(self):
        print "CP_BYE"
        return struct.pack(self.format, self.code)

cp_bye = CP_BYE()

class CP_LOGIN(CP):
    def __init__(self):
        self.code = 8
        self.format = '!bbxx16s16s16s' 
        self.tabulate(self.code, self.format)

    def data(self, query, name, password, login):
        print "CP_LOGIN query=",query,"name=",name
        return struct.pack(self.format, self.code, query, name, password, login)

cp_login = CP_LOGIN()

class CP_OUTFIT(CP):
    def __init__(self):
        self.code = 9
        self.format = '!bbbx'
        self.tabulate(self.code, self.format)

    def data(self, team, ship=SCOUT):
        print "CP_OUTFIT team=",team_decode(team),"ship=",ship
        return struct.pack(self.format, self.code, team, ship)

cp_outfit = CP_OUTFIT()

class CP_SPEED(CP):
    def __init__(self):
        self.code = 2
        self.format = '!bbxx'
        self.tabulate(self.code, self.format)

    def data(self, speed):
        print "CP_SPEED speed=",speed
        return struct.pack(self.format, self.code, speed)

cp_speed = CP_SPEED()

class CP_DIRECTION(CP):
    def __init__(self):
        self.code = 3
        self.format = '!bBxx'
        self.tabulate(self.code, self.format)

    def data(self, direction):
        print "CP_DIRECTION direction=",direction
        return struct.pack(self.format, self.code, direction)

cp_direction = CP_DIRECTION()

class CP_PLANLOCK(CP):
    def __init__(self):
        self.code = 15
        self.format = '!bbxx'
        self.tabulate(self.code, self.format)

    def data(self, pnum):
        print "CP_PLANLOCK pnum=",pnum
        return struct.pack(self.format, self.code, pnum)

cp_planlock = CP_PLANLOCK()

""" server originated packets
"""

class SP:
    def tabulate(self, number, format, instance):
        global sp_table
        sp_table[number] = (struct.calcsize(format), instance)

    def find(self, number):
        """ given a packet type return a tuple consisting of
            (size, instance)
        """
        global sp_table
        if not sp_table.has_key(number):
            return (1, self)
        return sp_table[number]

sp_table = {}
sp = SP()

class SP_MOTD(SP):
    def __init__(self):
        self.code = 11
        self.format = '!bxxx80s'
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, message) = struct.unpack(self.format, data)
        print strnul(message)

sp_motd = SP_MOTD()

class SP_YOU(SP):
    def __init__(self):
        self.code = 12
        self.format = '!bbbbbbxxIlllhhhh'
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, hostile, swar, armies, tractor, flags, damage,
         shield, fuel, etemp, wtemp, whydead, whodead) = struct.unpack(self.format, data)
        if verbose: print "SP_YOU pnum=",pnum,"hostile=",team_decode(hostile),"swar=",team_decode(swar),"armies=",armies,"tractor=",tractor,"flags=",flags,"damage=",damage,"shield=",shield,"fuel=",fuel,"etemp=",etemp,"wtemp=",wtemp,"whydead=",whydead,"whodead=",whodead
        me = galaxy.ship(pnum)
        ## FIXME: handle the packet
        global pending_login
        if pending_login:
            nt_send(cp_login.data(0, 'guest', '', 'try'))
            pending_login = False

sp_you = SP_YOU()

class SP_PL_LOGIN(SP):
    def __init__(self):
        self.code = 24
        self.format = "!bbbx16s16s16s" 
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, rank, name, monitor,
         login) = struct.unpack(self.format, data)
        if verbose: print "SP_PL_LOGIN pnum=",pnum,"rank=",rank,"name=",strnul(name),"monitor=",strnul(monitor),"login=",strnul(login)

sp_pl_login = SP_PL_LOGIN()

class SP_HOSTILE(SP):
    def __init__(self):
        self.code = 22
        self.format = "!bbbb"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, war, hostile) = struct.unpack(self.format, data)
        if verbose: print "SP_HOSTILE pnum=",pnum,"war=",team_decode(war),"hostile=",team_decode(hostile)

sp_hostile = SP_HOSTILE()

class SP_PLAYER_INFO(SP):
    def __init__(self):
        self.code = 2
        self.format = "!bbbb"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, shiptype, team) = struct.unpack(self.format, data)
        if verbose: print "SP_PLAYER_INFO pnum=",pnum,"shiptype=",shiptype,"team=",team_decode(team)

sp_player_info = SP_PLAYER_INFO()

class SP_KILLS(SP):
    def __init__(self):
        self.code = 3
        self.format = "!bbxxI"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, kills) = struct.unpack(self.format, data)
        if verbose: print "SP_KILLS pnum=",pnum,"kills=",kills

sp_kills = SP_KILLS()

class SP_PSTATUS(SP):
    def __init__(self):
        self.code = 20
        self.format = "!bbbx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, status) = struct.unpack(self.format, data)
        if verbose: print "SP_PSTATUS pnum=",pnum,"status=",status
        ship = galaxy.ship(pnum)
        ship.sp_pstatus(status)

sp_pstatus = SP_PSTATUS()

class SP_PLAYER(SP):
    def __init__(self):
        self.code = 4
        self.format = "!bbBbll"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, dir, speed, x, y) = struct.unpack(self.format, data)
        if verbose: print "SP_PLAYER pnum=",pnum,"dir=",dir,"speed=",speed,"x=",x,"y=",y
        ship = galaxy.ship(pnum)
        ship.sp_player(dir, speed, x, y)

sp_player = SP_PLAYER()

class SP_FLAGS(SP):
    def __init__(self):
        self.code = 18
        self.format = "!bbbxI"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, tractor, flags) = struct.unpack(self.format, data)
        if verbose: print "SP_FLAGS pnum=",pnum,"tractor=",tractor,"flags=",flags

sp_flags = SP_FLAGS()

class SP_PLANET_LOC(SP):
    def __init__(self):
        self.code = 26
        self.format = "!bbxxll16s" 
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, x, y, name) = struct.unpack(self.format, data)
        if verbose: print "SP_PLANET_LOC pnum=",pnum,"x=",x,"y=",y,"name=",strnul(name)
        planet = galaxy.planet(pnum)
        planet.sp_planet_loc(x, y, name)

sp_planet_loc = SP_PLANET_LOC()

class SP_LOGIN(SP):
    def __init__(self):
        self.code = 17
        self.format = "!bbxxl96s"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, accept, flags, keymap) = struct.unpack(self.format, data)
        if verbose: print "SP_LOGIN accept=",accept,"flags=",flags

sp_login = SP_LOGIN()

class SP_MASK(SP):
    def __init__(self):
        self.code = 19
        self.format = "!bbxx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, mask) = struct.unpack(self.format, data)
        if verbose: print "SP_MASK mask=",team_decode(mask)
        global pending_outfit
        if pending_outfit:
            nt_send(cp_outfit.data(0))
            pending_outfit = False

sp_mask = SP_MASK()

class SP_PICKOK(SP):
    def __init__(self):
        self.code = 16
        self.format = "!bbxx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, state) = struct.unpack(self.format, data)
        if verbose: print "SP_PICKOK state=",state

sp_pickok = SP_PICKOK()

class SP_RESERVED(SP):
    def __init__(self):
        self.code = 25
        self.format = "!bxxx16s"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, data) = struct.unpack(self.format, data)
        data = struct.unpack('16b', data)
        if verbose: print "SP_RESERVED data=",data

sp_reserved = SP_RESERVED()

class SP_TORP_INFO(SP):
    def __init__(self):
        self.code = 5
        self.format = "!bbbxhxx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, war, status, tnum) = struct.unpack(self.format, data)
        if verbose: print "SP_TORP_INFO war=",team_decode(war),"status=",status,"tnum=",tnum

sp_torp_info = SP_TORP_INFO()

class SP_TORP(SP):
    def __init__(self):
        self.code = 6
        self.format = "!bBhll"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, dir, tnum, x, y) = struct.unpack(self.format, data)
        if verbose: print "SP_TORP dir=",dir,"tnum=",tnum,"x=",x,"y=",y

sp_torp = SP_TORP()

class SP_PLASMA_INFO(SP):
    def __init__(self):
        self.code = 8
        self.format = "!bbbxhxx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, war, status, pnum) = struct.unpack(self.format, data)
        if verbose: print "SP_PLASMA_INFO war=",team_decode(war),"status=",status,"pnum=",pnum

sp_plasma_info = SP_PLASMA_INFO()

class SP_PLASMA(SP):
    def __init__(self):
        self.code = 9
        self.format = "!bxhll"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, x, y) = struct.unpack(self.format, data)
        if verbose: print "SP_PLASMA pnum=",pnum,"x=",x,"y=",y

sp_plasma = SP_PLASMA()

class SP_STATUS(SP):
    def __init__(self):
        self.code = 14
        self.format = "!bbxxIIIIIL"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, tourn, armsbomb, planets, kills, losses, time, timeprod) = struct.unpack(self.format, data)
        if verbose: print "SP_STATUS tourn=",tourn,"armsbomb=",armsbomb,"planets=",planets,"kills=",kills,"losses=",losses,"time=",time,"timepro=",timeprod

sp_status = SP_STATUS()

class SP_PHASER(SP):
    def __init__(self):
        self.code = 7
        self.format = "!bbbBlll" 
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, status, dir, x, y, target) = struct.unpack(self.format, data)
        if verbose: print "SP_PHASER pnum=",pnum,"status=",status,"dir=",dir,"x=",x,"y=",y,"target=",target

sp_phaser = SP_PHASER()

class SP_PLANET(SP):
    def __init__(self):
        self.code = 15
        self.format = "!bbbbhxxl" 
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, owner, info, flags, armies) = struct.unpack(self.format, data)
        if verbose: print "SP_PLANET pnum=",pnum,"owner=",owner,"info=",info,"flags=",flags,"armies=",armies
        planet = galaxy.planet(pnum)
        planet.sp_planet(owner, info, flags, armies)

sp_planet = SP_PLANET()

class SP_MESSAGE(SP):
    def __init__(self):
        self.code = 1
        self.format = "!bBBB80s"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, m_flags, m_recpt, m_from, mesg) = struct.unpack(self.format, data)
        if verbose: print "SP_MESSAGE m_flags=",m_flags,"m_recpt=",m_recpt,"m_from=",m_from,"mesg=",strnul(mesg)

sp_message = SP_MESSAGE()

class SP_STATS(SP):
    def __init__(self):
        self.code = 23
        self.format = "!bbxx13l"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, tkills, tlosses, kills, losses, tticks, tplanets, tarmies, sbkills, sblosses, armies, planets, maxkills, sbmaxkills) = struct.unpack(self.format, data)
        if verbose: print "SP_STATS pnum=",pnum,"tkills=",tkills,"tlosses=",tlosses,"kills=",kills,"losses=",losses,"tticks=",tticks,"tplanets=",tplanets,"tarmies=",tarmies,"sbkills=",sbkills,"sblosses=",sblosses,"armies=",armies,"planets=",planets,"maxkills=",maxkills,"sbmaxkills=",sbmaxkills

sp_stats = SP_STATS()

class SP_WARNING(SP):
    def __init__(self):
        self.code = 10
        self.format = '!bxxx80s'
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, message) = struct.unpack(self.format, data)
        print strnul(message)
        # FIXME: display the warning

sp_warning = SP_WARNING()

""" Netrek TCP
"""

s = None

def nt_connect(host, port):
    global s
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.setblocking(0)
    #s.settimeout(0.1)

def nt_send(data):
    global s
    s.send(data)

def nt_recv_one(byte):
    global s
    # FIXME: when server closes connection, we get something other than a byte
    number = struct.unpack('b', byte[0])[0]
    (size, instance) = sp.find(number)
    if size == 1:
        print "\n#### FIXME: UnknownPacketType ", number, "####\n"
        raise "UnknownPacketType, a packet was received from the server that is not known to this program, and since packet lengths are determined by packet types there is no reasonably way to continue operation"
        return
    s.setblocking(1)
    rest = s.recv(size-1, socket.MSG_WAITALL)
    if len(rest) != (size-1):
        print "### asked for %d and got %d bytes" % ((size-1), len(rest))
    instance.handler(byte + rest)
    s.setblocking(0)
    # FIXME: packet almalgamation may occur, s.recv second time may
    # return something less than the expected number of bytes, so we
    # have to wait for them.

def nt_recv():
    global s
    while 1:
        try:
            byte = s.recv(1)
            nt_recv_one(byte)
        except:
            return

def kb(key):
    """ keydown event handler
    """
    if event.key == pygame.K_SPACE:
        nt_send(cp_login.data(0, 'guest', '', 'try'))
    elif event.key == pygame.K_TAB:
        nt_send(cp_outfit.data(0, 0))
    elif event.key == pygame.K_q:
        screen.fill((0, 0, 0))
        pygame.display.flip()
        nt_send(cp_bye.data())
        sys.exit()
    elif event.key == pygame.K_LSHIFT: pass
    elif event.key == pygame.K_0: nt_send(cp_speed.data(0))
    elif event.key == pygame.K_1: nt_send(cp_speed.data(1))
    elif event.key == pygame.K_2 and (event.mod == pygame.KMOD_SHIFT or event.mod == pygame.KMOD_LSHIFT): nt_send(cp_speed.data(12))
    elif event.key == pygame.K_2: nt_send(cp_speed.data(2))
    elif event.key == pygame.K_3: nt_send(cp_speed.data(3))
    elif event.key == pygame.K_4: nt_send(cp_speed.data(4))
    elif event.key == pygame.K_5: nt_send(cp_speed.data(5))
    elif event.key == pygame.K_6: nt_send(cp_speed.data(6))
    elif event.key == pygame.K_7: nt_send(cp_speed.data(7))
    elif event.key == pygame.K_8: nt_send(cp_speed.data(8))
    elif event.key == pygame.K_9: nt_send(cp_speed.data(9))
    elif event.key == pygame.K_SEMICOLON:
        x, y = pygame.mouse.get_pos()
        nearest = galaxy.nearest_planet(x, y)
        if nearest != None:
            nt_send(cp_planlock.data(nearest.n))
        else:
            print "no nearest"
    else:
        print "kb: unhandled keydown, key=", event.key, "mod=", event.mod

def mb(position, button):
    """ mouse button down event handler
    position is a list of (x, y) screen coordinates
    button is a mouse button number
    """
    global me
    print position, button
    if button == 3 and me != None:
        print me.x, me.y
        nt_send(cp_direction.data(0))
    pass

# socket http://docs.python.org/lib/socket-objects.html
# struct http://docs.python.org/lib/module-struct.html
# built-ins http://docs.python.org/lib/built-in-funcs.html

# packages that may do network in pygame
# python-poker2d
# http://www.linux-games.com/castle-combat/

# general protocol state outline
#
# starting state
# CP_SOCKET
# CP_FEATURE (to indicate feature packets are known)
# SP_MOTD
# SP_FEATURE
# SP_YOU
# client shows name and password prompt and accepts input
# CP_LOGIN
# CP_FEATURE
# SP_LOGIN
# SP_YOU (identifies the slot number)
# SP_PLAYER_INFO
# SP_MASK
# client shows team selection window and accepts input
# CP_OUTFIT
# SP_PICKOK
# server places ship in game and play begins
# SP_? indicates POUTFIT state, returning client to team selection window
# CP_QUIT

# need sizes of each packet type, make a c program to dump the sizes,
# need structure definitions for each

pygame.init()

size = width, height = 1000, 1000

screen = pygame.display.set_mode(size)
sprites = pygame.sprite.OrderedUpdates(())

#background = ic.get("stars.png")
#screen.blit(background, (0, 0))
# FIXME: tile the background
#pygame.display.flip()

pending_login = True
pending_outfit = True

nt_connect(sys.argv[1], 2592)
if sys.argv[2] == 'verbose':
    verbose = 1
nt_send(cp_socket.data())

while 1:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            nt_send(cp_bye.data())
            sys.exit()
        elif event.type == pygame.KEYDOWN:
            kb(event.key)
        elif event.type == pygame.MOUSEBUTTONDOWN:
            mb(event.pos, event.button)

    # FIXME: select for *either* pygame events or network events
    # currently the code checks for network events at high rate
    # consuming CPU unnecessarily
    nt_recv()
    sprites.update()
    pygame.display.update(sprites.draw(screen))
