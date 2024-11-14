/*
  Copyright (c) 1998 - 2024
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
#include "mbt/Logging.h"
#include "mbt/Tagger.h"
#include "mbtserver/MbtServerBase.h"

#include <pthread.h>

using namespace std;
using namespace icu;
using namespace Timbl;
using namespace Tagger;
using namespace TiCC;

#define SLOG (*Log(theServer->logstream()))
#define SDBG (*Dbg(theServer->logstream()))

namespace MbtServer {

  void MbtServerClass::createServers( const TiCC::Configuration *c ){
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
      exp->setLog( logstream() );
      if ( exp->InitTagging() ){
	cerr << "Created a tagger " << key << endl;
	experiments[key] = exp;
      }
      else {
	cerr << "failed to created a tagger with name=" << key << endl;
	cerr << "and options = " << it.second << endl;
	delete exp;
      }
    }
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

  MbtServerClass::MbtServerClass( const TiCC::Configuration *c ):
    TcpServerBase( c, &experiments ) {
    logstream().set_message("MbtServer:");
    logstream().setstamp(StampMessage);
    if ( doDebug() ){
      logstream().setlevel( LogHeavy );
    }
    cerr << "mbtserver " << VERSION << endl;
    cerr << "based on " << Timbl::VersionName() << " and "
	 << TiCCServer::VersionName() << endl;

    createServers( c );
    if ( experiments.empty() ){
      cerr << "starting the server failed." << endl;
      usage();
      ServerBase::server_usage();
      exit( EXIT_FAILURE );
    }
  }

  MbtServerClass::~MbtServerClass(){
    for ( const auto& it : experiments ){
      delete it.second;
    }
  }

  inline void Split( const string& line, string& com, string& rest ){
    string::size_type b_it = line.find( '=' );
    if ( b_it != string::npos ){
      com = trim( line.substr( 0, b_it ) );
      rest = trim( line.substr( b_it+1 ) );
    }
    else {
      rest.clear();
      com = line;
    }
  }

  string strip_cr( string& line ){
    string::size_type r_pos = line.find('\r');
    if ( r_pos != string::npos ){
      line.erase(r_pos,1);
    }
    return line;
  }

  // ***** This is the routine that is executed from a new thread **********
  void MbtServerClass::callback( childArgs *args ){
    ServerBase *theServer = args->mother();
    args->os() << "Welcome to the Mbt server." << endl;
    if ( experiments.size() > 1 ){
      map<string,TaggerClass*>::const_iterator it = experiments.begin();
      bool first = true;
      while ( it != experiments.end() ){
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
    if ( getline( args->is(), Line ) ){
      SDBG << "FirstLine='" << Line << "'" << endl;
      Line = strip_cr( Line );
      SDBG << "Line='" << Line << "'" << endl;
      string command, param;
      Split( Line, command, param );
      string baseName = "default";
      if ( command == "base" ){
	baseName = param;
      }
      if ( experiments.find( baseName ) != experiments.end() ){
	exp = experiments[baseName]->clone( );
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
	if ( exp->enriched() ){
	  string text_block;
	  if ( command != "base" ){
	    SDBG << "input line '" << Line << "'" << endl;
	    text_block += Line + "\n";
	  }
	  while ( getline( args->is(), Line ) ){
	    Line = strip_cr( Line );
	    SDBG << "input line '" << Line << "'" << endl;
	    if ( Line == "<utt>" ){
	      break;
	    }
	    text_block += Line + "\n";
	  }
	  UnicodeString result;
	  SDBG << "call TagLine '" << text_block << "'" << endl;
	  int num = exp->TagLine( TiCC::UnicodeFromUTF8(text_block), result );
	  SDBG << "result     '" << result << "'" << endl;
	  if ( num > 0 ){
	    nw += num;
	    args->os() << result << endl;
	  }
	}
	else {
	  if ( command != "base" ){
	    SDBG << "input line '" << Line << "'" << endl;
	    UnicodeString result;
	    int num = exp->TagLine( TiCC::UnicodeFromUTF8(Line), result );
	    SDBG << "result     '" << result << "'" << endl;
	    if ( num > 0 ){
	      nw += num;
	      args->os() << result << endl;
	    }
	  }
	  while ( getline( args->is(), Line ) ){
	    Line = strip_cr( Line );
	    SDBG << "input line '" << Line << "'" << endl;
	    UnicodeString result;
	    int num = exp->TagLine( TiCC::UnicodeFromUTF8(Line), result );
	    SDBG << "result     '" << result << "'" << endl;
	    if ( num > 0 ){
	      nw += num;
	      args->os() << result << endl;
	    }
	    else
	      break;
	  }
	}
      }
    }
    SLOG << "Total: " << nw << " words processed " << endl;
    delete exp;
  }

  void StartServer( TiCC::CL_Options& opts ){
    if ( opts.is_present( "h" ) ||
	 opts.is_present( "help" ) ){
      usage();
      ServerBase::server_usage();
      exit( EXIT_SUCCESS );
    }
    if ( opts.is_present( "V" ) ||
	 opts.is_present( "version" ) ){
      exit( EXIT_SUCCESS );
    }
    bool verbose = opts.is_present( "debug" );
    Configuration *config = initServerConfig( opts );
    if ( config ){
      if ( verbose ){
	cout << "read config: " << config << endl;
      }
      if ( config->lookUp("protocol") == "json" ){
	LOG << "STARTING A JSON SERVER!" << endl;
	MbtJSONServerClass server( config );
	server.Run();
      }
      else {
	MbtServerClass server( config );
	server.Run();
      }
    }
    else {
      cerr << "starting failed" << endl;
      exit( EXIT_FAILURE );
    }
  }
}
