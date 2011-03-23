/*
  $Id$
  $URL$
  Copyright (c) 1998 - 2011
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

#include <csignal>
#include <cerrno>
#include <string>
#include <cstdio> // for remove()
#include "config.h"
#include "timbl/TimblAPI.h"
#include "timblserver/TimblServerAPI.h"
#include "timblserver/FdStream.h"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"
#include "mbtserver/MbtServerBase.h"

#include <pthread.h>

using namespace std;
using namespace Timbl;

#define SLOG (*Log(theServer->cur_log))
#define SDBG (*Dbg(theServer->cur_log))

namespace Tagger {

  TaggerClass *createPimpl( Timbl::TimblOpts& opts ){
    TaggerClass *exp = new TaggerClass();
    exp->parse_run_args( opts );
    exp->set_default_filenames();
    if ( exp->InitTagging() )
      return exp;
    else {
      delete exp;
      return 0;
    }
  }
  
  bool MbtServer::getConfig( const string& serverConfigFile ){
    maxConn = 25;
    serverPort = -1;
    ifstream is( serverConfigFile.c_str() );
    if ( !is ){
      cerr << "problem reading " << serverConfigFile << endl;
      return false;
    }
    else {
      string line;
      while ( getline( is, line ) ){
	if ( line.empty() || line[0] == '#' )
	  continue;
	string::size_type ispos = line.find('=');
	if ( ispos == string::npos ){
	  cerr << "invalid entry in: " << serverConfigFile 
	       <<  " offending line: '" << line << "'" << endl;
	  return false;
	}
	else {
	  string base = line.substr(0,ispos);
	  string rest = line.substr( ispos+1 );
	  base = compress(base);
	  rest = compress(rest);
	  if ( !rest.empty() ){
	    string tmp = base;
	    lowercase(tmp);
	    if ( tmp == "maxconn" ){
	      if ( !stringTo( rest, maxConn ) ){
		cerr << "invalid value for maxconn" << endl;
		return false;
	      }
	    }
	    else if ( tmp == "port" ){
	      if ( !stringTo( rest, serverPort ) ){
		cerr << "invalid value for port" << endl;
		return false;
	      }
	    }
	    else {
	      string::size_type spos = 0;
	      if ( rest[0] == '"' )
		spos = 1;
	      string::size_type epos = rest.length()-1;
	      if ( rest[epos] == '"' ) {
		--epos;
	      }
	      serverConfig[base] = rest.substr( spos, epos-spos+1 );
	    }
	  }
	}
      }
      if ( serverPort < 0 ){
	cerr << "missing 'port=' entry in config file" << endl;
	return false;
      }
      else
	return true;
    }
  }


  void MbtServer::createServers(){
    map<string, string>::const_iterator it = serverConfig.begin();
    while ( it != serverConfig.end() ){
      TaggerClass *exp = new TaggerClass();
      TimblOpts opts(it->second);
      exp->parse_run_args( opts );
      exp->set_default_filenames();
      if ( exp->InitTagging() ){
	cerr << "Created server " << it->first << endl;
	experiments[it->first] = exp;
      }
      else {
	cerr << "failed to created a server with name=" << it->first << endl;
	cerr << "and options = " << it->second << endl;
	delete exp;
      }
      ++it;
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

  MbtServer::MbtServer( Timbl::TimblOpts& opts ): cur_log("MbtServer", 
							  StampMessage ){
    maxConn = 25;
    serverPort = -1;
    tcp_socket = 0;
    doDaemon = true;
    dbLevel = LogNormal;
    string val;
    bool mood;
    if ( opts.Find( "config", val, mood ) ){
      configFile = val;
      opts.Delete( "config" );
    }
    if ( opts.Find( "S", val, mood ) ) {
      if ( !configFile.empty() ){
	cerr << "-S option not allowed when --config is used" << endl;
	usage();
	exit(1);
      }
      serverPort = Timbl::stringTo<int>( val );
      if ( serverPort < 1 || serverPort > 32767 ){
	cerr << "-S option, portnumber invalid: " << serverPort << endl;
	usage();
	exit(1);
      }
      opts.Delete( "S" );
    }
    else if ( configFile.empty() ){
      cerr << "missing -S<port> option" << endl;
      usage();
      exit(1);
    }
    if ( opts.Find( "pidfile", val ) ) {
      pidFile = val;
      opts.Delete( "pidfile" );
    }
    if ( opts.Find( "logfile", val ) ) {
      logFile = val;
      opts.Delete( "logfile" );
    }
    if ( opts.Find( "daemonize", val ) ) {
      doDaemon = ( val != "no" && val != "NO" && val != "false" && val != "FALSE" );
      opts.Delete( "daemonize" );
    }
    if ( opts.Find( 'D', val, mood ) ){
      if ( val == "LogNormal" )
	cur_log.setlevel( LogNormal );
      else if ( val == "LogDebug" )
	cur_log.setlevel( LogDebug );
      else if ( val == "LogHeavy" )
	cur_log.setlevel( LogHeavy );
      else if ( val == "LogExtreme" )
	cur_log.setlevel( LogExtreme );
      else {
	cerr << "Unknown Debug mode! (-D " << val << ")" << endl;
      }
      opts.Delete( 'D' );
    }

    if ( !configFile.empty() ){
      getConfig( configFile );
      createServers();
    }
    else {
      TaggerClass *exp = createPimpl( opts );
      if ( exp )
	experiments["default"] = exp;
    }
    if ( experiments.empty() ){
      cerr << "starting the server failed." << endl;
      usage();
      exit(1);
    }
  } 
 
  MbtServer::~MbtServer(){
    map<string, TaggerClass *>::const_iterator it = experiments.begin();
    while ( it != experiments.end() ){
      delete it->second;
      ++it;
    }
  }

  struct childArgs{
    MbtServer *Mother;
    Sockets::ServerSocket *socket;
    int maxC;
    TaggerClass *experiment;
  };

  inline void Split( const string& line, string& com, string& rest ){
    string::size_type b_it = line.find( '=' );
    if ( b_it != string::npos ){
      com = compress( line.substr( 0, b_it ) );
      rest = compress( line.substr( b_it+1 ) );
    }
    else {
      rest.clear();
      com = line;
    }
  }  

  // ***** This is the routine that is executed from a new thread **********
  void *tagChild( void *arg ){
    childArgs *args = (childArgs *)arg;
    MbtServer *theServer = args->Mother;
    Sockets::Socket *Sock = args->socket;
    static int service_count = 0;
    static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
    //
    // use a mutex to update the global service counter
    //
    pthread_mutex_lock( &my_lock );
    service_count++;
    int nw = 0;
    if ( service_count > args->maxC ){
      Sock->write( "Maximum connections exceeded\n" );
      Sock->write( "try again later...\n" );
      pthread_mutex_unlock( &my_lock );
      cerr << "Thread " << pthread_self() << " refused " << endl;
    }
    else {
      // Greeting message for the client
      //
      pthread_mutex_unlock( &my_lock );
      time_t timebefore, timeafter;
      time( &timebefore );
      // report connection to the server terminal
      //
      SLOG << "Thread " << pthread_self() << ", Socket number = "
	  << Sock->getSockId() << ", started at: " 
	  << asctime( localtime( &timebefore) );
      fdistream is( Sock->getSockId() );
      fdostream os( Sock->getSockId() );
      os << "Welcome to the Mbt server." << endl;
      string baseName = "default";
      if ( theServer->experiments.size() > 1 ){
	map<string,TaggerClass*>::const_iterator it = theServer->experiments.begin();
	bool first = true;
	while ( it != theServer->experiments.end() ){
	  if ( it->first != "default" ){
	    if ( first ){
	      os << "available bases: ";
	      first = false;
	    }
	    os << it->first << " ";
	  }
	  ++it;
	}
	os << endl;
      }
      string Line;
      TaggerClass *exp = 0;
      if ( getline( is, Line ) ){
	SDBG << "FirstLine='" << Line << "'" << endl;
	string command, param;
	string::size_type pos = Line.find('\r');
	if ( pos != string::npos )
	  Line.erase(pos,1);
	SDBG << "Line='" << Line << "'" << endl;
	Split( Line, command, param );
	if ( command == "base" ){
	  baseName = param;
	}
	if ( theServer->experiments.find( baseName ) !=  theServer->experiments.end() ){
	  exp = theServer->experiments[baseName]->clone( );
	  exp->setLog( theServer->cur_log );
	  if ( baseName != "default" ){
	    os << "base set to '" << baseName << "'" << endl;
	    SLOG << "Set basename " << baseName << endl;
	  }
	}
	else {
	  os << "invalid basename '" << baseName << "'" << endl;
	  SLOG << "Invalid basename " << baseName << " rejected" << endl;
	}
	if ( exp ){
	  string result;
	  if ( command != "base" ){
	    SDBG << "input line '" << Line << "'" << endl;
	    int num = exp->TagLine( Line, result );
	    SDBG << "result     '" << result << "'" << endl;
	    if ( num > 0 ){
	      nw += num;
	      os << result << endl;
	    }
	  }
	  while ( getline( is, Line ) ){
	    string::size_type pos = Line.find('\r');
	    if ( pos != string::npos )
	      Line.erase(pos,1);
	    SDBG << "input line '" << Line << "'" << endl;
	    int num = exp->TagLine( Line, result );
	    SDBG << "result     '" << result << "'" << endl;
	    if ( num > 0 ){
	      nw += num;
	      os << result << endl;
	    }
	    else
	      break;
	  }
	}
      }
      time( &timeafter );
      SLOG << "Thread " << pthread_self() << ", terminated at: " 
	  << asctime( localtime( &timeafter ) );
      SLOG << "Total time used in this thread: " << timeafter - timebefore 
	   << " sec, " << nw << " words processed " << endl;
      delete exp;
    }
    // exit this thread
    //
    pthread_mutex_lock( &my_lock );
    service_count--;
    SLOG << "Socket total = " << service_count << endl;
    pthread_mutex_unlock( &my_lock );
    delete Sock;
    return 0;
  }

  void StopServerFun( int Signal ){
    if ( Signal == SIGINT ){
      exit(EXIT_FAILURE);
    }
    signal( SIGINT, StopServerFun );
  }  
  
  void MbtServer::RunServer(){
    cerr << "trying to start a Server on port: " << serverPort << endl
	 << "maximum # of simultaneous connections: " << maxConn
	 << endl;
    if ( !logFile.empty() ){
      ostream *tmp = new ofstream( logFile.c_str() );
      if ( tmp && tmp->good() ){
	cerr << "switching logging to file " << logFile << endl;
	cur_log.associate( *tmp );
	cur_log.message( "MbtServer:" );
	LOG << "Started logging " << endl;	
      }
      else {
	cerr << "unable to create logfile: " << logFile << endl;
	cerr << "not started" << endl;
	exit(EXIT_FAILURE);
      }
    }
    int start;
    if ( logFile.empty() )
      start = daemonize( 1, 1 );
    else
      start = daemonize( 0, 0 );
    if ( start < 0 ){
      LOG << "Failed to daemonize error= " << strerror(errno) << endl;
      exit(EXIT_FAILURE);
    };
    if ( !pidFile.empty() ){
      // we have a liftoff!
      // signal it to the world
      remove( pidFile.c_str() ) ;
      ofstream pid_file( pidFile.c_str() ) ;
      if ( !pid_file ){
	LOG << "Unable to create pidfile:"<< pidFile << endl;
	LOG << "TimblServer NOT Started" << endl;
	exit(EXIT_FAILURE);
      }
      else {
	pid_t pid = getpid();
	pid_file << pid << endl;
      }
    }
    
      // set the attributes
    pthread_attr_t attr;
    if ( pthread_attr_init(&attr) ||
	 pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ) ){
      LOG << "Threads: couldn't set attributes" << endl;
      exit(EXIT_FAILURE);
    }
    //
    // setup Signal handling to abort the server.
    signal( SIGINT, StopServerFun );
    
    pthread_t chld_thr;
    
    // start up server
    // 
    LOG << "Started Server on port: " << serverPort << endl
	<< "Maximum # of simultaneous connections: " << maxConn
	<< endl;
    
    Sockets::ServerSocket server;
    string portString = toString<int>(serverPort);
    if ( !server.connect( portString ) ){
      LOG << "Failed to start Server: " << server.getMessage() << endl;
      exit(EXIT_FAILURE);
    }
    if ( !server.listen( maxConn ) < 0 ){
      LOG << "Server: listen failed " << strerror( errno ) << endl;
      exit(EXIT_FAILURE);
    };
      
    while(true){ // waiting for connections loop
      Sockets::ServerSocket *newSock = new Sockets::ServerSocket();
      if ( !server.accept( *newSock ) ){
	if( errno == EINTR )
	  continue;
	else {
	  LOG << "Server: Accept Error: " << server.getMessage() << endl;
	  exit(EXIT_FAILURE);
	}
      }
      LOG << "Accepting Connection " << newSock->getSockId()
	  << " from remote host: " << newSock->getClientName() << endl;
      
      // create a new thread to process the incoming request 
      // (The thread will terminate itself when done processing
      // and release its socket handle)
      //
      childArgs *args = new childArgs();
      args->Mother = this;
      args->maxC = maxConn;
      args->socket = newSock;
      pthread_create( &chld_thr, &attr, tagChild, (void *)args );
      // the server is now free to accept another socket request 
    }
  }
  
  void StartServer( TimblOpts& Opts ){
    MbtServer server( Opts );
    server.RunServer();
  }

}
