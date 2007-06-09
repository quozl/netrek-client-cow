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

http://www.cs.cmu.edu/afs/cs.cmu.edu/user/jch/netrek/humor

From: markiel@callisto.pas.rochester.edu (Andrew Markiel)
Newsgroups: rec.games.netrek
Subject: Re: Beginer!
Date: Thu, 4 Nov 93 00:33:05 GMT

STHOMAS@utcvm.utc.edu wrote:
>      I read many of the netrek postings and it seems real intresting.  Would
> anyone be willing to post instructions on how to get onto Netrek and begin
> playinf.  Particularly comand cods to get in.

Certainly! Most of them you can fish out of the code (it's written in sea),
but I can give you some pointers.

The idea behind netrek is that your race starts with 10 oysters, which
produce pearls (certain "agricultural" oysters produce pearls much faster
and thus are very valuable). The object of the game is to capture all of
the enemy's oysters by destroying all of their pearls and capturing them
with your own pearls. Each team has 8 fish attacking and defending the
oysters.

One of the most important ideas is to destroy enemy pearls (once you
destroy all the pearls on an enemy oyster, it becomes uncontrolled and
you can capture it by delivering one of your own pearls). You do this
by perching on the oyster and hitting 'b' (for bash), which causes
you to destroy the enemy pearls there.

If the oyster has more than 4 pearls, then it will be open and you
can attack all of them; however, once the oyster has less than 5, it
will close up with the pearls inside and you can't bash them anymore
(however, if you get lucky, you can bash several pearls in one swing
and thus reduce the number of pearls to below 4 before it closes up..
lobsters are very good at this).

Once the oyster is closed up, the only way to destroy the rest of the
pearls inside is to deliver your own pearls to the oyster. When you
put one of your own pearls next to the oyster, it will open up slightly
to crush it, which allows you to grab one of the ones inside and destroy
it before it closes again. Thus, for an oyster with 4 pearls, you need
to deliver 4 of your own pearls to be able to destroy all of the ones
inside, which makes the oyster uncontrolled. Then, if you put another
one of your own pearls inside, you will capture the oyster and it will
start making pearls for you.

The tricky bit is that to carry pearls around, you have to have a net
to carry them in. You make the net out of the scales of the enemy fish
that you defeat. Defeating one enemy fish gives you enough scales to
carry 2 pearls (unless you are in a lobster, in which case you have
enough to carry three pearls). They maximum number of pearls you can
carry depends on fish type. When your fish is defeated in combat,
you lose all your scales and any pearls you are carrying, and get sent
back to your spawning grounds to get a new fish. 

You fight either by tossing pebbles at the enemy fish, or by using
your scraper to scrape the scales off of them; note that after a
lot of fighting you get tired, and can't attack until you get more food
(indicated by your food stat, note that many other things, including
movement, consume food). You can get a little food anywhere, but certain
oysters provide food, which means that if you perch on a freindly food
oyster you will replace your food reserves much faster.

There are six types of fish you can control: salmon, dogfish, crabs,
bluefin, lobsters, and the mighty bass. You can always switch to
another fish by going back to your spawning ground and requesting
a new fish (and you keep whatever scales you had).

Salmon are small and fast. The have fast pebbles but very weak
scrapers, and they are too weak for most combat. They are best used
either for carrying pearls, or for bashing enemy pearls in their
backfield (every good team needs a salmon basher).

Dogfish are a little tougher than salmon, and also conserve food well.
However, they also have weak scrapers and thus aren't so useful in combat.
In fact, these fish tend to be seldom seen anymore.

Crabs are the workfish. They have lots of food and really good
scrapers, which makes them good for fighting enemy fish. Nowadays
these fish are used more than any other.

Bluefin are very big fish, but are slow and don't use food effectively,
so they tend to be useful only in close range fights near a food oyster.

Lobsters are an intersting sort of fish. They are very slow, but are
very good at bashing enemy pearls (if they get lucky, they can bash
4 pearls in one swing). They also can carry three pearls for each
enemy fish destroyed, which can be quite useful.

The bass is a very special fish. You side only can have one at a time,
and if it gets defeated it takes 30 minutes to put it back together
again. You also have a certain rank to take one. However, they are
very big and tough, and serve as a repository for pearls (where they
can't be bashed).

An interesting tactic is the bass og (it's short for ogtopus). This
is where your 8 fish surround the enemy bass (like the 8 arms of an
ogtopus). Then you wade in and beat the carp out of him. You can do
it with less fish, but it's not as effective.


Hope this helps. If you have more questions, the FAQ should be posted
in a few days.

> Stephen Thomas  University of Tennesee at Chattanooga

-Grey Elf
markiel@callisto.pas.rochester.edu

------------------------------


"""
import sys, socket, select, struct, pygame

verbose = 0
updates_per_second = 5

FED=0x1
ROM=0x2
KLI=0x4
ORI=0x8
GWIDTH=100000

PFREE=0
POUTFIT=1
PALIVE=2
PEXPLODE=3
PDEAD=4
POBSERV=5

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

def dir_to_angle(dir):
    """ convert netrek direction to angle, approximate
    """
    return dir * 360 / 256 / 10 * 10

def team_decode(input):
    """ convert a team mask to a list
    """
    x = []
    if input & FED: x.append('F')
    if input & ROM: x.append('R')
    if input & KLI: x.append('K')
    if input & ORI: x.append('O')
    return x

class IC:
    """ an image cache
    """
    def __init__(self):
        self.cache = {}
        self.cache_rotated = {}

    def read(self, name):
        image = pygame.image.load(name)
        return pygame.Surface.convert_alpha(image)

    def get(self, name):
        if name not in self.cache:
            self.cache[name] = self.read(name)
        return self.cache[name]

    def get_rotated(self, name, angle):
        if (name, angle) not in self.cache_rotated:
            unrotated = self.get(name)
            rotated = pygame.transform.rotate(unrotated, -angle)
            self.cache_rotated[(name, angle)] = rotated
        return self.cache_rotated[(name, angle)]
        
ic = IC()
    
class Planet(pygame.sprite.Sprite):
    """ netrek planets
    """
    def __init__(self, n):
        pygame.sprite.Sprite.__init__(self)
        self.n = n
        self.x = self.old_x = -10000
        self.y = self.old_y = -10000
        self.armies = self.old_armies = 50
        self.name = self.old_name = ''
        self.image = self.image_closed = ic.get("oyster-closed.png")
        self.rect = self.rect_closed = self.image.get_rect()
        self.image_open = ic.get("oyster-open.png")
        self.rect_open = self.image_open.get_rect()
        sprites.add(self)

    def update(self):
        if self.armies != self.old_armies:
            if self.armies > 4:
                self.image = self.image_closed
                self.rect = self.rect_closed
            else:
                self.image = self.image_open
                self.rect = self.rect_open
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
        self.armies = armies
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
        self.dir = self.old_dir = 0
        self.image = ic.get("fish-1.png")
        self.rect = self.image.get_rect()

    def show(self):
        sprites.add(self)

    def hide(self):
        sprites.remove(self)

    def update(self):
        if self.dir != self.old_dir:
            self.image = ic.get_rotated("fish-1.png", self.dir)
            self.rect = self.image.get_rect()
            self.old_dir = self.dir
        if self.x != self.old_x or self.y != self.old_y:
            self.rect.center = scale(self.x, self.y)
            self.old_x = self.x
            self.old_y = self.y
        
    def sp_player_info(self, shiptype, team):
        self.shiptype = shiptype
        self.team = team
        # FIXME: display shiptype and team

    def sp_kills(self, kills):
        self.kills = kills
        # FIXME: display kills

    def sp_player(self, dir, speed, x, y):
        self.dir = dir_to_angle(dir)
        self.speed = speed
        self.x = x
        self.y = y

    def sp_pstatus(self, status):
        if status == PALIVE:
            self.show()
        else:
            self.hide()

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

class CP_UPDATES(CP):
    def __init__(self):
        self.code = 31
        self.format = '!bxxxI'
        self.tabulate(self.code, self.format)

    def data(self, usecs):
        print "CP_UPDATES usecs=",usecs
        return struct.pack(self.format, self.code, usecs)

cp_updates = CP_UPDATES()

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
            nt.send(cp_updates.data(1000000/updates_per_second))
            nt.send(cp_login.data(0, 'guest', '', 'try'))
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
        ship = galaxy.ship(pnum)
        ship.sp_player_info(shiptype, team)

sp_player_info = SP_PLAYER_INFO()

class SP_KILLS(SP):
    def __init__(self):
        self.code = 3
        self.format = "!bbxxI"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, pnum, kills) = struct.unpack(self.format, data)
        if verbose: print "SP_KILLS pnum=",pnum,"kills=",kills
        ship = galaxy.ship(pnum)
        ship.sp_kills(kills)

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
            nt.send(cp_outfit.data(0))
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

class SP_FEATURE(SP):
    def __init__(self):
        self.code = 60
        self.format = "!bbbbi80s"
        self.tabulate(self.code, self.format, self)

    def handler(self, data):
        (ignored, type, arg1, arg2, value, name) = struct.unpack(self.format, data)
        # FIXME: process the packet

sp_feature = SP_FEATURE()

## end of server packets

## from Xlib.display import Display
## from Xlib import X
## class XlibEventSource:
##     """ incomplete code for obtaining an Xlib event source that can be
##     used to wake the client when keyboard or mouse events occur,
##     usable by select.
##     """
##     def handle_event(event):
##         keycode = event.detail
##         if event.type == X.KeyPress:
##             print event.detail

##     def main():
##         disp = Display()
##         root = disp.screen().root
        
##         root.change_attributes(event_mask = X.KeyPressMask)
##         root.grab_key(49, X.AnyModifier, 1, X.GrabModeAsync, X.GrabModeAsync)
        
##         while 1:
##             event = root.display.next_event()
##             handle_event(event)
##             return

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

    def recv(self):
        # FIXME: a network update may cost more local time in
        # processing than the time between updates from the server,
        # which results in a pause to display updates since this
        # function does not return until the network queue is empty
        # ... this could be detected and CP_UPDATES negotiation made
        # to reduce the update rate.
        while 1:
            is_readable = [self.socket]
            is_writable = []
            is_error = []
            r, w, e = select.select(is_readable, is_writable, is_error, 0.04)
            if not r: return
            try:
                byte = self.socket.recv(1)
            except:
                print "recv failure"
                sys.exit()
            if len(byte) == 1:
                self.recv_packet(byte)
            else:
                # FIXME: when server closes connection, offer to reconnect
                # FIXME: ghostbust occurs if player is inactive, must ping
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

def kb(event):
    """ keydown event handler
    """
    if event.key == pygame.K_SPACE:
        nt.send(cp_login.data(0, 'guest', '', 'try'))
    elif event.key == pygame.K_TAB:
        nt.send(cp_outfit.data(0, 0))
    elif event.key == pygame.K_q:
        screen.fill((0, 0, 0))
        pygame.display.flip()
        nt.send(cp_bye.data())
        sys.exit()
    elif event.key == pygame.K_LSHIFT: pass
    elif event.key == pygame.K_0: nt.send(cp_speed.data(0))
    elif event.key == pygame.K_1: nt.send(cp_speed.data(1))
    elif event.key == pygame.K_2 and (event.mod == pygame.KMOD_SHIFT or event.mod == pygame.KMOD_LSHIFT): nt.send(cp_speed.data(12))
    elif event.key == pygame.K_2: nt.send(cp_speed.data(2))
    elif event.key == pygame.K_3: nt.send(cp_speed.data(3))
    elif event.key == pygame.K_4: nt.send(cp_speed.data(4))
    elif event.key == pygame.K_5: nt.send(cp_speed.data(5))
    elif event.key == pygame.K_6: nt.send(cp_speed.data(6))
    elif event.key == pygame.K_7: nt.send(cp_speed.data(7))
    elif event.key == pygame.K_8: nt.send(cp_speed.data(8))
    elif event.key == pygame.K_9: nt.send(cp_speed.data(9))
    elif event.key == pygame.K_SEMICOLON:
        x, y = pygame.mouse.get_pos()
        nearest = galaxy.nearest_planet(x, y)
        if nearest != None:
            nt.send(cp_planlock.data(nearest.n))
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
        nt.send(cp_direction.data(0))
    pass

def pygame_event_sink():
    """ read and process all pygame events
    """
    for event in pygame.event.get():
        if event.type == pygame.MOUSEBUTTONDOWN:
            mb(event.pos, event.button)
        elif event.type == pygame.KEYDOWN:
            kb(event)
        elif event.type == pygame.QUIT:
            nt.send(cp_bye.data())
            sys.exit()

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

pygame.init()

size = width, height = 1000, 1000
screen = pygame.display.set_mode(size)
sprites = pygame.sprite.OrderedUpdates(())

#background = ic.get("stars.png")
background = screen.copy()
background.fill((255, 255, 255))
#background.fill((0, 0, 0))
screen.blit(background, (0, 0))
# FIXME: tile the background
pygame.display.flip()

pending_login = True
pending_outfit = True

for argv in sys.argv:
    if argv == 'verbose': verbose = 1

nt = Client()
nt.connect(sys.argv[1], 2592)
nt.send(cp_socket.data())

while 1:
    # FIXME: select for *either* pygame events or network events.
    # Currently the code is suboptimal because it waits on network
    # events with a timeout of a twentieth of a second, after which it
    # checks for pygame events.  Therefore pygame events are delayed
    # by up to a twentieth of a second.
    nt.recv()
    pygame_event_sink()
    sprites.clear(screen, background)
    sprites.update()
    pygame.display.update(sprites.draw(screen))
