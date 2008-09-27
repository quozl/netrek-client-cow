########################################################################
#  No editables below this point.                                      #
########################################################################

include key.mk

KEYGOD = clientkeys@clientkeys.netrek.org
MAIL   = mail

include $(KEYDEF)

PACKAGE=netrek-client-cow
VERSION=$(shell ./name)
DVERSION=$(shell head -1 debian/changelog|cut -f2 -d\(|cut -f1 -d\))

all: netrek 

netrek: system.mk netrekI

netrekI::
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) netrek

netrek.shared: name system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) S=SHARED netrek.shared

profile:
	$(MAKE) -f system.mk OPT="-ggdb3 -pg -a" EXTRALINKFLAGS="-ggdb3 -pg -a" KEYDEF=$(KEYDEF) netrek

java: netrek.shared
	cd java; $(MAKE)

convert: mkkey $(KEYFILE) $(KEYSH)
	./mkkey -h $(KEYSH) $(KEYFILE) "Client Of Win" \
	$(DESC) $(MAKER) $(COMMENT)

newkey: mkkey
	./mkkey $(KEYFILE) "Client Of Win" $(DESC) $(MAKER) \
	 $(COMMENT) "inl,standard2"

mkkey: system.mk 
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) mkkey

clean:
	rm -f *.o $(OBJ) $(SHAREDTARGET)
	cd java; $(MAKE) clean

reallyclean: clean
	rm -f netrek randomize mkkey rsa_box*.c name mkcflags \
	config.h system.mk config.status config.log config.cache key.mail null \
	netrek.shared lib*

distclean: clean reallyclean

tags: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) tags

names: name
	@echo "PACKAGE=$(PACKAGE)"
	@echo "VERSION=$(VERSION)"
	@echo "PACKAGE_VERSION=$(PACKAGE)-$(VERSION)"
	@echo "DEBIAN_PACKAGE_VERSION=$(PACKAGE)-$(DVERSION)"

dist: name
	mkdir $(PACKAGE)-$(VERSION)
	tar cf - `cat manifest` | (cd $(PACKAGE)-$(VERSION);tar xf -)
	tar cvf - $(PACKAGE)-$(VERSION) | gzip -9 > $(PACKAGE)-$(VERSION).tar.gz
	rm -rf $(PACKAGE)-$(VERSION)

distdoc: name XTREKRC
	mkdir $(PACKAGE)-$(VERSION).doc
	tar cf - README.* COW.DOC CHANGES XTREKRC netrekrc.example \
		*.html *.css stars.gif | (cd $(PACKAGE)-$(VERSION).doc; tar xf -)
	tar cvf - $(PACKAGE)-$(VERSION).doc | gzip -9 > $(PACKAGE)-$(VERSION).doc.tar.gz
	rm -rf $(PACKAGE)-$(VERSION).doc

distbin: name netrek
	-strip netrek
	-rm -f $(PACKAGE)-$(VERSION).$(ARCH)
	cp netrek $(PACKAGE)-$(VERSION).$(ARCH)
	-rm -f $(PACKAGE)-$(VERSION).$(ARCH).gz
	gzip -9 $(PACKAGE)-$(VERSION).$(ARCH)

distkey: netrek $(KEYFILE)
	echo "This is an automatic generated mail." >key.mail
	echo "Please add the following $(ARCH) COW key to the metaserver:" >>key.mail
	echo "" >>key.mail
	cat $(KEYFILE) >>key.mail
	echo "" >>key.mail
	./netrek -v >>key.mail
	cat key.mail | $(MAIL) $(KEYGOD)

name: name.c version.h patchlevel.h
	$(CC) $(CFLAGS) -o name name.c

depend: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) depend

system.mk: Makefile system.mk.in config.h.in configure install.sh $(KEYDEF)
	./configure $(CONFFLAGS)
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) depend

install.sh:
	touch install.sh

XTREKRC: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) xtrekrc
	mv xtrekrc XTREKRC	

configure: configure.in
	rm -f configure
	$(AUTOCONF) configure.in > configure
	chmod +x configure

unproto: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) unproto

proto: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) proto

indent: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) indent

to_unix: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) to_unix

to_dos: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) to_dos

install: netrek
	if [ -f $(BINDIR)/netrek ] ; then \
	  mv $(BINDIR)/netrek $(BINDIR)/netrek.old ; \
	fi 
	install -cs netrek $(DESTDIR)$(BINDIR)

install.alpha: netrek
	rm -f $(ALPHADIR)/netrek.gz
	install -cs -m 755 netrek $(ALPHADIR)
	gzip -9 $(ALPHADIR)/netrek
	install -c -m 644 README.rel $(ALPHADIR)/rel.README
	./netrek -v | head -1 > $(ALPHADIR)/HEADER
	chmod 644 $(ALPHADIR)/HEADER

package:
	fakeroot dpkg-buildpackage -Igtk -Ipygtk -Ipyqt

# targets specific to quozl
WWW=~/public_html/external/mine/netrek

upload:
	mv ../$(PACKAGE)_$(VERSION)*{.dsc,.changes,.tar.gz,.deb} $(WWW)

update:
	(cd $(WWW);make)

release: package upload update
