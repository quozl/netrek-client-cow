#!/usr/bin/python
import sys, socket, struct

def strnul(input):
    """ convert a NUL terminated string to a normal string
    """
    return input.split('\000')[0]

FED=0x1
ROM=0x2
KLI=0x4
ORI=0x8

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
        print "CP_SOCKET"
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
        print "size=", len(data)
        (ignored, pnum, hostile, swar, armies, tractor, flags, damage,
         shield, fuel, etemp, wtemp, whydead, whodead) = struct.unpack(self.format, data)
        print "SP_YOU pnum=",pnum,"hostile=",team_decode(hostile),"swar=",team_decode(swar),"armies=",armies,"tractor=",tractor,"flags=",flags,"damage=",damage,"shield=",shield,"fuel=",fuel,"etemp=",etemp,"wtemp=",wtemp,"whydead=",whydead,"whodead=",whodead
        ## FIXME: handle the packet

sp_you = SP_YOU()

class SP_PL_LOGIN(SP):
    def __init__(self):
        self.code = 24
        self.format = "!bbbx16s16s16s" 
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, rank, name, monitor,
         login) = struct.unpack(self.format, data)
        print "SP_PL_LOGIN pnum=",pnum,"rank=",rank,"name=",strnul(name),"monitor=",strnul(monitor),"login=",strnul(login)

sp_pl_login = SP_PL_LOGIN()

class SP_HOSTILE(SP):
    def __init__(self):
        self.code = 22
        self.format = "!bbbb"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, war, hostile) = struct.unpack(self.format, data)
        print "SP_HOSTILE pnum=",pnum,"war=",team_decode(war),"hostile=",team_decode(hostile)

sp_hostile = SP_HOSTILE()

class SP_PLAYER_INFO(SP):
    def __init__(self):
        self.code = 2
        self.format = "!bbbb"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, shiptype, team) = struct.unpack(self.format, data)
        print "SP_PLAYER_INFO pnum=",pnum,"shiptype=",shiptype,"team=",team_decode(team)

sp_player_info = SP_PLAYER_INFO()

class SP_KILLS(SP):
    def __init__(self):
        self.code = 3
        self.format = "!bbxxI"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, kills) = struct.unpack(self.format, data)
        print "SP_KILLS pnum=",pnum,"kills=",kills

sp_kills = SP_KILLS()

class SP_PSTATUS(SP):
    def __init__(self):
        self.code = 20
        self.format = "!bbbx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, status) = struct.unpack(self.format, data)
        print "SP_PSTATUS pnum=",pnum,"status=",status

sp_pstatus = SP_PSTATUS()

class SP_PLAYER(SP):
    def __init__(self):
        self.code = 4
        self.format = "!bbBbll"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, dir, speed, x, y) = struct.unpack(self.format, data)
        print "SP_PLAYER pnum=",pnum,"dir=",dir,"speed=",speed,"x=",x,"y=",y

sp_player = SP_PLAYER()

class SP_FLAGS(SP):
    def __init__(self):
        self.code = 18
        self.format = "!bbbxI"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, tractor, flags) = struct.unpack(self.format, data)
        print "SP_FLAGS pnum=",pnum,"tractor=",tractor,"flags=",flags

sp_flags = SP_FLAGS()

class SP_PLANET_LOC(SP):
    def __init__(self):
        self.code = 26
        self.format = "!bbxxll16s" 
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, x, y, name) = struct.unpack(self.format, data)
        print "SP_PLANET_LOC pnum=",pnum,"x=",x,"y=",y,"name=",strnul(name)

sp_planet_loc = SP_PLANET_LOC()

class SP_LOGIN(SP):
    def __init__(self):
        self.code = 17
        self.format = "!bbxxl96s"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, accept, flags, keymap) = struct.unpack(self.format, data)
        print "SP_LOGIN accept=",accept,"flags=",flags

sp_login = SP_LOGIN()

class SP_MASK(SP):
    def __init__(self):
        self.code = 19
        self.format = "!bbxx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, mask) = struct.unpack(self.format, data)
        print "SP_MASK mask=",team_decode(mask)

sp_mask = SP_MASK()

##
## progress report, all packet types handled to the point of team selection
##

""" Netrek TCP
"""

s = None

def nt_connect(host, port):
    global s
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    #s.setblocking(0)
    s.settimeout(0.1)

def nt_send(data):
    global s
    s.send(data)

def nt_recv():
    global s
    try:
        byte = s.recv(1)
    except:
        return
    number = struct.unpack('b', byte[0])[0]
    (size, instance) = sp.find(number)
    if size == 1:
        print "#### FIXME: UnknownPacketType ", number, "####"
        raise UnknownPacketType
        return
    print "packet type=", number, "size=", size
    instance.handler(byte + s.recv(size-1))

nt_connect(sys.argv[1], 2592)
nt_send(cp_socket.data())
nt_send(cp_login.data(0, 'guest', '', 'try'))

# socket http://docs.python.org/lib/socket-objects.html
# struct http://docs.python.org/lib/module-struct.html
# built-ins http://docs.python.org/lib/built-in-funcs.html

# packages the may do network in pygame
# python-poker2d
# http://www.linux-games.com/castle-combat/

# send CP_SOCKET version=4, udp_version=10, socket=20380,
# TODO: send CP_FEATURE
# receive SP_MOTD
# receive SP_YOU
# send C->S CP_LOGIN     query=0, name="guest", password="", login="james",
# receive S->C SP_LOGIN     accept=1, flags=0xAF, keymap=" !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
# S->C SP_YOU
# SP_PLAYER_INFO
# S->C SP_MASK      mask=1,
# C->S CP_OUTFIT    team=0, ship=2,
# S->C SP_PICKOK    state=1,
# ... play ...
# C->S CP_QUIT      no args,

# need sizes of each packet type, make a c program to dump the sizes,
# need structure definitions for each

import pygame
pygame.init()

size = width, height = 320, 240
speed = [2, 2]
black = 0, 0, 0

screen = pygame.display.set_mode(size)

ball = pygame.image.load("netrek.png")
ballrect = ball.get_rect()
while 1:
    nt_recv()
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            nt_send(cp_bye.data())
            sys.exit()
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_SPACE:
                nt_send(cp_login.data(0, 'guest', '', 'try'))

    ballrect = ballrect.move(speed)
    if ballrect.left < 0 or ballrect.right > width:
        speed[0] = -speed[0]
    if ballrect.top < 0 or ballrect.bottom > height:
        speed[1] = -speed[1]

    screen.fill(black)
    screen.blit(ball, ballrect)
    pygame.display.flip()
    