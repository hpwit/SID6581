
#include "Deepsid.hpp"



namespace Deepsid
{
  using namespace net;
  using namespace utils;

  using goblib::json::ElementPath;
  using goblib::json::ElementValue;


  /********************** DirItem **********************/


  std::string DirItem::name()
  {
    if( type == TYPE_FOLDER ) {
      return base_name( attr["foldername"] );
    } else if( type == TYPE_FILE ) {
      return base_name( attr["filename"] );
    }
    return "";
  }


  std::string DirItem::path( bool rewrite )
  {
    std::string ret = "";
    // bool is_root_child = rewrite
    //     ?  dir ? dir->name() == "/" : false
    //     : false
    // ;
    if( type == TYPE_FOLDER ) {
      if( attr["rf_path"] != "" ) { // real path provided
        ret = attr["rf_path"];
      } else if (attr["foldername"].find('/') != std::string::npos) { // slash in foldername
        ret = attr["foldername"];
      } else {
        ret = dir ? pathConcat( dir->name(), attr["foldername"] ) : attr["foldername"]; // chained with parent
      }
    } else if( type == TYPE_FILE ) {
      if (attr["filename"].find('/') != std::string::npos) { // path provided
        ret = attr["filename"];
      } else {
        ret = dir ? pathConcat( dir->name(), attr["filename"] ) : attr["filename"]; // chained with parent
      }
    }

    //pathConcat( deepsid_dir->sidFolder(), item.path( true ) )

    return rewrite ? pathRewrite( ret.c_str() ) : ret.c_str();
  }

  std::string DirItem::url()
  {
    std::string url = deepSID.config().api_url;

    if( type == TYPE_FOLDER ) {
      url += deepSID.config().api_endpoint;
      std::string folderpath = "";
      std::string searchargs = "";
      if (attr["ss_type"]!="" || attr["ss_query"]!="" ) { // search terms provided
        folderpath = dir ? dir->name() : folderpath; // search query uses parent folder
        searchargs = "&searchType=" + urlencode(attr["ss_type"]) + "&searchQuery=" + urlencode(attr["ss_query"]);
      } else if( attr["rf_path"] != "" ) { // real path provided
        folderpath = attr["rf_path"];
      } else if (attr["foldername"].find('/') != std::string::npos) { // slash in foldername
        folderpath = attr["foldername"];
      } else {
        folderpath = dir ? pathConcat( dir->name(), attr["foldername"] ) : attr["foldername"]; // root or relative
      }
      url += "?folder=" + urlencode( folderpath ) + searchargs;
      return url;

    } else if( type == TYPE_FILE ) {

      url = pathConcat(url, deepSID.config().api_hvsc_dir );

      if (attr["filename"].find('/') != std::string::npos) { // path provided
        url = pathConcat( url, pathencode( attr["filename"] ) );
      } else {
        url += (dir ? pathencode( pathConcat( dir->name(), attr["filename"] ) ) : attr["filename"] ); // chained with parent
      }
      return url;
    }
    return "";
  }


  bool DirItem::getSIDMeta( DirItem &item, SID_Meta_t* meta )
  {
    if( item.type != TYPE_FILE ) return false;
    if( meta == nullptr ) {
      // calloc
      meta = (SID_Meta_t *)sid_calloc(1,sizeof(SID_Meta_t));
      if( meta == NULL ) {
        log_e("Unable to alloc %d bytes, aborting", sizeof(SID_Meta_t) );
        return false;
      }
    }

    if( ! deepSID.config().fs->exists( item.path().c_str() ) ) {
      // retrieve the file if it does not exist
      if( ! downloadFile( item.dir->apiUrl(), deepSID.config().fs, item.path().c_str() ) ) {
        return false;
      }
    }

    snprintf( meta->filename, 255, "%s", item.attr["filename"].c_str() );
    snprintf( (char*)meta->name,      32, "%s", item.attr["name"].c_str() );
    snprintf( (char*)meta->author,    32, "%s", item.attr["author"].c_str() );
    //snprintf( meta->md5,       32, "%s", item.attr["md5"].c_str() );
    snprintf( (char*)meta->published, 32, "%s", item.attr["copyright"].c_str() );
    meta->subsongs  = atoi( item.attr["subtunes"].c_str() );
    meta->startsong = atoi( item.attr["startsubtune"].c_str() );
    String SIDDuration = String( item.attr["lengths"].c_str() );
    setDurations( SIDDuration, meta );

    return true;
  }


  /********************** JsonFragmentIndexIterator **********************/


  size_t JsonFragmentIndexIterator::dirs_count()
  {
    return fragment->values["folders"].size();
  }

  size_t JsonFragmentIndexIterator::files_count()
  {
    return fragment->values["files"].size();
  }

  size_t JsonFragmentIndexIterator::size()
  {
    return dirs_count() + files_count();
  }

  DirItemType_t JsonFragmentIndexIterator::nextOffset( size_t *next_offset )
  {
    size_t dSize = fragment->values["folders"].size();
    size_t fSize = fragment->values["files"].size();
    if( position >= dSize + fSize ) {
      // is none
      return TYPE_NONE;
    }
    if( position<dSize ) {
      // is folder
      *next_offset = fragment->values["folders"][position];
      position++;
      return TYPE_FOLDER;
    }
    // is file
    *next_offset = fragment->values["files"][ position - dSize ];
    position++;
    return TYPE_FILE;
  }



  /********************** JsonIndexHandler **********************/


  void JsonIndexHandler::startDocument() { /*log_d("Document start");*/ }
  void JsonIndexHandler::endDocument() { /*log_d("Document end");*/ fragment_ended = true; }
  void JsonIndexHandler::whitespace(const char c) { }
  void JsonIndexHandler::endObject(const ElementPath& path) { }
  void JsonIndexHandler::value(const ElementPath& path, const ElementValue& value) { }
  void JsonIndexHandler::startArray(const ElementPath& path)
  {
    //log_d("Array start %s", path.toString().c_str() );
    for( auto keyFields : params->keys) {
      if( strcmp( keyFields.first.c_str(), path.toString().c_str() ) == 0 ) {
        capturekey = keyFields.first.c_str();
        captured++;
      }
    }
  }
  void JsonIndexHandler::endArray(const ElementPath& path)
  {
    //log_d("Array end %s at offset %d", path.toString().c_str(), params->file->position() );
    for( auto keyFields : params->keys) {
      if( strcmp( keyFields.first.c_str(), path.toString().c_str() ) == 0 ) {
        capturekey = "";
        if( captured == params->keys.size() ) {
          //log_d("Capture end");
          fragment_ended = true;
        }
      }
    }
  }
  void JsonIndexHandler::startObject(const ElementPath& path)
  {
    //log_d("Object start %s", path.toString().c_str() );
    if( capturekey!="" ) {
      params->values[capturekey].push_back( params->file->position()-1 );
    }
  }
  bool JsonIndexHandler::available()
  {
    return params->file->available() && !fragment_ended;
  }



  /********************** JsonObjectFragmentHandler **********************/


  void JsonObjectFragmentHandler::setParams( JsonFragmentObject* params_ )
  {
    assert( params_ );
    params = params_;
    fragment_ended = false;
  };
  void JsonObjectFragmentHandler::startDocument() { /*log_d("Doc::obj start"); */}
  void JsonObjectFragmentHandler::endDocument() { /*log_d("Doc::obj end");*/ fragment_ended = true; }
  void JsonObjectFragmentHandler::startObject(const ElementPath& path) { /*log_d("Obj start");*/  }
  void JsonObjectFragmentHandler::endObject(const ElementPath& path) { /*log_d("Obj end");*/  fragment_ended = true; }
  void JsonObjectFragmentHandler::startArray(const ElementPath& path) { /*log_d("Obj::arr start");*/ }
  void JsonObjectFragmentHandler::endArray(const ElementPath& path) { /*log_d("Obj::arr end");*/ }
  void JsonObjectFragmentHandler::whitespace(const char c) { }
  void JsonObjectFragmentHandler::value(const ElementPath& path, const ElementValue& value)
  {
    const char* currentKey = path.getKey();
    if(currentKey[0] == '\0') {
      // array item
      // SID TAG?
    } else { // name/value pair
      for( int i=0;i<params->keys->size(); i++) {
        if( strcmp(  (*params->keys)[i].c_str(), currentKey ) == 0 ) {
          params->tmpItem[currentKey] = value.toString().c_str();
        }
      }
    }
  }
  bool JsonObjectFragmentHandler::available()
  {
    return params->file->available() && !fragment_ended;
  }



  /********************** Dir **********************/


  Dir::~Dir()
  {
    if( fragmentIndex ) delete fragmentIndex;
    if( fragmentIterator ) delete fragmentIterator;
    close();
  }

  void Dir::close() { if( file ) file.close(); }

  fs::File *Dir::getFile() { return &file; }

  bool Dir::open()
  {
    file = cfg->fs->open( json_file.c_str() );
    if( file ) {
      fragmentIndex = new JsonFragmentIndex( &file );
      fragmentIterator = new JsonFragmentIndexIterator( fragmentIndex );
      dir_pos = 0;
      return true;
    }
    return false;
  }



  DirItem Dir::openNextFile()
  {
    // fetch type and position of next item
    DirItemType_t itemType = fragmentIterator->nextOffset( &dir_pos );

    if( itemType == TYPE_NONE ) {
      return DirItem{ TYPE_NONE, {}, nullptr };
    }

    JsonFragmentObject *objFragment = new JsonFragmentObject( &fragmentIndex->keys[(itemType==TYPE_FOLDER )?"folders":"files"], fragmentIndex->file );

    getFile()->seek( dir_pos );

    goblib::json::StreamingParser parser;
    JsonObjectFragmentHandler handler( objFragment );
    parser.setHandler(&handler);
    while( handler.available() ) {
      parser.parse( getFile()->read() );
    }

    if( objFragment->tmpItem.size() == 0 ) {
      delete objFragment;
      return DirItem{ TYPE_NONE, {}, this };
    }

    DirItem ret{ itemType, objFragment->tmpItem, this };

    delete objFragment;
    return ret;
  }


  void Dir::rewind()
  {
    if( fragmentIterator ) fragmentIterator->position = 0;
  }


  void Dir::forward( size_t steps )
  {
    if( fragmentIterator && fragmentIterator->position + steps < fragmentIterator->size() )
      fragmentIterator->position += steps;
  }

  size_t Dir::size()
  {
    if( fragmentIterator ) return fragmentIterator->size();
    return 0;
  }

  size_t Dir::dirs_count()
  {
    if( fragmentIterator ) return fragmentIterator->dirs_count();
    return 0;
  }

  size_t Dir::files_count()
  {
    if( fragmentIterator ) return fragmentIterator->files_count();
    return 0;
  }

  void Dir::debug()
  {
    log_d("\nname:%s\n, path:%s\n, sidFolder:%s\n, jsonFile:%s\n, apiUrl:%s\n, httpDir:%s\n",
      name()      .c_str(),
      path()      .c_str(),
      sidFolder() .c_str(),
      jsonFile()  .c_str(),
      apiUrl()    .c_str(),
      httpDir()   .c_str()
    );
  }


  void Dir::_init( std::string relative_path )
  {
    bool is_root  = (relative_path=="/");
    basename      = relative_path;
    dir_path      = is_root ? "/" : pathConcat( "/", relative_path );

    sid_folder    = pathConcat( cfg->fs_hvsc_dir, dir_path );
    json_file     = pathConcat( cfg->fs_json_dir, dir_path ) + (is_root ? "root" : "") + ".json";

    json_file     = cleanPath( json_file );  // remove fat32-incompatible chars
    sid_folder    = cleanPath( sid_folder ); // remove fat32-incompatible chars

    api_dir_path  = is_root ? "" : urlencode( dir_path );
    api_url       = cfg->api_url + cfg->api_endpoint + ( is_root ? "" : ("?folder=" + api_dir_path) );

    http_dir_path = pathConcat( cfg->api_hvsc_dir, pathencode( dir_path ) );
    http_dir      = cfg->api_url + http_dir_path;
  }


  /********************** VirtualFS **********************/



  Dir *VirtualFS::openDir( std::string relative_path, bool force_download, tickerCb_t activityCb )
  {
    Dir *dir = new Dir( relative_path, cfg );
    std::string jsonFile = dir->jsonFile();

    if( !force_download ) {
      if( !cfg->fs->exists( jsonFile.c_str() ) ) force_download = true;
      else if( timeSynced && getTime()-file_date(jsonFile)>cfg->cache_max_seconds ) force_download = true;
    }

    if( force_download /*|| ! cfg->fs->exists( jsonFile.c_str() )*/ ) {
      log_d("%s Will fetch JSON File: %s", force_download?"[Download forced]":"[File not found]", jsonFile.c_str() );
      if(activityCb) activityCb("Downloading", 1 );
      if( ! downloadFile( dir->apiUrl(), cfg->fs, jsonFile ) ) {
        delete dir;
        if( activityCb ) activityCb("DL Fail", 1 );
        return nullptr;
      }
    }

    if( !dir->open() ) {
      log_e("File does not exist: %s", jsonFile.c_str() );
      if( activityCb ) activityCb("FS Fail", 1 );
      return nullptr;
    }

    size_t total_bytes = dir->getFile()->size();
    size_t bytes_read = 0;
    size_t last_progress = 0;

    goblib::json::StreamingParser parser;
    JsonIndexHandler handler( dir->fragmentIndex );
    parser.setHandler(&handler);
    while( handler.available() ) {
      parser.parse( dir->getFile()->read() );
      bytes_read++;
      size_t progress = (float(bytes_read)/float(total_bytes))*100.0;
      if( progress != last_progress ) {
        Serial.printf(".");
        last_progress = progress;
        if( activityCb ) activityCb("100/", progress );
      }
    }
    Serial.println();
    log_d("Parser stored indexes for %d folders and %d files", dir->fragmentIndex->values["folders"].size(), dir->fragmentIndex->values["files"].size() );

    return dir;
  }


  /********************** DeepSID **********************/


  void DeepSID::listContents( FOLDERList_t& folders, FILEList_t& files )
  {
    std::string currentPath = "";
    std::string rewrittenPath = "";
    Serial.printf("Got %d folders\n", folders.size() );
    for( int i=0;i<folders.size();i++ ) {
      Serial.printf("[%03d][%8s][ %s (%s items)]\n", i, folders[i]["foldertype"].c_str(), folders[i]["foldername"].c_str(), folders[i]["filescount"].c_str() );
    }
    Serial.printf("Got %d files\n", files.size() );
    for( int i=0;i<files.size();i++ ) {
      Serial.printf("[%03d][%8s][ %s (%s tunes)]\n", i, files[i]["filename"].c_str(), files[i]["sidmodel"].c_str(), files[i]["subtunes"].c_str() );
    }
  }



};



namespace net
{

  bool wifiConnected = false;
  bool timeSynced = false;

  HTTPClient http;


  void wifiOn()
  {
    uint8_t wifi_retry_count = 0;
    uint8_t max_retries = 10;
    unsigned long stubbornness_factor = 3000; // ms to wait between attempts

    #ifdef ESP32
      while (WiFi.status() != WL_CONNECTED && wifi_retry_count < max_retries)
    #else
      while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_retry_count < max_retries)
    #endif
    {
      #if defined WIFI_SSID && defined WIFI_PASS
        WiFi.begin( WIFI_SSID, WIFI_PASS ); // put your ssid / pass if required, only needed once
      #else
        WiFi.begin();
      #endif
      Serial.print(WiFi.macAddress());
      Serial.printf(" => WiFi connect - Attempt No. %d\n", wifi_retry_count+1);
      delay( stubbornness_factor );
      wifi_retry_count++;
    }

    if(wifi_retry_count >= max_retries ) {
      Serial.println("no connection, forcing restart");
      ESP.restart();
    }

    if (WiFi.waitForConnectResult() == WL_CONNECTED){
      Serial.println("Connected as");
      Serial.println(WiFi.localIP());
      wifiConnected = true;
    }
  }


  void wifiOff()
  {
    if( WiFi.getMode() != WIFI_OFF ) {
      WiFi.mode( WIFI_OFF );
    }
    wifiConnected = false;
  }


  void net_cb_main( std::string tickerMsg, std::string logFmt, std::string logVal ) { log_d( "[%s] %s: %s", tickerMsg.c_str(), logFmt.c_str(), logVal.c_str() ); }
  void net_cb_onHTTPBegin(const std::string out) { net_cb_main( "HTTP Begin", "Opening URL", out ); }
  void net_cb_onHTTPGet(const std::string out)   { net_cb_main( "HTTP Get", "Sending request", out ); }
  void net_cb_onFileOpen(const std::string out)  { net_cb_main( "Creating", "Will Save file to", out ); }
  void net_cb_onFileWrite(const std::string out) { net_cb_main( "Writing", "Writing file to", out ); }
  void net_cb_onFileClose(const std::string out) { net_cb_main( "Saved", "Writing file to", out ); }
  void net_cb_onSuccess(const std::string out)   { net_cb_main( "Done!", "Successfully written file to", out ); }
  void net_cb_onFail(const std::string out)      { net_cb_main( "Fail :(", "Failed to writte file to", out ); }

  download_cb_t download_cb_t_default( &net_cb_onHTTPBegin, &net_cb_onHTTPGet, &net_cb_onFileOpen, &net_cb_onFileWrite, &net_cb_onFileClose, &net_cb_onSuccess, &net_cb_onFail );

  bool downloadFile( const std::string url, fs::FS *fs, const std::string savepath, download_cb_t callbacks )
  {
    bool was_connected = wifiConnected;
    if( !wifiConnected ) wifiOn();

    String _url = String( url.c_str() );

    //log_d("Opening URL %s", _url.c_str() );

    callbacks.onHTTPBegin( _url.c_str() );
    http.begin( _url );

    bool ret = false;
    fs::File file;
    String contentType;
    int httpCode;
    const char* UserAgent = "ESP32-SIDView-HTTPClient";
    const char * headerkeys[] = { "content-type" };
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

    http.setUserAgent( UserAgent );
    http.collectHeaders(headerkeys, headerkeyssize);
    http.addHeader( "X-Requested-With", "XMLHttpRequest" );

    callbacks.onHTTPGet( _url.c_str() );
    httpCode = http.GET();

    if(httpCode <= 0) {
      log_e("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
      goto _end;
    }

    if( !http.hasHeader("content-type") ) {
      log_e(" Error, no content type in response");
      goto _end;
    }

    contentType = http.header("content-type");

    if( _url.endsWith(".sid") && contentType != "audio/prs.sid" ) {
      log_e(" Error, bad content type in response: %s", contentType.c_str());
      goto _end;
    }

    //log_d("Saving file to: %s", savepath.c_str() );
    callbacks.onFileOpen( savepath.c_str() );
    file = fs->open( savepath.c_str(), "w", true );

    if(!file ) {
      log_e("Couldn't create file, invalid char in path?");
      goto _end;
    }

    callbacks.onFileWrite( savepath.c_str() );
    if(httpCode == HTTP_CODE_OK) {
      http.writeToStream( &file );
    }

    file.close();
    callbacks.onFileClose( savepath.c_str() );

    _end:
    http.end();
    ret = fs->exists( savepath.c_str() );
    if( !was_connected ) wifiOff();

    if( ret ) callbacks.onSuccess( savepath.c_str() );
    else callbacks.onFail( savepath.c_str() );

    return ret;
  }



};



namespace utils
{

  // FAT32 utility, some chars can't be used in paths
  // replaces '?:' chars by '_'
  std::string cleanPath( std::string dirty )
  {
    String clean = String( dirty.c_str() );
    clean.replace('?', '_');
    clean.replace(':', '_');
    return clean.c_str();
  }


  bool ends_with(std::string const & value, std::string const & ending)
  {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
  }

  bool starts_with(std::string const & value, std::string const & starting)
  {
    if (starting.size() > value.size()) return false;
    return value.find( starting, 0 ) == 0;
  }


  // based on https://github.com/zenmanenergy/ESP8266-Arduino-Examples/ urlencode example
  String urlencode(String str, bool raw)
  {
    String escaped="";
    char c, code0, code1;
    for ( int i =0; i < str.length(); i++ ) {
      c = str.charAt(i);
      if (c == ' ') {
        escaped += raw?"%20":"+";
      } else if ( isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ) {
        escaped+=c;
      } else {
        code1 = (c & 0xf)+'0';
        if ( (c & 0xf) >9 ) {
          code1 = (c & 0xf) - 10 + 'A';
        }
        c = (c>>4) & 0xf;
        code0 = c+'0';
        if ( c > 9 ){
          code0=c - 10 + 'A';
        }
        escaped+='%';
        escaped+=code0;
        escaped+=code1;
      }
      yield();
    }
    return escaped;
  }


  std::string urlencode( std::string str, bool raw )
  {
    String _str = String( str.c_str() );
    _str = urlencode( _str, raw );
    return _str.c_str();
  }



  String pathencode( String str )
  {
    String tmp = str;
    int idx = tmp.indexOf('/');
    if( idx < 0 ) return tmp;

    String out;

    bool ls = tmp[0] == '/'; // leading slash
    bool ts = tmp[tmp.length()-1] == '/'; // trailing slash

    if( ls ) {
      log_v("Has leading slash");
      out = "/";
      tmp.remove(0, 1);
    }

    do {
      idx = tmp.indexOf('/');
      if( idx < 0 ) {
        log_v("no more slash");
        out += urlencode( tmp );
        break;
      }
      if( idx == 0 ) {
        out += "/";
      } else {
        out += urlencode( tmp.substring( 0, idx ) ) + "/";
      }
      tmp.remove( 0, idx+1 );
    } while( tmp.length()>0 );

    if( !ts && out[out.length()-1] == '/' ) {
      out = out.substring( 0, out.length()-2 );
    }
    return out;
  }


  std::string pathencode( std::string str )
  {
    String _str = String( str.c_str() );
    _str = pathencode( _str);
    return _str.c_str();
  }


  // concat two path parts without creating double-slash
  String pathConcat( String path1, String path2 )
  {
    String p1=path1, p2=path2;
    bool p1_ts = p1[p1.length()-1] == '/';
    bool p2_ls = p2[0] == '/';
    if( p1_ts && p2_ls ) p2.remove(0,1);
    else if( !p1_ts && !p2_ls ) p1 += "/";
    return p1+p2;
  }


  std::string pathConcat( std::string path1, std::string path2 )
  {
    String ret = pathConcat( String(path1.c_str()), String(path2.c_str()) );
    return ret.c_str();
  }




  URLParts parseURL( std::string _url ) // logic stolen from HTTPClient::beginInternal()
  {
    String url = String( _url.c_str() );
    URLParts urlParts;
    int index = url.indexOf(':');
    if(index < 0) {
      log_e("failed to parse protocol");
      return urlParts;
    }
    urlParts.url = "" + url;
    urlParts.scheme = url.substring(0, index);
    url.remove(0, (index + 3)); // remove http:// or https://
    index = url.indexOf('/');
    String host = url.substring(0, index);
    url.remove(0, index); // remove host part
    index = host.indexOf('@'); // get Authorization
    if(index >= 0) { // auth info
      urlParts.auth = host.substring(0, index);
      host.remove(0, index + 1); // remove auth part including @
    }
    index = host.indexOf(':'); // get port
    if(index >= 0) {
      urlParts.host = host.substring(0, index); // hostname
      host.remove(0, (index + 1)); // remove hostname + :
      urlParts.port = host.toInt(); // get port
    } else {
      urlParts.host = host;
    }
    urlParts.uri = url;

    index = urlParts.uri.indexOf('?'); // has query
    if( index >= 0 ) {
      urlParts.query = urlParts.uri.substring( index+1, urlParts.uri.length()-1 );
      urlParts.path  = urlParts.uri.substring( 0, index );
    } else {
      urlParts.path  = urlParts.uri;
    }

    index = urlParts.query.indexOf('#'); // has fragment
    if( index >= 0 ) {
      urlParts.fragment = urlParts.query.substring( index+1, urlParts.uri.length()-1 );
      urlParts.query.remove( index, urlParts.query.length()-1 );
    }

    return urlParts;
  }


  URLParts parseURL( String url )
  {
    std::string _url = url.c_str();
    return parseURL( _url );
  }



  void rewrite( String &strIn, String search, String replace )
  {
    bool had_leading_slash = strIn[0] == '/'; // check if string has leading slash as it may be removed during process
    strIn.replace(search, replace );
    while( strIn.indexOf("//")>=0 ) {
      strIn.replace( "//", "/" ); // strip double slashes if any
    }
    if( had_leading_slash && strIn[0] != '/' ) {
      // restore leading slash
      strIn = "/" + strIn;
    }
  }

  void rewrite( std::string &strIn, std::string search, std::string replace )
  {
    String _strIn   = String( strIn.c_str() );
    String _search  = String( search.c_str() );
    String _replace = String( replace.c_str() );
    rewrite( _strIn, _search, _replace );
    strIn = _strIn.c_str();
  }


  // in: /virtual/path/to/file, out: /rewritten/path/file
  // used for API calls
  std::string pathRewrite( const char* val )
  {
    auto _cfg = deepSID.config();
    String path = val;
    String replace = String( _cfg.api_hvsc_dir.c_str() );
    if( path.indexOf('/')<0 ) {
      path = pathConcat( replace, path );
    }
    String find    = String( _cfg.rewritecond.c_str() );
    rewrite( path, find /*, replace*/ );
    return path.c_str();
  };


  // in: /virtual/path/to/foldername, out: /replaced/path/to/foldername
  // used for FILESYSTEM calls
  std::string pathTranslate( std::string folder_path )
  {
    String _folder_path = String( folder_path.c_str() );
    String find = String( deepSID.config().rewritecond.c_str() );
    rewrite( _folder_path, find );
    String json_dir = String( deepSID.config().fs_json_dir.c_str() );
    String clean_path = pathConcat( json_dir, _folder_path );
    return clean_path.c_str();
  };



  uint32_t mm_ss_to_millis( String mm_ss )
  {
    int idx = mm_ss.indexOf(':');
    if( idx < 1 ) return 0;
    String mmStr = mm_ss.substring( 0, idx );
    String ssStr = mm_ss.substring(idx+1, mm_ss.length() );
    int mm = atoi(mmStr.c_str() ) * 60000;
    int ss = (atoll(ssStr.c_str() ) * 1000.0);
    log_v("%s => mm(%s), ss(%s)", mm_ss.c_str(), mmStr.c_str(), ssStr.c_str() );
    return mm+ss;
  }


  void setDurations( String _durations, SID_Meta_t *meta )
  {
    String durations = _durations;
    durations.trim();
    if( meta->durations == nullptr ) {
      if( meta->subsongs > 0 ) {
        meta->durations = (uint32_t*)sid_calloc(meta->subsongs, sizeof(uint32_t));
        if( meta->durations == NULL ) {
          log_e("Failed to alloc memory for song lengths :-(");
          return;
        }
      } else {
        log_d("No durations, using default");
        return;
      }
    }
    log_v("Iterating %d durations", meta->subsongs);
    for( int i=0; i < meta->subsongs; i++ ) {
      int idx = durations.indexOf(' ');
      if( idx < 0 ) {
        meta->durations[i] = mm_ss_to_millis( durations );
        log_v("Duration #%d = %d ms (in=%s)", i, meta->durations[i], durations.c_str() );
        // end of buffer
        break;
      }
      String chunk = durations.substring( 0, idx );
      meta->durations[i] = mm_ss_to_millis( chunk );
      log_v("Duration #%d = %d ms (in=%s)", i, meta->durations[i], chunk.c_str() );
      durations.remove( 0, idx+1 );
    }
  }



  unsigned long getTime()
  {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      //Serial.println("Failed to obtain time");
      return(0);
    }
    time(&now);
    return now;
  }


  unsigned long file_date( std::string path )
  {
    auto file = deepSID.config().fs->open( path.c_str() );
    if( !file ) {
      log_w("Tried to check date on nonexistent file %s", path.c_str() );
      return getTime(); // fail gracefully
    }
    time_t t = file.getLastWrite();
    file.close();
    return t;
    //struct tm * lastmod = localtime(&t);
    //Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(lastmod->tm_year)+1900,( lastmod->tm_mon)+1, lastmod->tm_mday,lastmod->tm_hour , lastmod->tm_min, lastmod->tm_sec);

  }




};


Deepsid::DeepSID deepSID;

