#if !defined(_CONTROL_ASSIST_H)
#define  _CONTROL_ASSIST_H

#include <WebSocketsServer.h>

#define CT_CLASS_VERSION "1.0.7"        // Class version

// Define Platform libs
#if defined(ESP32)
  #define WEB_SERVER WebServer
#else
  #define WEB_SERVER ESP8266WebServer
#endif

#include "espLogger.h"

typedef std::function<void(uint8_t ndx)> WebSocketServerEventG;
typedef std::function<void(void)> WebSocketServerEvent;

//Structure for control elements
struct ctrlPairs {
    String key;
    String val;
    WebSocketServerEvent ev;
    bool autoSendOnCon;
};

class ControlAssist{ 
  public:
    ControlAssist(); 
    ControlAssist(uint16_t port);    
    ~ControlAssist() {};
  public:
    // Start web sockets server
    void begin();
    // Setup web server handlers
    void setup(WEB_SERVER &server, const char *uri = "/");
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
    // Stop web sockets
    void close();
  public:    
    // Implement operator [] i.e. val = config['key']    
    String operator [] (String k) { return getVal(k); }
    // Implement operator [] i.e. val = config[ndx]    
    ctrlPairs operator [] (uint8_t ndx) { return _ctrls[ndx]; }
    // Return the val of a given key, Empty on not found
    String getVal(String key);
     // Return the position of given key
    int getKeyNdx(String key);
    // Return next key and value from configs on each call in key order
    bool getNextPair(ctrlPairs &c);  
    // Add vectors by key (name in ctrlPairs)
    size_t add(String key, String val);
    // Add vectors pairs
    size_t add(ctrlPairs &c);
    // Set the val at key index, (int)
    bool set(int keyNdx, int val, bool forceSend = false);
    // Set the val at key index, (string)
    bool set(int keyNdx, String val, bool forceSend = false);
    // Put val (int) to Key. forcesend to false to send changes only, forceAdd to add it if not exists
    bool put(String key, int val, bool forceSend = false, bool forceAdd = false);
    // Put val (string) to Key. forcesend to false to send changes only, forceAdd to add it if not exists
    bool put(String key, String val, bool forceSend = false, bool forceAdd = false);
    // Display config items
    void dump();
  public:
    // Sort vectors by key (name in confPairs)
    void sort();
    // Run websockets
    void loop() { if(_socket) _socket->loop(); }
    // Get the initialization java script 
    String getInitScript();
    // Render html to client
    void sendHtml(WEB_SERVER &server);
    // Render html to client
    void sendHtml() {sendHtml(*_server); };
    // Get number of connected clients
    uint8_t getClientsNum() { return _clientsNum; }
    // Set the auto send on ws connection flag on key
    bool setAutoSendOnCon(String key, bool send);
    // Set the auto send on ws connection flag on all keys
    void setAutoSendOnCon(bool send);
  private:
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
    uint8_t _clientsNum;
    const char* _html_headers;
    const char* _html_body;
    const char* _html_footer;
    uint16_t _port;
    bool _wsEnabled;
    WEB_SERVER *_server;
    WebSocketsServer *_socket;
    WebSocketServerEventG _ev;
};

#endif // _CONTROL_ASSIST_H
