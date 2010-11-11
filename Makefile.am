# $Id$
# $URL$

ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include docs example

EXTRA_DIST = bootstrap AUTHORS TODO NEWS

pkgconfigdir = $(libdir)/pkgconfig


$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
