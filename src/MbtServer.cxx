/*
  $Id$
  $URL$
  Copyright (c) 1998 - 2015
  ILK   - Tilburg University
  CLiPS - University of Antwerp

  This file is part of mbtserver

  mbtserver is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  mbtserver is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/

#include <string>
#include <iostream>
#include "mbtserver/MbtServerBase.h"

int main( int argc, const char *argv[]) {
  TiCC::CL_Options opts( mbt_short_opts + "S:",
			 mbt_long_opts + ",config:,logfile:,pidfile:,daemonize:" );
  opts.init( argc, argv );
  MbtServer::StartServer( opts );
  exit(EXIT_SUCCESS);
}
