#if !defined(_CONTROL_ASSIST_H)
#define  _CONTROL_ASSIST_H

#include <vector>
#include <WebSocketsServer.h>

#ifndef LOGGER_LOG_LEVEL
  #define LOGGER_LOG_LEVEL 4     // Set log level for this module
#endif

#define CT_CLASS_VERSION "1.1.6"          // Class version
#define CTRLASSIST_STREAM_CHUNKSIZE 2048  // Stream file buffer size

#if !defined(STORAGE)
  #if defined(ESP8266) || defined(CA_USE_LITTLEFS)
    #include <LittleFS.h>
    #define STORAGE LittleFS // one of: SPIFFS LittleFS SD_MMC
  #else
    #include "SPIFFS.h"
    #define STORAGE SPIFFS // SPIFFS
  #endif
#endif

#include "espLogger.h"

// Web socket events
typedef std::function<void(uint8_t ndx)> WebSocketServerEventG;
typedef std::function<void(void)> WebSocketServerEvent;

//Structure for control elements
struct ctrlPairs {
    String key;
    String val;
    WebSocketServerEvent ev;
    bool autoSendOnCon;
};

// Positions of keys inside array
struct ctrlsNdx {
    String key;
    size_t ndx;
};

class ControlAssist{
  public:
    ControlAssist();
    ControlAssist(uint16_t port);
    ~ControlAssist() {};
  public:
    // Start web sockets server
    void begin();
    // Set web sockets server listening port
    void setPort(uint16_t port) {if(!_wsEnabled)  _port = port; }
    // Bind a html control with id = key to a control variable
    int  bind(const char* key);
    // Bind a html control with id = key to a control variable and an on event function
    int  bind(const char* key, WebSocketServerEvent ev);
    // Bind a html control with id = key to a control variable with start int val
    int  bind(const char* key, const int val);
    // Bind a html control with id = key to a control variable with start char val
    int  bind(const char* key, const char* val);
    // Bind a html control with id = key to a control variable and an on event function
    int  bind(const char* key, const int val, WebSocketServerEvent ev);
    // Bind a html control with id = key to a control variable, a start val and an on event function
    int  bind(const char* key, const char* val, WebSocketServerEvent ev);

    // Add a global callback function to handle changes
    void setGlobalCallback(WebSocketServerEventG ev);
    // Set html page code
    void setHtmlHeaders(const char *html_headers) { _html_headers = html_headers; }
    void setHtmlBody(const char *html_body) { _html_body = html_body; }
    void setHtmlFooter(const char *html_footer) { _html_footer = html_footer; }
    // Set html page code from file
    void setHtmlHeadersFile(const char *fileName) { _html_headers_file = fileName; }
    void setHtmlBodyFile(const char *fileName) { _html_body_file = fileName; }
    void setHtmlFooterFile(const char *fileName) { _html_footer_file = fileName; }
    // Stop web sockets
    void close();
  public:
    // Implement operator [] i.e. val = config['key']
    String operator [] (String k) { return getVal(k); }
    // Implement operator [] i.e. val = config[ndx]
    ctrlPairs operator [] (uint8_t ndx) { return _ctrls[ndx]; }
    // Return the val of a given key, Empty on not found
    String getVal(const String &key);
     // Return the position of given key
    int getKeyNdx(const String &key);
    // Return next key and value from configs on each call in key order
    bool getNextPair(ctrlPairs &c);
    // Add vectors by key (name in ctrlPairs)
    size_t add(const String &key, const String &val);
    // Add vectors pairs
    size_t add(ctrlPairs &c);
    // Set the val at key index, (int)
    bool set(int keyNdx, int val, bool forceSend = false);
    // Set the val at key index, (string)
    bool set(int keyNdx, const String &val, bool forceSend = false);
    // Put val (int) to Key. forcesend to false to send changes only, forceAdd to add it if not exists
    bool put(const String &key, int val, bool forceSend = false, bool forceAdd = false);
    // Put val (string) to Key. forcesend to false to send changes only, forceAdd to add it if not exists
    bool put(const String &key, const String &val, bool forceSend = false, bool forceAdd = false);
    // Send system message. (Connection close)
    bool sendSystemMsg(const String &msg);
    // Display config items
    bool dump(String &res);

  public:
    // Sort vectors by key (name in confPairs)
    void sort();
    // Run websockets
    void loop() { if(_socket) _socket->loop(); }
    // Get the initialization java script
    String getInitScript();
    // Get a chunk from a html file
    bool getFileChunk(const char *fname, String &res);
    // Get a chunk from a html buffer
    bool getStringChunk(const char *src, String &res);
    // Render html to client
    bool getHtmlChunk(String &res);
    // Get number of connected clients
    uint8_t getClientsNum() { return _clientsNum; }
    // Set the auto send on ws connection flag on key
    bool setAutoSendOnCon(const String &key, bool send);
    // Set the auto send on ws connection flag on all keys
    void setAutoSendOnCon(bool send);
  private:
    // Load a file into text
    bool loadText(const String &fPath, String &txt);
    // Start websockets
    void startWebSockets();
    // Stop websockets
    void stopWebSockets();
    // Send the keys with auto send flag on ws connect
    void autoSendKeys(uint8_t num);
    // Response to a websocket event
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
  private:
    std::vector<ctrlPairs> _ctrls;
    std::vector<ctrlsNdx> _keysNdx;
    uint8_t _clientsNum;
    const char* _html_headers;
    const char* _html_body;
    const char* _html_footer;
    const char* _html_headers_file;
    const char* _html_body_file;
    const char* _html_footer_file;
    uint16_t _port;
    bool _wsEnabled;
    WebSocketsServer *_socket;
    WebSocketServerEventG _ev;
};

#endif // _CONTROL_ASSIST_H
