# $Id$
# $URL$

ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include docs example

EXTRA_DIST = bootstrap.sh AUTHORS TODO NEWS

pkgconfigdir = $(libdir)/pkgconfig

ChangeLog: NEWS
	svn update; svn2cl --include-rev
