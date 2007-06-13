#!/usr/bin/python
"""
    netrek lurk, version 1.0
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

--

Usage: lurk.py server-name [verbose]

"""
import sys, socket, select, struct

""" utility functions """

def strnul(input):
    """ convert a NUL terminated string to a normal string
    """
    return input.split('\000')[0]

""" netrek protocol documentation, from server include/packets.h

	general protocol state outline

	starting state

	CP_SOCKET
	CP_FEATURE, optional, to indicate feature packets are known
	SP_MOTD
	SP_FEATURE, only if CP_FEATURE was seen
	SP_QUEUE, optional, repeats until slot is available
	SP_YOU, indicates slot number assigned

	login state, player slot status is POUTFIT
	client shows name and password prompt and accepts input

	CP_LOGIN
	CP_FEATURE
	SP_LOGIN
	SP_YOU
	SP_PLAYER_INFO
	various other server packets

	outfit state, player slot status is POUTFIT
	client shows team selection window

	SP_MASK, sent regularly during outfit

	client accepts team selection input
	CP_OUTFIT
	SP_PICKOK, signals server acceptance of alive state

	alive state,
	server places ship in game and play begins

	SP_PSTATUS, indicates PDEAD state
	client animates explosion

	SP_PSTATUS, indicates POUTFIT state
	clients returns to team selection window

	CP_QUIT
	CP_BYE
"""

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
        print "CP_LOGIN"
        return struct.pack(self.format, self.code, query, name, password, login)

cp_login = CP_LOGIN()

class CP_OUTFIT(CP):
    def __init__(self):
        self.code = 9
        self.format = '!bbbx'
        self.tabulate(self.code, self.format)

    def data(self, team, ship=0):
        print "CP_OUTFIT"
        return struct.pack(self.format, self.code, team, ship)

cp_outfit = CP_OUTFIT()

class CP_UPDATES(CP):
    def __init__(self):
        self.code = 31
        self.format = '!bxxxI'
        self.tabulate(self.code, self.format)

    def data(self, usecs):
        print "CP_UPDATES"
        return struct.pack(self.format, self.code, usecs)

cp_updates = CP_UPDATES()

class CP_MESSAGE(CP):
    def __init__(self):
        self.code = 1
        self.format = "!bBBx80s"
        self.tabulate(self.code, self.format)

    def data(self, group, indiv, mesg):
        print "CP_MESSAGE group=",group,"indiv=",indiv,"mesg=",mesg
        return struct.pack(self.format, self.code, group, indiv, mesg)

cp_message = CP_MESSAGE()

class CP_QUIT(CP):
    def __init__(self):
        self.code = 7
        self.format = '!bxxx'
        self.tabulate(self.code, self.format)

    def data(self):
        print "CP_QUIT"
        return struct.pack(self.format, self.code)

cp_quit = CP_QUIT()

class CP_RESERVED(CP):
    def __init__(self):
        self.code = 33
        self.format = "!bxxx16s16s" 
        self.tabulate(self.code, self.format)

    def data(self, data, resp):
        print "CP_RESERVED"
        return struct.pack(self.format, self.code, data, resp)

cp_reserved = CP_RESERVED()

class CP_PING_RESPONSE(CP):
    def __init__(self):
        self.code = 42
        self.format = "!bBbxll" 
        self.tabulate(self.code, self.format)

    def data(self, number, pingme, cp_sent, cp_recv):
        return struct.pack(self.format, self.code, number, pingme, cp_sent, cp_recv)

cp_ping_response = CP_PING_RESPONSE()

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

    def handler(self, data):
        pass

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
        self.armed = True

    def handler(self, data):
        # send one CP_LOGIN when the first SP_YOU is seen
        if self.armed:
            global opt
            nt.send(cp_login.data(0, opt.name, opt.password, opt.login))
            nt.send(cp_updates.data(1000000/opt.updates))
            self.armed = False

sp_you = SP_YOU()

class SP_QUEUE(SP):
    def __init__(self):
        self.code = 13
        self.format = '!bxh'
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pos) = struct.unpack(self.format, data)
        print "SP_QUEUE pos=",pos

sp_queue = SP_QUEUE()

class SP_PL_LOGIN(SP):
    def __init__(self):
        self.code = 24
        self.format = "!bbbx16s16s16s" 
        self.tabulate(self.code, self.format, self)

sp_pl_login = SP_PL_LOGIN()

class SP_HOSTILE(SP):
    def __init__(self):
        self.code = 22
        self.format = "!bbbb"
        self.tabulate(self.code, self.format, self)

sp_hostile = SP_HOSTILE()

class SP_PLAYER_INFO(SP):
    def __init__(self):
        self.code = 2
        self.format = "!bbbb"
        self.tabulate(self.code, self.format, self)

sp_player_info = SP_PLAYER_INFO()

class SP_KILLS(SP):
    def __init__(self):
        self.code = 3
        self.format = "!bbxxI"
        self.tabulate(self.code, self.format, self)

sp_kills = SP_KILLS()

class SP_PSTATUS(SP):
    def __init__(self):
        self.code = 20
        self.format = "!bbbx"
        self.tabulate(self.code, self.format, self)

sp_pstatus = SP_PSTATUS()

class SP_PLAYER(SP):
    def __init__(self):
        self.code = 4
        self.format = "!bbBbll"
        self.tabulate(self.code, self.format, self)

sp_player = SP_PLAYER()

class SP_FLAGS(SP):
    def __init__(self):
        self.code = 18
        self.format = "!bbbxI"
        self.tabulate(self.code, self.format, self)

sp_flags = SP_FLAGS()

class SP_PLANET_LOC(SP):
    def __init__(self):
        self.code = 26
        self.format = "!bbxxll16s" 
        self.tabulate(self.code, self.format, self)

sp_planet_loc = SP_PLANET_LOC()

class SP_LOGIN(SP):
    def __init__(self):
        self.code = 17
        self.format = "!bbxxl96s"
        self.tabulate(self.code, self.format, self)

sp_login = SP_LOGIN()

class SP_MASK(SP):
    def __init__(self):
        self.code = 19
        self.format = "!bbxx"
        self.tabulate(self.code, self.format, self)
        self.armed = True

    def handler(self, data):
        (ignored, mask) = struct.unpack(self.format, data)
        print "SP_MASK"
        # automatically join first available team
        if self.armed:
            if mask & 0x1: nt.send(cp_outfit.data(0))
            elif mask & 0x2: nt.send(cp_outfit.data(1))
            elif mask & 0x4: nt.send(cp_outfit.data(2))
            elif mask & 0x8: nt.send(cp_outfit.data(3))
            self.armed = False

sp_mask = SP_MASK()

class SP_PICKOK(SP):
    def __init__(self):
        self.code = 16
        self.format = "!bbxx"
        self.tabulate(self.code, self.format, self)

sp_pickok = SP_PICKOK()

class SP_RESERVED(SP):
    def __init__(self):
        self.code = 25
        self.format = "!bxxx16s"
        self.tabulate(self.code, self.format, self)

sp_reserved = SP_RESERVED()

class SP_TORP_INFO(SP):
    def __init__(self):
        self.code = 5
        self.format = "!bbbxhxx"
        self.tabulate(self.code, self.format, self)

sp_torp_info = SP_TORP_INFO()

class SP_TORP(SP):
    def __init__(self):
        self.code = 6
        self.format = "!bBhll"
        self.tabulate(self.code, self.format, self)

sp_torp = SP_TORP()

class SP_PLASMA_INFO(SP):
    def __init__(self):
        self.code = 8
        self.format = "!bbbxhxx"
        self.tabulate(self.code, self.format, self)

sp_plasma_info = SP_PLASMA_INFO()

class SP_PLASMA(SP):
    def __init__(self):
        self.code = 9
        self.format = "!bxhll"
        self.tabulate(self.code, self.format, self)

sp_plasma = SP_PLASMA()

class SP_STATUS(SP):
    def __init__(self):
        self.code = 14
        self.format = "!bbxxIIIIIL"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, tourn, armsbomb, planets, kills, losses, time, timeprod) = struct.unpack(self.format, data)
        global opt
        # exit on t-mode transition if directed to do so
        if tourn == 1 and opt.twu: sys.exit()
        if tourn == 0 and opt.twd: sys.exit()

sp_status = SP_STATUS()

class SP_PHASER(SP):
    def __init__(self):
        self.code = 7
        self.format = "!bbbBlll" 
        self.tabulate(self.code, self.format, self)

sp_phaser = SP_PHASER()

class SP_PLANET(SP):
    def __init__(self):
        self.code = 15
        self.format = "!bbbbhxxl" 
        self.tabulate(self.code, self.format, self)

sp_planet = SP_PLANET()

class SP_MESSAGE(SP):
    def __init__(self):
        self.code = 1
        self.format = "!bBBB80s"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, m_flags, m_recpt, m_from, mesg) = struct.unpack(self.format, data)
        print strnul(mesg)

sp_message = SP_MESSAGE()

class SP_STATS(SP):
    def __init__(self):
        self.code = 23
        self.format = "!bbxx13l"
        self.tabulate(self.code, self.format, self)

sp_stats = SP_STATS()

class SP_WARNING(SP):
    def __init__(self):
        self.code = 10
        self.format = '!bxxx80s'
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, message) = struct.unpack(self.format, data)
        print strnul(message)

sp_warning = SP_WARNING()

class SP_FEATURE(SP):
    def __init__(self):
        self.code = 60
        self.format = "!bbbbi80s"
        self.tabulate(self.code, self.format, self)

sp_feature = SP_FEATURE()

class SP_BADVERSION(SP):
    def __init__(self):
        self.code = 21
        self.format = "!bbxx"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, why) = struct.unpack(self.format, data)
        print "SP_BADVERSION why=",why

sp_badversion = SP_BADVERSION()

class SP_PING(SP):
    def __init__(self):
        self.code = 46
        self.format = "!bBHBBBB" 
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, number, lag, tloss_sc, tloss_cs, iloss_sc, iloss_cs) = struct.unpack(self.format, data)
        nt.send(cp_ping_response.data(0, 1, 0, 0))

sp_ping = SP_PING()

## end of server packets

class Client:
    """ Netrek TCP Client
    """
    def __init__(self):
        self.socket = None
        
    def connect(self, host, port):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((host, port))
        
    def send(self, data):
        self.socket.send(data)

    def recv(self, time):
        while 1:
            is_readable = [self.socket]
            is_writable = []
            is_error = []
            r, w, e = select.select(is_readable, is_writable, is_error, time)
            if not r: return
            try:
                byte = self.socket.recv(1)
            except:
                print "recv failure"
                sys.exit()
            if len(byte) == 1:
                self.recv_packet(byte)
            else:
                print "server disconnection"
                sys.exit()

    def recv_packet(self, byte):
        number = struct.unpack('b', byte[0])[0]
        (size, instance) = sp.find(number)
        if size == 1:
            print "\n#### FIXME: UnknownPacketType ", number, "####\n"
            raise "UnknownPacketType, a packet was received from the server that is not known to this program, and since packet lengths are determined by packet types there is no reasonably way to continue operation"
            return
        rest = self.socket.recv(size-1, socket.MSG_WAITALL)
        if len(rest) != (size-1):
            print "### asked for %d and got %d bytes" % ((size-1), len(rest))
        # handle the prefix byte and the rest of the packet as a whole
        instance.handler(byte + rest)
        # FIXME: packet almalgamation may occur, s.recv second time may
        # return something less than the expected number of bytes, so we
        # have to wait for them.

from optparse import OptionParser
parser= OptionParser()
parser.add_option("-s", "--server", dest="server",
                  help="netrek server to connect to")
parser.add_option("-p", "--port", type="int", dest="port", default="2593",
                  help="netrek observer port number to connect to")
parser.add_option("--name", dest="name", default="guest",
                  help="character name to show, default guest")
parser.add_option("--password", dest="password", default="",
                  help="password for character name")
parser.add_option("--login", dest="login", default="lurker",
                  help="username to show on player list")
parser.add_option("-u", "--tournament-wait-up", "--for-fun",
                  action="store_true", dest="twu", default="False",
                  help="exit when t-mode begins or if already begun")
parser.add_option("-d", "--tournament-wait-down", "--for-aid",
                  action="store_true", dest="twd", default="False",
                  help="exit when t-mode ends or if not yet t-mode")
parser.add_option("-r", "--updates",
                  type="int", dest="updates", default="1",
                  help="updates per second from server, default 1")
(opt, args) = parser.parse_args()

def main():
    global nt
    nt = Client()
    nt.connect(opt.server, int(opt.port))
    nt.send(cp_socket.data())
    try:
        while 1:
            nt.recv(1.0)
            nt.send(cp_ping_response.data(0, 1, 0, 0))
    except:
        nt.send(cp_bye.data())
        sys.exit()

main()
