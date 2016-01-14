ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include docs example

EXTRA_DIST = bootstrap.sh AUTHORS TODO NEWS

pkgconfigdir = $(libdir)/pkgconfig

ChangeLog: NEWS
	git pull; git2cl > ChangeLog
