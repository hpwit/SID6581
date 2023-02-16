#pragma once

#include <FS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <gob_json.hpp> // https://github.com/GOB52/gob_json

#include "../Player/SID_Track.h" // import SID_Meta_t struct

#include <vector>
#include <map>


namespace net
{

  extern HTTPClient http;
  extern bool wifiConnected;
  extern bool timeSynced;

  typedef void(*net_cb_t)(const std::string out);
  void net_cb_main( std::string tickerMsg, std::string logFmt, std::string logVal );
  void net_cb_onHTTPBegin(const std::string out="");
  void net_cb_onHTTPGet(const std::string out="");
  void net_cb_onFileOpen(const std::string out="");
  void net_cb_onFileWrite(const std::string out="");
  void net_cb_onFileClose(const std::string out="");
  void net_cb_onSuccess(const std::string out="");
  void net_cb_onFail(const std::string out="");

  struct download_cb_t
  {
    download_cb_t( net_cb_t onHTTPBegin_, net_cb_t onHTTPGet_, net_cb_t onFileOpen_, net_cb_t onFileWrite_, net_cb_t onFileClose_, net_cb_t onSuccess_, net_cb_t onFail_ )
    : onHTTPBegin(onHTTPBegin_), onHTTPGet(onHTTPGet_), onFileOpen(onFileOpen_), onFileWrite(onFileWrite_), onFileClose(onFileClose_), onSuccess(onSuccess_), onFail(onFail_) {}
    net_cb_t onHTTPBegin;
    net_cb_t onHTTPGet;
    net_cb_t onFileOpen;
    net_cb_t onFileWrite;
    net_cb_t onFileClose;
    net_cb_t onSuccess;
    net_cb_t onFail;
  };

  extern download_cb_t download_cb_t_default;

  void wifiOn();
  void wifiOff();

  bool downloadFile( const std::string url, fs::FS *fs, const std::string savepath, download_cb_t callbacks=download_cb_t_default );

};


namespace utils
{

  struct URLParts
  {
    String url;
    String uri;
    String scheme;
    String host;
    String port;
    String auth;
    String path;
    String query;
    String fragment;
  };

  String urlencode(String str, bool raw=true);
  std::string urlencode( std::string str, bool raw=true);

  String pathencode( String str );
  std::string pathencode( std::string str);

  URLParts parseURL( String url );
  URLParts parseURL( std::string url );

  std::string pathTranslate( std::string folder_path );
  std::string pathRewrite( const char* val );

  String pathConcat( String path1, String path2 );
  std::string pathConcat( std::string path1, std::string path2 );

  std::string cleanPath( std::string dirty );

  void rewrite( String &strIn, String search, String replace="" );
  void rewrite( std::string &strIn, std::string search, std::string replace="" );

  bool ends_with(std::string const & value, std::string const & ending);
  bool starts_with(std::string const & value, std::string const & starting);

  template<class T>
  T base_name(T const & path, T const & delims = "/\\")
  {
    return path.substr(path.find_last_of(delims) + 1);
  }


  uint32_t mm_ss_to_millis( String mm_ss );

  void setDurations( String _durations, SID_Meta_t *meta );

  unsigned long file_date( std::string path );
  unsigned long getTime();


};



namespace Deepsid
{

  using namespace net;
  using namespace utils;

  using goblib::json::ElementPath;
  using goblib::json::ElementValue;

  // name/value pairs
  typedef struct std::map<std::string,std::string> FOLDER_t;
  typedef struct std::map<std::string,std::string> FILE_t;
  // arrays
  typedef struct std::vector<FOLDER_t> FOLDERList_t;
  typedef struct std::vector<FILE_t>   FILEList_t;

  struct config_t;
  class JsonIndexHandler;
  class JsonFragmentIndex;
  class JsonFragmentIndexIterator;
  class JsonFragmentObject;
  class JsonObjectFragmentHandler;
  class VirtualFS;
  class Dir;
  class DirItem;
  class DeepSID;


  struct config_t
  {
    std::string api_url      = "http://deepsid.chordian.net"; // API URL *without the endpoint path*
    std::string api_endpoint = "/php/hvsc.php"; // API endpoint path
    std::string api_hvsc_dir = "/hvsc"; // real path to SID folder in Deepsid site
    std::string fs_json_dir  = "/json/deepsid";   // absolute path to JSON files root directory on filesystem
    std::string fs_hvsc_dir;   // absolute path to HVSC directory on filesystem, where SID files will be saved, typically "/sid"
    // path translation for SID files
    std::string rewritecond  = "_High Voltage SID Collection"; // fancy name for root folder in Deepsid API
    //size_t      page_size    = 1024;
    fs::FS *fs;             // &SD, &LittleFS;
    uint32_t cache_max_seconds = 86400; // don't update JSON unless this seconds old
  };


  struct JsonFragmentIndex
  {
    JsonFragmentIndex( fs::File * file_ ) : file(file_){  }
    fs::File * file = nullptr;
    // paths to capture
    std::map<std::string,std::vector<std::string>> keys = {
      { "folders", {"foldername", "foldertype", "filescount", "incompatible", "hvsc", "ss_type", "ss_query", "rf_path"} },
      { "files",   {"filename", "substname", "lengths", "type", "clockspeed", "sidmodel", "datasize", "loadaddr", "initaddr",
                    "playaddr", "subtunes", "startsubtune", "name", "author", "copyright", "hvsc", "profile", "rf_path"} }
    };
    // captured indexes
    std::map<std::string,std::vector<size_t>> values = {
      { "folders", {} },
      { "files",   {} }
    };
  };


  enum DirItemType_t
  {
    TYPE_NONE,
    TYPE_FOLDER,
    TYPE_FILE
  };


  class DirItem
  {
    public:
      DirItem( DirItemType_t type_, std::map<std::string,std::string> attr_, Dir* dir_ ) : type(type_), attr(attr_), dir(dir_) { };
      DirItemType_t type = TYPE_NONE;
      std::map<std::string,std::string> attr;
      Dir* dir = nullptr;
      std::string name();
      std::string path( bool rewrite = false );
      std::string url();
      static bool getSIDMeta( DirItem &item, SID_Meta_t* meta );
  };



  class Dir
  {
    public:
      Dir( std::string relative_path, config_t *cfg_ ) : cfg(cfg_) { _init( relative_path); }
      ~Dir();
      std::string name()      { return basename; }
      std::string path()      { return dir_path; }
      std::string sidFolder() { return sid_folder; }
      std::string jsonFile()  { return json_file; }
      std::string apiUrl()    { return api_url; }
      std::string httpDir()   { return http_dir; }
      JsonFragmentIndex *fragmentIndex = nullptr;            // JSON Fragment indexes
      JsonFragmentIndexIterator *fragmentIterator = nullptr; // JSON Values SAX-like parser
      bool open();
      void close();
      void rewind();
      void forward( size_t steps );
      DirItem openNextFile();
      size_t size();
      size_t dirs_count();
      size_t files_count();
      fs::File *getFile();
      void debug();
    private:
      void _init( std::string relative_path );
      config_t *cfg;
      fs::File file;
      size_t dir_pos = 0;        // temporary placeholder when walking dir
      std::string basename;      // blah/path to/folder            [maybe leading/trailing slash]
      std::string dir_path;      // /blah/path to/folder           [leading slash]
      std::string sid_folder;    // /sid/blah/path to/folder       [complete path to dir]
      std::string json_file;     // /json/deepsid/blah/path to/folder.json
      std::string api_dir_path;  // %2Fblah%2Fpath%20to%2Ffolder   [url encoded]
      std::string api_url;       // http://server.com/api.endpoint?folder=%2Fblah%2Fpath%20to%2Ffolder
      std::string http_dir_path; // /hvsc/blah/path%20to/folder    [path encoded]
      std::string http_dir;      // http://server.com/hvsc/blah/path%20to/folder
  };



  class JsonFragmentIndexIterator
  {
    public:
      JsonFragmentIndexIterator( JsonFragmentIndex* fragment_ ) : fragment(fragment_) { position=0; };
      JsonFragmentIndex* fragment;
      size_t position = 0;
      //size_t count = 0;
      size_t size();
      size_t dirs_count();
      size_t files_count();
      DirItemType_t nextOffset( size_t *next_offset );
  };


  class JsonFragmentObject
  {
    public:
      JsonFragmentObject( std::vector<std::string> *keys_, fs::File *file_ ) : keys(keys_), file(file_) {  }
      std::vector<std::string> *keys; // key names
      std::map<std::string,std::string> tmpItem; // output map
      fs::File *file;
  };


  typedef bool(*tickerCb_t)(const char* title, size_t count);

  class VirtualFS
  {
    public:
      VirtualFS( config_t *cfg_ ) : cfg(cfg_) { };
      Dir *openDir( std::string relative_path, bool force_download = true, tickerCb_t cb=nullptr );
    private:
      config_t *cfg;
  };



  // parse whole json for index entries and store object offsets
  class JsonIndexHandler: public goblib::json::Handler
  {
    private:
      JsonFragmentIndex* params;
      bool fragment_ended = false;
      std::string capturekey = "";
      int captured = 0;
    public:
      JsonIndexHandler( JsonFragmentIndex* params_ )  { assert( params_ ); params = params_; fragment_ended = false; captured = 0; };
      void startDocument();
      void endDocument();
      void whitespace(const char c);
      void endObject(const ElementPath& path);
      void value(const ElementPath& path, const ElementValue& value);
      void startArray(const ElementPath& path);
      void endArray(const ElementPath& path);
      void startObject(const ElementPath& path);
      bool available();
  };


  // name/value parsing of a single JSON Fragment: the closing bracket of the
  // fragment will trigger and endDocument event
  class JsonObjectFragmentHandler: public goblib::json::Handler
  {
    private:
      JsonFragmentObject* params;
      bool fragment_ended = false;
    public:
      JsonObjectFragmentHandler( JsonFragmentObject* params_ ) { setParams( params_ ); };
      void setParams( JsonFragmentObject* params_ );
      void startDocument();
      void endDocument();
      void startObject(const ElementPath& path);
      void endObject(const ElementPath& path);
      void startArray(const ElementPath& path);
      void endArray(const ElementPath& path);
      void whitespace(const char c);
      void value(const ElementPath& path, const ElementValue& value);
      bool available();
  };



  // controller
  class DeepSID
  {
  public:
    ~DeepSID() { if( FS ) delete FS; };
    void config(const config_t& config) { _cfg = config; if( FS ) delete FS; FS = new VirtualFS(&_cfg); };
    const config_t& config(void) const { return _cfg; };
    void listContents( FOLDERList_t& folders, FILEList_t& files );
    VirtualFS *FS = nullptr;
  protected:
    bool root_loaded = false;
    config_t _cfg;
  };


};


extern Deepsid::DeepSID deepSID;

