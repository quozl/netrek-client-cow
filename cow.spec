#
# $Id: cow.spec,v 1.6 2002/05/06 08:24:43 tanner Exp $
#
# HOW TO COMPILE
#
# on redhat 7.2:           --define "keydef path/to/key.def"
#
# rpm -ba --define "keydef /home/basic/.key.def" cow.spec
# rpm --rebuild --define "keydef /home/basic/.key.def" cow-3.00_XXXX.src.rpm
# 
# If you do not define a key.def file, I'll default to the sample_key.def found in the
# cow source distribution. Please read the FAQ about blessed clients if this does not
# make sense to you <http://www.inl.org/netrek/netrekFAQ.html#10>
#
Summary: Netrek Client
Name: cow
Version: 3.00_20020506
Release: realtime.4
Copyright: Undetermined
URL: http://cow.netrek.org/
Vendor: Real Time Enterprises, Inc. <support@real-time.com>
Packager: Real Time Enterprises, Inc. <support@real-time.com>
Distribution: Red Hat Linux 7.2 / i386
Serial: 1
Group: Amusements/Games
Source0: %{name}-%{version}.tar.bz2
Source2: COW-Sound.3.00.tar.gz
Source3: pixmaps.tgz
Source4: COW.3.00pl2.doc.tar.gz
Source5: cow.desktop
Source6: cow.png
Patch0: cow-3.00-xpmfix.patch

#
# Sorry, I don't distribute my key. Even with the source code, see the COW.DOC file
# on generating your own key
#
#Source10: key.def
BuildRequires: gmp-devel, kde1-compat-devel, qt1x-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Requires: gmp, kde1-compat, qt1x, tclug-menu

%description
This is a client for the multi-player game of Netrek.

Netrek is the probably the first video game which can accurately be described
as a "sport."  It has more in common with basketball than with arcade games or
Quake.  Its vast and expanding array of tactics and strategies allows for many
different play styles; the best players are the ones who think fastest, not
necessarily the ones who twitch most effectively.  It can be enjoyed as a
twitch game, since the dogfighting system is extremely robust, but the things
that really set Netrek apart from other video games are the team and strategic
aspects.  Team play is dynamic and varied, with roles constantly changing as
the game state changes.  Strategic play is explored in organized league games;
after 6+ years of league play, strategies are still being invented and refined.

The game itself has existed for over 10 years, and has a solid playerbase,
including some people who have been playing for nearly as long as the game has
existed.

All Netrek clients and servers are completely free of charge, although there
are several people working on commercial netrek variants or derivatives.

Netrek web site:          <http://www.netrek.org/>
Development web site:     <http://cow.netrek.org/>

To start the client program, run /usr/bin/netrek, and a list of servers should
be displayed.  

%prep

%setup -q -a 2 -a 3 -a 4
%patch0 -p1 

%build

%{__autoconf}
%configure --enable-unstable
#
# If we find a keydef then use it, otherwise use the sample_key.def
#
%{__make} OPT="$RPM_OPT_FLAGS" %{?keydef:KEYDEF="%{keydef}"}

%{__make} OPT="$RPM_OPT_FLAGS" \
	KDEDIR="/usr/lib/kde1-compat" \
	LFLAGS="-L/usr/lib/kde1-compat/lib -L/usr/lib/qt-2.3.1/lib -lmediatool -lqt" \
	-C sound/soundlib

%install
rm -rf %{buildroot}

%{__install} -m 755 -d %{buildroot}%{_sysconfdir}/X11/applnk/Games/Tclug
%{__install} -m 755 -d %{buildroot}%{_datadir}/gnome/ximian/Programs/Games/Tclug
%{__install} -m 755 -d %{buildroot}%{_datadir}/gnome/apps/Games/Tclug
%{__install} -m 755 -d %{buildroot}%{_datadir}/pixmaps
%{__install} -m 755 -d %{buildroot}%{_bindir}
%{__install} -m 755 -d %{buildroot}%{_datadir}/sounds/%{name}
%{__install} -m 755 -d %{buildroot}%{_datadir}/pixmaps/%{name}

%{__install} %SOURCE5 %{buildroot}%{_sysconfdir}/X11/applnk/Games/Tclug
%{__install} %SOURCE5 %{buildroot}%{_datadir}/gnome/ximian/Programs/Games/Tclug
%{__install} %SOURCE5 %{buildroot}%{_datadir}/gnome/apps/Games/Tclug
%{__install} %SOURCE6 %{buildroot}%{_datadir}/pixmaps
%{__install} -m 755 -s netrek %{buildroot}%{_bindir}/cow
%{__install} -m 755 -s sound/soundlib/bgsndplay %{buildroot}%{_bindir}

# Using tar to keep symlinks
(cd sound/sounds/; tar -cf - .)|(cd %{buildroot}%{_datadir}/sounds/%{name}; tar -xf -)
(cd pixmaps; tar -cp \
    --exclude readme.txt \
    --exclude rotate.bas \
    --exclude setarace.bat \
    --exclude setrace.bat \
    -f - .) | (cd %{buildroot}%{_datadir}/pixmaps/%{name}; tar -xpf -)

%files
%defattr(-,root,root)
%doc %{name}-docs-3.00pl12/*
%doc sound/SOUND.DOC 
%doc pixmaps/readme.txt
%{_bindir}/%{name}
%{_bindir}/bgsndplay
%attr(0755,root,root) %dir %{_datadir}/sounds/%{name}
%attr(0444,root,root) %{_datadir}/sounds/%{name}/*
%attr(0755,root,root) %dir %{_datadir}/pixmaps/%{name}
%attr(0444,root,root) %{_datadir}/pixmaps/%{name}/*
%attr(0644,root,root)%{_sysconfdir}/X11/applnk/Games/Tclug/%{name}.desktop
%attr(0644,root,root)%{_datadir}/gnome/apps/Games/Tclug/%{name}.desktop
%attr(0644,root,root)%{_datadir}/gnome/ximian/Programs/Games/Tclug/%{name}.desktop
%attr(0644,root,root)%{_datadir}/pixmaps/%{name}.png

%clean
rm -rf %{buildroot}

%changelog
* Sat May 06 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00_20040504-realtime.4
  - submitted keys to metaserver, recompiled binaries for those keys
  - added ability to pass into the rpm build process the location of keydef file
  - fixed permission on pixmap directory
  - patch [Bug #552772]

* Sat May 04 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00_20040504-realtime.2
  - first attempt at building cow for source. previous rpm was binary only
  - changed hard coded commands to rpm macros
  - setup compile of bgsndplay 

* Tue Jul 24 2001 James Cameron <quozl@us.netrek.org>
  + cow-3.00pl12-1
  - intitial spec file
  - this date is just a guess using rcs2log and looking for the first entry
