Summary: Netrek Client
Name: netrek-client
Version: 3.00pl1
Release: 2
Copyright: Undetermined
Packager: quozl@us.netrek.org
URL: http://cow.netrek.org/
Group: Amusements/Games
Source0: http://cow.netrek.org/current/COW.3.00pl1.ix86_linux.gz

%description
This is a client for the multi-player game of Netrek.

Netrek is the probably the first video game which can accurately be
described as a "sport."  It has more in common with basketball than
with arcade games or Quake.  Its vast and expanding array of tactics
and strategies allows for many different play styles; the best players
are the ones who think fastest, not necessarily the ones who twitch
most effectively.  It can be enjoyed as a twitch game, since the
dogfighting system is extremely robust, but the things that really set
Netrek apart from other video games are the team and strategic
aspects.  Team play is dynamic and varied, with roles constantly
changing as the game state changes.  Strategic play is explored in
organized league games; after 6+ years of league play, strategies are
still being invented and refined.

The game itself has existed for over 10 years, and has a solid
playerbase, including some people who have been playing for nearly as
long as the game has existed.

All Netrek clients and servers are completely free of charge, although
there are several people working on commercial netrek variants or
derivatives.

Netrek web site:          <http://www.netrek.org/>
Development web site:     <http://cow.netrek.org/>

To start the client program, run /usr/bin/netrek, and a list of
servers should be displayed.  See also /usr/doc/cow-3.00pl1/index.html

%prep
rm -rf $RPM_BUILD_ROOT/usr/games/cow-3.00pl1/
rm -rf $RPM_BUILD_ROOT/usr/doc/cow-3.00pl1/
rm -rf $RPM_BUILD_ROOT/usr/bin/netrek
mkdir -p $RPM_BUILD_ROOT/usr/games/cow-3.00pl1
mkdir -p $RPM_BUILD_ROOT/usr/doc/cow-3.00pl1

%install
#
#	Unpack the binary distribution in the Right Places.
#
install $RPM_SOURCE_DIR/COW.3.00pl1.ix86_linux \
	$RPM_BUILD_ROOT/usr/games/cow-3.00pl1/
cd /usr/games/cow-3.00pl1
tar xfz $RPM_SOURCE_DIR/pixmaps.tgz
tar xfz $RPM_SOURCE_DIR/COW-Sound.3.00.tar.gz
cd /usr/doc/cow-3.00pl1
tar xfz $RPM_SOURCE_DIR/COW.3.00pl1.doc.tar.gz
#
#	Create script for starting client
#	(which creates a working .xtrekrc file if one is not there)
#
cat << EOF > $RPM_BUILD_ROOT/usr/bin/netrek
#!/bin/sh
cd /usr/games/cow-3.00pl1
if [ ! -f ~/.xtrekrc ]
then
	cat << eox > ~/.xtrekrc
# your .xtrekrc was created by /usr/bin/netrek
# for further documentation see /usr/doc/cow-3.00pl1/index.html
#
# enable sound, point to sound files and player program
sound: on
sounddir: /usr/games/cow-3.00pl1/sound/sounds
soundplayer: /usr/games/cow-3.00pl1/sound/bgsndplay
# point to the pixmaps directory
pixmapDir: /usr/games/cow-3.00pl1/pixmaps
eox
fi
./COW.3.00pl1.ix86_linux -r ~/.xtrekrc -m
EOF
#
#	Fix protections on script.
#
chmod +x $RPM_BUILD_ROOT/usr/bin/netrek
#
#	Remove the KDE sound player, because it creates a libmediatool
#	dependency for the package.
#
cd $RPM_BUILD_ROOT/usr/games/cow-3.00pl1/sound
rm bgsndplay.linux.kde
rm bgsndplay
mv bgsndplay.linux bgsndplay
#
#	Fix ownerships on all files.
#	(games username is not consistently available across distributions)
#
chown -R root:root \
	$RPM_BUILD_ROOT/usr/games/cow-3.00pl1 \
	$RPM_BUILD_ROOT/usr/doc/cow-3.00pl1 
#
#	Decompress the pixmaps that have arrrived in the package as compressed,
#	so as to lose the warning created by COW on startup.
#
gunzip -f $RPM_BUILD_ROOT/usr/games/cow-3.00pl1/pixmaps/Misc/genocide.xpm.gz
gunzip -f $RPM_BUILD_ROOT/usr/games/cow-3.00pl1/pixmaps/Misc/greet.xpm.gz
gunzip -f $RPM_BUILD_ROOT/usr/games/cow-3.00pl1/pixmaps/Misc/hockey.xpm.gz
rm $RPM_BUILD_ROOT/usr/games/cow-3.00pl1/pixmaps/Misc/ghostbust.xpm.gz
cd $RPM_BUILD_ROOT/usr/games/cow-3.00pl1/pixmaps/Misc/
ln -sf genocide.xpm ghostbust.xpm

%files
/usr/games/cow-3.00pl1/
/usr/doc/cow-3.00pl1/
/usr/bin/netrek

%clean

%changelog
