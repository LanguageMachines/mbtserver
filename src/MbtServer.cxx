/*
  Copyright (c) 1998 - 2020
  CLST  - Radboud University
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
      https://github.com/LanguageMachines/mbt/issues
  or send mail to:
      lamasoftware (at ) science.ru.nl

*/

#include <string>
#include <iostream>
#include "mbtserver/MbtServerBase.h"

using namespace std;

int main( int argc, const char *argv[]) {
  TiCC::CL_Options opts( Tagger::mbt_short_opts + TimblServer::serv_short_opts,
			 Tagger::mbt_long_opts + TimblServer::serv_long_opts );
  try {
    opts.init( argc, argv );
  }
  catch ( const exception& e ){
    cerr << e.what() << endl;
    exit(EXIT_FAILURE);
  }
  MbtServer::StartServer( opts );
  exit(EXIT_SUCCESS);
}
