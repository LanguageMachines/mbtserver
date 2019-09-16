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

#define SLOG (*Log(theServer->myLog))
#define SDBG (*Dbg(theServer->myLog))

namespace MbtServer {

  json TR_to_json( const TaggerClass *context,
			     const vector<TagResult>& trs ){
    json result = json::array();
    for ( const auto& tr : trs ){
      // lookup the assigned category
      json one_entry;
      one_entry["word"] = tr.word();
      one_entry["known"] = tr.is_known();
      if ( context->enriched() ){
	one_entry["enrichment"] = tr.enrichment();
      }
      one_entry["tag"] = tr.assigned_tag();
      if ( context->confidence_is_set() ){
	one_entry["confidence"] = tr.confidence();
      }
      if ( context->distrib_is_set() ){
	one_entry["distribution"] = tr.distribution();
      }
      if ( context->distance_is_set() ){
	one_entry["distance"] = tr.distance();
      }
      result.push_back( one_entry );
    } // end of output loop through one sentence
    return result;
  }

  vector<TagResult> json_to_TR( const json& in ){
    //    cerr << "json_to_TR( " << in  << ")" << endl;
    vector<TagResult> result;
    for ( auto& i : in ){
      //      cerr << "looping json_to_TR( " << i << ")" << endl;
      TagResult tr;
      tr.set_word( i["word"] );
      if ( i.find("known") != i.end() ){
	tr.set_known( i["known"] == "true" );
      }
      tr.set_tag( i["tag"] );
      if ( i.find("confidence") != i.end() ){
	tr.set_confidence( i["confidence"] );;
      }
      if ( i.find("distance") != i.end() ){
	tr.set_distance( i["distance"] );
      }
      if ( i.find("distribution") != i.end() ){
	tr.set_distribution( i["distribution"] );
      }
      if ( i.find("enrichment") != i.end() ){
	tr.set_enrichment( i["enrichment"] );
      }
      result.push_back( tr );
    }
    return result;
  }


  string extract_text( json& my_json ){
    string result;
    if ( my_json.is_array() ){
      for ( const auto& it : my_json ){
	string tmp = it["word"];
	result += tmp + " ";
	if ( it.find("enrichment") != it.end() ){
	  tmp = it["enrichment"];
	  result += tmp + " ";
	  if ( it.find("tag") != it.end() ){
	    tmp = it["tag"];
	    result += tmp;
	  }
	  else {
	    result += "??";
	  }
	  result += "\n";
	}
      }
    }
    else {
      string tmp = my_json["word"];
      result += tmp +  " ";
      if ( my_json.find("enrichment") != my_json.end() ){
	tmp = my_json["enrichment"];
	result += tmp + " ";
	if ( my_json.find("tag") != my_json.end() ){
	  tmp = my_json["tag"];
	  result += tmp;
	}
	else {
	  result += "??";
	}
	result += "\n";
      }
    }
    return result;
  }

  bool MbtJSONServerClass::read_json( istream& is,
				      json& the_json ){
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
    map<string, TaggerClass*> *experiments =
      static_cast<map<string, TaggerClass*> *>(callback_data);
    json out_json;
    out_json["status"] = "ok";
    string baseName = "default";
    int nw = 0;
    if ( experiments->size() == 1 ){
      exp = (*experiments)[baseName]->clone();
    }
    else {
      json arr = json::array();
      for ( const auto& it : *experiments ){
	arr.push_back( it.first );
      }
      out_json["available_bases"] = arr;
    }
    cerr << "send JSON: " << out_json.dump(2) << endl;
    args->os() << out_json << endl;

    json in_json;
    bool go_on = true;
    while ( go_on && theServer->read_json( args->is(), in_json ) ){
      if ( in_json.empty() ){
	continue;
      }
      cerr<< "handling JSON" << in_json.dump(2) << endl;
      string command;
      string param;
      vector<string> params;
      if ( in_json.find( "command" ) != in_json.end() ){
	command = in_json["command"];
      }
      if ( command.empty() ){
	DBG << sockId << " Don't understand '" << in_json << "'" << endl;
	json out_json;
	out_json["error"] = "Illegal instruction:'" + in_json.dump() + "'";
	cerr << "send JSON: " << out_json.dump(2) << endl;
	args->os() << out_json << endl;
      }
      if ( command == "base" ){
	if ( in_json.find("param") != in_json.end() ){
	  param = in_json["param"];
	}
	if ( param.empty() ){
	  json out_json;
	  out_json["error"] = "missing param for 'base' instruction:'";
	  cerr << "send JSON: " << out_json.dump(2) << endl;
	  args->os() << out_json << endl;
	}
	else {
	  baseName = in_json["param"];
	  if ( experiments->find( baseName ) != experiments->end() ){
	    exp = (*experiments)[baseName]->clone( );
	    json out_json;
	    out_json["base"] = baseName;
	    args->os() << out_json << endl;
	    cerr << "Set basename " << baseName << endl;
	  }
	  else {
	    json out_json;
	    out_json["error"] = "unknown base '" + baseName + "'";
	    cerr << "send JSON: " << out_json.dump(2) << endl;
	    args->os() << out_json << endl;
	  }
	}
      }
      else if ( command == "tag" ){
	if ( !exp ){
	  json out_json;
	  out_json["error"] = "no base selected yet";
	  cerr << "send JSON: " << out_json.dump(2) << endl;
	  args->os() << out_json << endl;
	}
	else {
	  cerr << "handle json request '" << in_json << "'" << endl;
	  string text = extract_text( in_json["sentence"] );
	  cerr << "TagLine (" << text << ")" << endl;
	  vector<TagResult> v = exp->tagLine( text );
	  int num = v.size();
	  cerr << "ALIVE, got " << num << " tags" << endl;
	  if ( num > 0 ){
	    nw += num;
	    json got_json = TR_to_json( exp, v );
	    SDBG << "voor WRiTE json! " << got_json << endl;
	    args->os() << got_json << endl;
	    SDBG << "WROTE json!" << endl;
	  }
	  else {
	    SDBG << "NO RESULT FOR TagLine (" << text << ")" << endl;
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
