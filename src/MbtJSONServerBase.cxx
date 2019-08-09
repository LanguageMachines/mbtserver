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
#include "json/json.hpp"
#include <pthread.h>

using namespace std;
using namespace Timbl;
using namespace Tagger;
using namespace TiCC;
using TiCC::operator<<;

#define SLOG (*Log(theServer->myLog))
#define SDBG (*Dbg(theServer->myLog))

namespace MbtServer {

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

  MbtJSONServerClass::MbtJSONServerClass( const TiCC::Configuration *c ):
    TcpServerBase( c ) {
    myLog.message("MbtServer:");
    myLog.setstamp(StampMessage);
    if ( doDebug() )
      myLog.setlevel( LogHeavy );
    cerr << "mbtserver " << VERSION << endl;
    cerr << "based on " << Timbl::VersionName() << " and "
	 << TimblServer::VersionName() << endl;
    createServers( c );
    if ( experiments.empty() ){
      cerr << "starting the server failed." << endl;
      usage();
      exit( EXIT_FAILURE );
    }
    callback_data = &experiments;
  }

  MbtJSONServerClass::~MbtJSONServerClass(){
    for ( const auto& it : experiments ){
      delete it.second;
    }
    delete config;
  }

  void MbtJSONServerClass::createServers( const TiCC::Configuration *c ){
    map<string,string> allvals;
    if ( c->hasSection("experiments") )
      allvals = c->lookUpAll("experiments");
    else {
      allvals = c->lookUpAll("global");
      // old style, everything is global
      // remove all already processed stuff
      auto it = allvals.begin();
      while ( it != allvals.end() ){
	if ( it->first == "port" ||
	     it->first == "protocol" ||
	     it->first == "configDir" ||
	     it->first == "logfile" ||
	     it->first == "debug" ||
	     it->first == "pidfile" ||
	     it->first == "daemonize" ||
	     it->first == "maxconn" ){
	  it = allvals.erase(it);
	}
	else {
	  ++it;
	}
      }
    }
    for( const auto& it : allvals ){
      string key = trim(it.first);
      TaggerClass *exp = new TaggerClass();
      TiCC::CL_Options opts( mbt_short_opts, mbt_long_opts );
      opts.init( it.second );
      exp->parse_run_args( opts, true );
      exp->set_default_filenames();
      exp->setLog( myLog );
      if ( exp->InitTagging() ){
	cerr << "Created server " << key << endl;
	experiments[key] = exp;
      }
      else {
	cerr << "failed to created a server with name=" << key << endl;
	cerr << "and options = " << it.second << endl;
	delete exp;
      }
    }
  }

  string extract_text( nlohmann::json& my_json ){
    string result;
    if ( my_json.is_array() ){
      for ( const auto& it : my_json ){
	result += it["word"];
	result += " ";
	if ( it.find("enriched") != it.end() ){
	  result += it["enriched"];
	  result += " ";
	  result += it["tag"];
	}
	result += "\n";
      }
    }
    else {
      result += my_json["word"];
      result += " ";
      if ( my_json.find("enriched") != my_json.end() ){
	result += my_json["enriched"];
	result += " ";
	result += my_json["tag"];
      }
      result += "\n";
    }
    return result;
  }

  nlohmann::json MbtJSONServerClass::to_json( const string& line,
					      bool enriched ) {
    if ( enriched ){
      LOG << "handle ENRICHED: " << line << endl;
      vector<string> lines = TiCC::split_at_first_of( line, "\n\r" );
      LOG << "got " << lines.size() << " lines" << endl;
      nlohmann::json result = nlohmann::json::array();
      for ( const auto& l : lines ){
	LOG << "handle line: " << l << endl;
	vector<string> parts;
	TiCC::split_at( l, parts, "/", true );
	LOG << "split into " << parts.size() << " parts " << endl;
	if ( parts.size() == 1 ){
	  break;
	}
	nlohmann::json entry;
	entry["word"] = parts[0];
	vector<string> word_tags;
	if ( parts.size() == 2 ){
	  word_tags = TiCC::split_at( parts[1], "??" );
	  entry["known"] = "yes";
	}
	else {
	  word_tags = TiCC::split_at( parts[2], "??" );
	}
	LOG << "word-tags=" << word_tags << endl;
	vector<string> res = TiCC::split( word_tags[1] );
	entry["tag"] = res[0];
	vector<string> conf = TiCC::split_at_first_of( res[1], "[]" );
	entry["confidence"] = conf[0];
	result.push_back( entry );
      }
      return result;
    }
    else {
      LOG << "handle Normal: " << line << endl;
      nlohmann::json result = nlohmann::json::array();
      vector<string> parts = TiCC::split( line );
      for ( const auto& p : parts ){
	vector<string> word_tags;
	TiCC::split_at( p, word_tags, "/", true );
	LOG << "word-tags=" << word_tags << endl;
	if ( word_tags.size() == 1 ){
	  break;
	}
	if ( word_tags.size() == 2 ){
	  nlohmann::json entry;
	  entry["word"] = word_tags[0];
	  entry["tag"] = word_tags[1];
	  entry["known"] = "yes";
	  result.push_back( entry );
	  //	  LOG << "created: " << entry << endl;
	}
	else if ( word_tags.size() == 3 ){
	  nlohmann::json entry;
	  entry["word"] = word_tags[0];
	  entry["tag"] = word_tags[2];
	  entry["known"] = "no";
	  //	  LOG << "created: " << entry << endl;
	  result.push_back( entry );
	}
	else if ( word_tags.size() == 4 ){
	  nlohmann::json entry;
	  entry["word"] = word_tags[0];
	  entry["tag"] = word_tags[2];
	  entry["confidence"] = word_tags[3];
	  //	  LOG << "created: " << entry << endl;
	  result.push_back( entry );
	}
      }
      LOG << "created: " << result << endl;
      LOG << endl << endl << "returnning now" << endl;
      return result;
    }
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
    args->is() >> my_json;
    SDBG << "READ json='" << my_json << "'" << endl;
    bool skip=false;
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
      if ( !skip ){
	SDBG << "handle json request '" << my_json << "'" << endl;
	string text = extract_text( my_json );
	string result;
	SDBG << "TagLine (" << text << ")" << endl;
	int num = exp->TagLine( text, result );
	SDBG << "num=" << num << " tagged result = (" << result << ")" << endl;
	if ( num > 0 ){
	  nw += num;
	  nlohmann::json got_json = to_json( result , exp->enriched() );
	  SDBG << "voor WRiTE json! " << got_json << endl;
	  args->os() << got_json << endl;
	  SDBG << "WROTE json!" << endl;
	}
      }
      while ( args->is().good() && args->is() >> my_json ){
	SDBG << "handle json request '" << my_json << "'" << endl;
	string text = extract_text( my_json );
	string result;
	SDBG << "TagLine (" << text << ")" << endl;
	int num = exp->TagLine( text, result );
	SDBG << "NAM=" << num << " tagged result = (" << result << ")" << endl;
	if ( num > 0 ){
	  nw += num;
	  nlohmann::json got_json = to_json( result, exp->enriched() );
	  SDBG << "voor WRiTE json!: " << got_json << endl;
	  args->os() << got_json << endl;
	  SDBG << "WROTE json!" << endl;
	}
      }
    }
    SLOG << "Total: " << nw << " words processed " << endl;
    delete exp;
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
