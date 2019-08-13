ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include docs example

EXTRA_DIST = bootstrap.sh AUTHORS TODO NEWS README.md mbtserver.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mbtserver.pc

ChangeLog: NEWS
	git pull; git2cl > ChangeLog
