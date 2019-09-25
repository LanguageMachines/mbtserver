/*
  Copyright (c) 1998 - 2019
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

#include <csignal>
#include <cerrno>
#include <string>
#include <cstdio> // for remove()
#include "config.h"
#include "ticcutils/ServerBase.h"
#include "ticcutils/PrettyPrint.h"
#include "ticcutils/json.hpp"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"
#include "mbtserver/MbtServerBase.h"
#include <pthread.h>

using namespace std;
using namespace Timbl;
using namespace Tagger;
using namespace TiCC;
using namespace nlohmann;

using TiCC::operator<<;

#define SLOG (*Log(theServer->logstream()))
#define SDBG (*Dbg(theServer->logstream()))

namespace MbtServer {

  bool read_json( istream& is, json& the_json ){
    the_json.clear();
    string json_line;
    if ( getline( is, json_line ) ){
      try {
	the_json = json::parse( json_line );
      }
      catch ( const exception& e ){
	return false;
      }
      return true;
    }
    return false;
  }

  // ***** This is the routine that is executed from a new thread **********
  void MbtJSONServerClass::callback( childArgs *args ){
    MbtJSONServerClass *theServer = dynamic_cast<MbtJSONServerClass*>( args->mother() );
    int sockId = args->id();
    TaggerClass *exp = 0;
    map<string, TaggerClass*> experiments =
      *(static_cast<map<string, TaggerClass*> *>(callback_data()));
    json out_json;
    out_json["status"] = "ok";
    string baseName = "default";
    int nw = 0;
    if ( experiments.size() == 1 ){
      exp = experiments[baseName]->clone();
    }
    else {
      json arr = json::array();
      for ( const auto& it : experiments ){
	arr.push_back( it.first );
      }
      out_json["available_bases"] = arr;
    }
    if ( doDebug() ){
      SLOG << "send JSON: " << out_json.dump(2) << endl;
    }
    args->os() << out_json << endl;

    json in_json;
    bool go_on = true;
    while ( go_on && read_json( args->is(), in_json ) ){
      if ( in_json.empty() ){
	continue;
      }
      if ( doDebug() ){
	SLOG << "handling JSON" << in_json.dump(2) << endl;
      }
      string command;
      string param;
      if ( in_json.find( "command" ) != in_json.end() ){
	command = in_json["command"];
      }
      if ( command.empty() ){
	DBG << sockId << " Don't understand '" << in_json << "'" << endl;
	json out_json;
	out_json["error"] = "Illegal instruction:'" + in_json.dump() + "'";
	if ( doDebug() ){
	  SLOG << "send JSON: " << out_json.dump(2) << endl;
	}
	args->os() << out_json << endl;
      }
      if ( command == "base" ){
	if ( in_json.find("param") != in_json.end() ){
	  param = in_json["param"];
	}
	if ( param.empty() ){
	  json out_json;
	  out_json["error"] = "missing param for 'base' instruction:'";
	  if ( doDebug() ){
	    SLOG << "send JSON: " << out_json.dump(2) << endl;
	  }
	  args->os() << out_json << endl;
	}
	else {
	  baseName = in_json["param"];
	  if ( experiments.find( baseName ) != experiments.end() ){
	    exp = experiments[baseName]->clone( );
	    json out_json;
	    out_json["base"] = baseName;
	    args->os() << out_json << endl;
	    if ( doDebug() ){
	      SLOG << "Set basename " << baseName << endl;
	    }
	  }
	  else {
	    json out_json;
	    out_json["error"] = "unknown base '" + baseName + "'";
	    if ( doDebug() ){
	      SLOG << "send JSON: " << out_json.dump(2) << endl;
	    }
	    args->os() << out_json << endl;
	  }
	}
      }
      else if ( command == "tag" ){
	if ( !exp ){
	  json out_json;
	  out_json["error"] = "no base selected yet";
	  if ( doDebug() ){
	    SLOG << "send JSON: " << out_json.dump(2) << endl;
	  }
	  args->os() << out_json << endl;
	}
	else {
	  if ( doDebug() ){
	    SLOG << "handle json request '" << in_json << "'" << endl;
	  }
	  json out_json = exp->tag_JSON_to_JSON( in_json["sentence"] );
	  int num = out_json.size();
	  if ( doDebug() ){
	    SLOG << "ALIVE, got " << num << " tags" << endl;
	  }
	  if ( num > 0 ){
	    nw += num;
	    SDBG << "voor WRiTE json! " << out_json << endl;
	    args->os() << out_json << endl;
	    SDBG << "WROTE json!" << endl;
	  }
	  else {
	    SDBG << "NO RESULT FOR TagJSON (" << in_json << ")" << endl;
	    json out_json;
	    out_json["error"] = "tagger failed";
	    args->os() << out_json << endl;
	  }
	}
      }
    }
    SLOG << "Total: " << nw << " words processed " << endl;
    delete exp;
  }

}
