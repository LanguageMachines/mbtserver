# $Id: Makefile.am 2904 2009-09-07 09:45:22Z sloot $
# $URL: https://ilk.uvt.nl/svn/trunk/sources/Mbt3/Makefile.am $

ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include docs example

EXTRA_DIST = bootstrap AUTHORS TODO NEWS

pkgconfigdir = $(libdir)/pkgconfig

$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
