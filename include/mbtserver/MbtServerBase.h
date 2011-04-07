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
#ifndef MBTSERVER_H
#define MBTSERVER_H

#include "timbl/TimblAPI.h"
#include "mbt/MbtAPI.h"
#include "timbl/LogStream.h"
#include "timblserver/SocketBasics.h"

namespace MbtServer {

  class MbtServerClass {
    friend class TaggerClass;
    friend void *tagChild( void * );
  public:
    LogStream cur_log;
    virtual ~MbtServerClass();
    static std::string VersionInfo( bool );
    MbtServerClass( Timbl::TimblOpts& );
    void RunServer();
    void createServers();
  protected:
    bool getConfig( const std::string& );
    Sockets::ServerSocket *TcpSocket() const { return tcp_socket; };
    std::map<std::string, TaggerClass *> experiments;
    std::string logFile;
    std::string pidFile;
    bool doDaemon;
  private:
    int maxConn;
    int serverPort;
    LogLevel dbLevel;
    Sockets::ServerSocket *tcp_socket;
    std::string configFile;
    std::map<std::string, std::string> serverConfig;
  };

  void StartServer( Timbl::TimblOpts& );
}
#endif
