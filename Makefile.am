ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = \
	Makefile.in \
	aclocal.m4 \
	config.guess \
	config.h.in \
	config.sub \
	configure \
	depcomp \
	install-sh \
	ltconfig \
	ltmain.sh \
	missing \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.gz \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.bz2 \
	$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-doc.tar.bz2 \
	m4/libtool.m4 \
	m4/lt~obsolete.m4 \
	m4/ltoptions.m4 \
	m4/ltsugar.m4 \
	m4/ltversion.m4

SUBDIRS = src/lzma \
	  src/lib/libzpaq \
	  src \
	  man \
	  doc

dist_doc_DATA = \
	AUTHORS \
	BUGS \
	ChangeLog \
	COPYING \
	README.md \
	README-NOT-BACKWARD-COMPATIBLE \
	TODO \
	WHATS-NEW

EXTRA_DIST = \
	src/lrztar \
	description-pak \
	autogen.sh \
	INSTALL \
	VERSION \
	$(dist_doc_DATA)

install-exec-hook:
	$(LN_S) -f lrzip-next$(EXEEXT) $(DESTDIR)$(bindir)/lrunzip$(EXEEXT)
	$(LN_S) -f lrzip-next$(EXEEXT) $(DESTDIR)$(bindir)/lrzcat$(EXEEXT)
	$(LN_S) -f lrztar$(EXEEXT) $(DESTDIR)$(bindir)/lrzuntar$(EXEEXT)
	$(LN_S) -f lrzip-next$(EXEEXT) $(DESTDIR)$(bindir)/lrz$(EXEEXT)


uninstall-local:
	rm -f $(bindir)/lrunzip
	rm -f $(bindir)/lrzcat
	rm -f $(bindir)/lrzuntar
	rm -f $(bindir)/lrz

.PHONY: doc

# Documentation

doc: all
	@echo "entering doc/"
	$(MAKE) -C doc doc

