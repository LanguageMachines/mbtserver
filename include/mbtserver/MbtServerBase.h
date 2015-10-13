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
#ifndef MBTSERVER_H
#define MBTSERVER_H

#include "mbt/MbtAPI.h"
#include "ticcutils/LogStream.h"
#include "timblserver/FdStream.h"
#include "timblserver/ServerBase.h"
//#include "timblserver/SocketBasics.h"

namespace MbtServer {
  using namespace TimblServer;

  class MbtServerClass : public TcpServerBase {
    friend class TaggerClass;
    friend void *tagChild( void * );
  public:
    MbtServerClass( TiCC::Configuration *, TiCC::CL_Options& );
    virtual ~MbtServerClass();
    static std::string VersionInfo( bool );
    void callback( childArgs* );
    void RunServer();
    void createServers( TiCC::Configuration& );
  protected:
    std::map<std::string, TaggerClass *> experiments;
  };

  void StartServer( TiCC::CL_Options& );
}
#endif
