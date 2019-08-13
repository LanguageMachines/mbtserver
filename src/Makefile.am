AM_CPPFLAGS = -I@top_srcdir@/include
AM_CXXFLAGS = -std=c++11

LDADD = libmbtserver.la

bin_PROGRAMS = mbtserver

mbtserver_SOURCES = MbtServer.cxx

lib_LTLIBRARIES = libmbtserver.la
libmbtserver_la_LDFLAGS= -version-info 1:0:0

libmbtserver_la_SOURCES = MbtServerBase.cxx MbtJSONServerBase.cxx
