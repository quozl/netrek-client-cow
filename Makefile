########################################################################
#  CHANGE THE KEYDEF LINE TO THE .def FILE OF YOUR SYSTEM AND RSA KEY  #
########################################################################

KEYDEF = sample_key.def

########################################################################
#  No editables below this point.                                      #
########################################################################

SHELL  = /bin/sh

KEYGOD = clientkeys@clientkeys.netrek.org
MAIL   = mail
AUTOCONF = autoconf

include $(KEYDEF)

all : netrek 

netrek: system.mk netrekI

netrekI: 
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) netrek

netrek.shared: name system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) S=SHARED netrek.shared

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
	-$(RM) *.o $(OBJ) $(SHAREDTARGET)
	cd java; $(MAKE) clean

reallyclean: clean
	rm -f netrek randomize mkkey rsa_box*.c name mkcflags \
	config.h system.mk config.status config.log config.cache key.mail null \
	netrek.shared lib*

distclean: clean reallyclean

tags: system.mk
	$(MAKE) -f system.mk KEYDEF=$(KEYDEF) tags

dist: name
	mkdir COW.`./name`
	tar cf - `cat manifest` | (cd COW.`./name`;tar xf -)
	tar cvf - COW.`./name` | gzip -9 > COW.`./name`.tar.gz
	rm -rf COW.`./name`

distdoc: name XTREKRC
	mkdir COW.`./name`.doc
	tar cf - README.* COW.DOC CHANGES XTREKRC netrekrc.example \
		*.html *.css stars.gif | (cd COW.`./name`.doc; tar xf -)
	tar cvf - COW.`./name`.doc | gzip -9 > COW.`./name`.doc.tar.gz
	rm -rf COW.`./name`.doc

distbin: name netrek
	-strip netrek
	-rm -f COW.`./name`.$(ARCH)
	cp netrek COW.`./name`.$(ARCH)
	-rm -f COW.`./name`.$(ARCH).gz
	gzip -9 COW.`./name`.$(ARCH)

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

# make patches file from old directory
patches: name
	cd $(PATCHDIR); make name
	@echo Making COW.`./name`\-`$(PATCHDIR)/name`.diffs
	-rm ../COW.`./name`\-`$(PATCHDIR)/name`.diffs
	-for f in `cat manifest` ; do \
	if [ ! -f $(PATCHDIR)/$${f} ] ; then touch $(PATCHDIR)/$${f} ; fi ; \
	diff -w -r -c $(PATCHDIR)/$${f} $${f} >> ../COW.`./name`\-`$(PATCHDIR)/name`.diffs ; \
	done

# make patches file from specified tar file (slower than above)
tarpatches: name
	@echo Making COW.`name`\-$(OLD).diffs
	-rm ../COW.`name`\-$(OLD).diffs
	-for f in `cat manifest` ; do \
	echo tar -xOzf $(TF) $${f} \| diff -c - $${f} ;\
	tar -xOzf $(TF) $${f} | diff -c - $${f} >> ../COW.`name`\-$(OLD).diffs ; \
	done

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
	install -cs netrek $(BINDIR)

install.alpha: netrek
	rm -f $(ALPHADIR)/netrek.gz
	install -cs -m 755 netrek $(ALPHADIR)
	gzip -9 $(ALPHADIR)/netrek
	install -c -m 644 README.rel $(ALPHADIR)/rel.README
	./netrek -v | head -1 > $(ALPHADIR)/HEADER
	chmod 644 $(ALPHADIR)/HEADER
