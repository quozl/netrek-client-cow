#
# $Id: cow.spec,v 1.9 2002/07/07 22:22:11 tanner Exp $
#
# HOW TO COMPILE
#
# on redhat:           --define "keydef path/to/key.def"
#
# rpm -ba --define "keydef /home/basic/.key.def" cow.spec
# rpm --rebuild --define "keydef /home/basic/.key.def" cow-3.00_XXXX.src.rpm
# 
# If you do not define a key.def file, I'll default to the sample_key.def 
# found in the cow source distribution. Please read the FAQ about blessed 
# clients if this does not make sense to you 
# <http://www.inl.org/netrek/netrekFAQ.html#10>
#

Summary: Netrek Client
Name: cow
Version: 3.01pl0
Release: realtime.1
Copyright: Undetermined
URL: http://cow.netrek.org/
Vendor: Real Time Enterprises, Inc. <support@real-time.com>
Packager: Bob Tanner <basic@us.netrek.org>
Distribution: Red Hat Linux 7.3 / i386
Group: Amusements/Games
Source0: %{name}-%{version}.tar.bz2
Source3: pixmaps.tgz
Source4: COW.3.00pl2.doc.tar.gz
%define png	$RPM_BUILD_DIR/%{name}-%{version}/cow.png
%define desktop $RPM_BUILD_DIR/%{name}-%{version}/cow.desktop

#
# Sorry, I don't distribute my key with the source code,  see the COW.DOC file 
# on generating your own key
#
#Source10: key.def
#
# You can get the tclug-menu at Twin Cities Linux Users Group official ftp
# server <ftp://ftp.mn-linux.org/linux/apt/realtime/7.3/i386/RPMS.tclug/>
# The tclug-menu package just setups up the appropriate menu entries for cow.
#
Requires: gmp, tclug-menu, SDL >= 1.2.4, SDL_mixer >= 1.2.4
BuildRequires: gmp-devel, SDL-devel >= 1.2.4, SDL_mixer-devel >= 1.2.4
BuildRoot: %{_tmppath}/%{name}-%{version}-root

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

%setup -q -a 3 -a 4

%build
#%{__autoconf}
%configure --enable-unstable
#
# If we find a keydef then use it, otherwise use the sample_key.def
#
%{__make} OPT="$RPM_OPT_FLAGS" %{?keydef:KEYDEF="%{keydef}"} %{?_smp_mflags}

%install
rm -rf %{buildroot}

#
# Redhat GNOME and Ximian GNOME desktop directories
#
%{__install} -m 755 -d %{buildroot}%{_sysconfdir}/X11/applnk/Games/Tclug
%{__install} -m 755 -d %{buildroot}%{_datadir}/gnome/ximian/Programs/Games/Tclug
%{__install} -m 755 -d %{buildroot}%{_datadir}/gnome/apps/Games/Tclug
#
# KDE desktop directory
#
%{__install} -m 755 -d %{buildroot}%{_datadir}/applnk/Games/Arcade
%{__install} -m 755 -d %{buildroot}%{_datadir}/icons/hicolor/16x16/apps
%{__install} -m 755 -d %{buildroot}%{_datadir}/icons/hicolor/32x32/apps
%{__install} -m 755 -d %{buildroot}%{_datadir}/icons/hicolor/48x48/apps
%{__install} -m 755 -d %{buildroot}%{_datadir}/icons/locolor/16x16/apps
%{__install} -m 755 -d %{buildroot}%{_datadir}/icons/locolor/32x32/apps
%{__install} -m 755 -d %{buildroot}%{_datadir}/icons/locolor/48x48/apps
#
# Common directories
#
%{__install} -m 755 -d %{buildroot}%{_datadir}/pixmaps
%{__install} -m 755 -d %{buildroot}%{_bindir}

%{__install} -m 755 -d %{buildroot}%{_datadir}/sounds/%{name}
%{__install} -m 755 -d %{buildroot}%{_datadir}/pixmaps/%{name}
#
# Install desktop entries in GNOME areas
#
%{__install} %{desktop} %{buildroot}%{_sysconfdir}/X11/applnk/Games/Tclug
%{__install} %{desktop} %{buildroot}%{_datadir}/gnome/ximian/Programs/Games/Tclug
%{__install} %{desktop} %{buildroot}%{_datadir}/gnome/apps/Games/Tclug
#
# Install desktop entry into KDE areas
#
%{__install} %{desktop} %{buildroot}%{_datadir}/applnk/Games/Arcade
%{__install} %{png} %{buildroot}%{_datadir}/icons/hicolor/16x16/apps
%{__install} %{png} %{buildroot}%{_datadir}/icons/hicolor/32x32/apps
%{__install} %{png} %{buildroot}%{_datadir}/icons/hicolor/48x48/apps
%{__install} %{png} %{buildroot}%{_datadir}/icons/locolor/16x16/apps
%{__install} %{png} %{buildroot}%{_datadir}/icons/locolor/32x32/apps
%{__install} %{png} %{buildroot}%{_datadir}/icons/locolor/48x48/apps
#
# Common files
#
%{__install} %{png} %{buildroot}%{_datadir}/pixmaps
%{__install} -m 755 -s netrek %{buildroot}%{_bindir}/cow

# Using tar to keep symlinks
(cd spike/cow-test/sounds/; tar -cf - .)|(cd %{buildroot}%{_datadir}/sounds/%{name}; tar -xf -)
(cd pixmaps; tar -cp \
    --exclude readme.txt \
    --exclude rotate.bas \
    --exclude setarace.bat \
    --exclude setrace.bat \
    -f - .) | (cd %{buildroot}%{_datadir}/pixmaps/%{name}; tar -xpf -)

%files
%defattr(-,root,root)
%doc %{name}-docs-3.00pl12/*
%doc pixmaps/readme.txt
%{_bindir}/%{name}
%attr(0755,root,root) %dir %{_datadir}/sounds/%{name}
%attr(-,root,root) %{_datadir}/sounds/%{name}/*
%attr(0755,root,root) %dir %{_datadir}/pixmaps/%{name}
%attr(-,root,root) %{_datadir}/pixmaps/%{name}/*
%attr(0644,root,root)%{_sysconfdir}/X11/applnk/Games/Tclug/%{name}.desktop
%attr(0644,root,root)%{_datadir}/gnome/apps/Games/Tclug/%{name}.desktop
%attr(0644,root,root)%{_datadir}/gnome/ximian/Programs/Games/Tclug/%{name}.desktop
%attr(0644,root,root)%{_datadir}/applnk/Games/Arcade/%{name}.desktop
%attr(0644,root,root)%{_datadir}/icons/hicolor/16x16/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/icons/hicolor/32x32/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/icons/hicolor/16x16/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/icons/hicolor/32x32/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/icons/hicolor/48x48/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/icons/locolor/16x16/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/icons/locolor/32x32/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/icons/locolor/48x48/apps/%{name}.png
%attr(0644,root,root)%{_datadir}/pixmaps/%{name}.png

%clean
rm -rf %{buildroot}

%changelog
* Sun Jul 07 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.01p-realtime.1
  - rolled new RPM from HEAD of cow tree

* Fri Jun 21 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00p3-SDL-realtime.8
  - rolled new RPM from HEAD of cow tree

* Sat Jun 16 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00_20020616-realtime.7
  - first release of cow with SDL_mixer sound support

* Sat May 06 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00_20020504-realtime.6
  - small patch to fix compilation under redhat 7.3 an kde3

* Sat May 06 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00_20020504-realtime.5
  - patch [Bug #553113] to fix sound problems under 2.4.x kernels/redhat 7.2

* Sat May 06 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00_20020504-realtime.4
  - submitted keys to metaserver, recompiled binaries for those keys
  - added ability to pass into the rpm build process the location of keydef file
  - fixed permission on pixmap directory
  - patch [Bug #552772] to fix configure not detecting xpm under redhat 7.2

* Sat May 04 2002 Bob Tanner <tanner@real-time.com>
  + cow-3.00_20020504-realtime.2
  - first attempt at building cow for source. previous rpm was binary only
  - changed hard coded commands to rpm macros
  - setup compile of bgsndplay 

* Tue Jul 24 2001 James Cameron <quozl@us.netrek.org>
  + cow-3.00pl12-1
  - intitial spec file
  - this date is just a guess using rcs2log and looking for the first entry
