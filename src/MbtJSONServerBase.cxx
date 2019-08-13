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
#include "json/json.hpp"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"
#include "mbtserver/MbtServerBase.h"
#include <pthread.h>

using namespace std;
using namespace Timbl;
using namespace Tagger;
using namespace TiCC;
using TiCC::operator<<;

#define SLOG (*Log(theServer->myLog))
#define SDBG (*Dbg(theServer->myLog))

namespace MbtServer {

  string extract_text( nlohmann::json& my_json ){
    string result;
    if ( my_json.is_array() ){
      for ( const auto& it : my_json ){
	result += it["word"];
	result += " ";
	if ( it.find("enrichment") != it.end() ){
	  result += it["enrichment"];
	  result += " ";
	  if ( it.find("tag") != it.end() ){
	    result += it["tag"];
	  }
	  else {
	    result += "??";
	  }
	  result += "\n";
	}
      }
    }
    else {
      result += my_json["word"];
      result += " ";
      if ( my_json.find("enrichment") != my_json.end() ){
	result += my_json["enrichment"];
	result += " ";
	if ( my_json.find("tag") != my_json.end() ){
	  result += my_json["tag"];
	}
	else {
	  result += "??";
	}
	result += "\n";
      }
    }
    return result;
  }

  bool MbtJSONServerClass::read_json( ServerBase* theServer,
				      istream& is,
				      nlohmann::json& the_json ){
    the_json.clear();
    string json_line;
    if ( getline( is, json_line ) ){
      SDBG << "READ json_line='" << json_line << "'" << endl;
      try {
	the_json = nlohmann::json::parse( json_line );
      }
      catch ( const exception& e ){
	SLOG << "json parsing failed on '" << json_line + "':"
	     << e.what() << endl;
	return false;
      }
      return true;
    }
    return false;
  }

  // ***** This is the routine that is executed from a new thread **********
  void MbtJSONServerClass::callback( childArgs *args ){
    ServerBase *theServer = args->mother();
    map<string, TaggerClass*> *experiments =
      static_cast<map<string, TaggerClass*> *>(callback_data);

    args->os() << "Welcome to the Mbt server." << endl;
    string baseName = "default";
    if ( experiments->size() > 1 ){
      map<string,TaggerClass*>::const_iterator it = experiments->begin();
      bool first = true;
      while ( it != experiments->end() ){
	if ( it->first != "default" ){
	  if ( first ){
	    args->os() << "available bases: ";
	    first = false;
	  }
	  args->os() << it->first << " ";
	}
	++it;
      }
      args->os() << endl;
    }
    int nw = 0;
    string Line;
    TaggerClass *exp = 0;
    SDBG << "Start READING json" << endl;
    nlohmann::json my_json;
    bool skip=false;
    if ( read_json( theServer, args->is(), my_json ) ){
      if ( my_json.find( "base" ) != my_json.end() ){
	baseName = my_json["base"];
	SDBG << "Command is: base='" << baseName << "'" << endl;
	skip = true;
      }
      if ( experiments->find( baseName ) != experiments->end() ){
	exp = (*experiments)[baseName]->clone( );
	if ( baseName != "default" ){
	  args->os() << "base set to '" << baseName << "'" << endl;
	  SLOG << "Set basename " << baseName << endl;
	}
      }
      else {
	args->os() << "invalid basename '" << baseName << "'" << endl;
	SLOG << "Invalid basename " << baseName << " rejected" << endl;
      }
      if ( exp ){
	if ( skip ){
	  if ( !read_json( theServer, args->is(), my_json ) ){
	    SLOG << "reading json failed" << endl;
	    abort();
	  }
	}
	SDBG << "handle json request '" << my_json << "'" << endl;
	string text = extract_text( my_json );
	string result;
	SDBG << "TagLine (" << text << ")" << endl;
	vector<TagResult> v = exp->tagLine( text );
	int num = v.size();
	SDBG << "ALIVE, got " << num << " tags" << endl;
	if ( num > 0 ){
	  nw += num;
	  nlohmann::json got_json = exp->TR_to_json( v );
	  SDBG << "voor WRiTE json! " << got_json << endl;
	  args->os() << got_json << endl;
	  SDBG << "WROTE json!" << endl;
	}
	else {
	  SDBG << "NO RESULT FOR TagLine (" << text << ")" << endl;
	}
	while ( read_json( theServer, args->is(), my_json ) ){
	  SDBG << "handle next json request '" << my_json << "'" << endl;
	  string text = extract_text( my_json );
	  string result;
	  SDBG << "TagLine (" << text << ")" << endl;
	  vector<TagResult> v = exp->tagLine( text );
	  int num = v.size();
	  if ( num > 0 ){
	    nw += num;
	    nlohmann::json got_json = exp->TR_to_json( v );
	    SDBG << "voor WRiTE json!: " << got_json << endl;
	    args->os() << got_json << endl;
	    SDBG << "WROTE json!" << endl;
	  }
	}
      }
    }
    else {
      SLOG << "reading json failed!" << endl;
    }
    SLOG << "Total: " << nw << " words processed " << endl;
    delete exp;
  }

  inline void usage(){
    cerr << "usage:  mbtserver --config=config-file"
	 << endl;
    cerr << "or      mbtserver -s settings-file -S port"
	 << endl;
    cerr << "or      mbtserver {MbtOptions} -S port"
	 << endl;
    cerr << "see 'mbt -h' for all MbtOptions"
	 << endl;
    cerr << endl;
  }

  void StartJSONServer( TiCC::CL_Options& opts ){
    if ( opts.is_present( "h" ) ||
	 opts.is_present( "help" ) ){
      usage();
      exit( EXIT_SUCCESS );
    }
    if ( opts.is_present( "V" ) ||
	 opts.is_present( "version" ) ){
      exit( EXIT_SUCCESS );
    }
    Configuration *config = initServerConfig( opts );
    LOG << "STARTING A JSON SERVER!" << endl;
    MbtJSONServerClass server( config );
    server.Run();
  }

}
