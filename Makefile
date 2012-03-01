########################################################################
#  No editables below this point.                                      #
########################################################################

PACKAGE=netrek-client-cow
VERSION=$(shell ./name)
DVERSION=$(shell head -1 debian/changelog|cut -f2 -d\(|cut -f1 -d\))

all: netrek-client-cow

netrek-client-cow: system.mk netrekI

netrekI::
	$(MAKE) -f system.mk netrek-client-cow

netrek.shared: name system.mk
	$(MAKE) -f system.mk S=SHARED netrek.shared

profile:
	$(MAKE) -f system.mk OPT="-ggdb3 -pg -a" EXTRALINKFLAGS="-ggdb3 -pg -a" netrek-client-cow

clean:
	rm -f *.o $(OBJ) $(SHAREDTARGET)

reallyclean: clean
	rm -f netrek-client-cow name mkcflags \
	config.h system.mk config.status config.log config.cache \
	null netrek.shared lib* cflags.c \
	po/Makefile po/Makefile.in

distclean: clean reallyclean

tags: system.mk
	$(MAKE) -f system.mk tags

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

distbin: name netrek-client-cow
	-strip netrek-client-cow
	-rm -f $(PACKAGE)-$(VERSION).$(ARCH)
	cp netrek-client-cow $(PACKAGE)-$(VERSION).$(ARCH)
	-rm -f $(PACKAGE)-$(VERSION).$(ARCH).gz
	gzip -9 $(PACKAGE)-$(VERSION).$(ARCH)

name: name.c version.h patchlevel.h
	$(CC) $(CFLAGS) -o name name.c

depend: system.mk
	$(MAKE) -f system.mk depend

system.mk: Makefile system.mk.in config.h.in configure install.sh
	./configure $(CONFFLAGS)
	$(MAKE) -f system.mk depend

install.sh:
	touch install.sh

XTREKRC: system.mk
	$(MAKE) -f system.mk xtrekrc
	mv xtrekrc XTREKRC

configure: configure.in
	rm -f configure
	$(AUTOCONF) configure.in > configure
	chmod +x configure

unproto: system.mk
	$(MAKE) -f system.mk unproto

proto: system.mk
	$(MAKE) -f system.mk proto

indent: system.mk
	$(MAKE) -f system.mk indent

to_unix: system.mk
	$(MAKE) -f system.mk to_unix

to_dos: system.mk
	$(MAKE) -f system.mk to_dos

install: system.mk
	$(MAKE) -f system.mk install

package:
	fakeroot dpkg-buildpackage -Igtk -Ipygtk -Ipyqt

# targets specific to quozl
WWW=~/public_html/external/mine/netrek

upload:
	mv ../$(PACKAGE)_$(VERSION)*{.dsc,.changes,.tar.gz,.deb} $(WWW)

update:
	(cd $(WWW);make)

release: package upload update
