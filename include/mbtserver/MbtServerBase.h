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
#ifndef MBTSERVER_H
#define MBTSERVER_H

#include "mbt/MbtAPI.h"
#include "ticcutils/ServerBase.h"
#include "ticcutils/json.hpp"

namespace MbtServer {
  using namespace TimblServer;

  class MbtServerClass : public TcpServerBase {
    friend class Tagger::TaggerClass;
  public:
    explicit MbtServerClass( const TiCC::Configuration * );
    virtual ~MbtServerClass();
    static std::string VersionInfo( bool );
  private:
    void callback( childArgs* );
    void createServers( const TiCC::Configuration * );
    std::map<std::string, Tagger::TaggerClass *> experiments;
  };

  class MbtJSONServerClass : public MbtServerClass {
    friend class Tagger::TaggerClass;
  public:
    explicit MbtJSONServerClass( const TiCC::Configuration *c ):
    MbtServerClass(c)
    {};
  private:
    void callback( childArgs* );
  };

  void StartServer( TiCC::CL_Options& );
  void StartJSONServer( TiCC::CL_Options& );
  nlohmann::json TR_to_json( const Tagger::TaggerClass *,
			     const std::vector<Tagger::TagResult>& );
  std::vector<Tagger::TagResult> json_to_TR( const nlohmann::json& );

}
#endif
