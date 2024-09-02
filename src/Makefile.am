AM_CPPFLAGS = -I@top_srcdir@/include
AM_CXXFLAGS = -std=c++17 -W -Wall -g -O3 -pedantic

LDADD = libmbtserver.la

bin_PROGRAMS = mbtserver

mbtserver_SOURCES = MbtServer.cxx

lib_LTLIBRARIES = libmbtserver.la
libmbtserver_la_LDFLAGS= -version-info 2:0:0

libmbtserver_la_SOURCES = MbtServerBase.cxx MbtJSONServerBase.cxx
