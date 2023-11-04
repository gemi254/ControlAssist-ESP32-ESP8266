#include <Arduino.h>
#include <sstream>
#include <vector>
#if defined(ESP32)
  #include <WebServer.h>
#else
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
//  #include "TZ.h"
#endif

#include <WebSocketsServer.h>
#include "controlAssistPMem.h" //Memory static valiables (html pages)
#define LOGGER_LOG_LEVEL 4     //Set log level for this module
#include "ControlAssist.h"

WebSocketsServer *ControlAssist::_pWebSocket = NULL;
std::vector<ctrlPairs> ControlAssist::_ctrls;
std::vector<String> ControlAssist::_chnToKeys;
WebSocketServerEventG ControlAssist::_ev;
uint8_t ControlAssist::_clientsNum;

ControlAssist::ControlAssist() { 
  _pWebSocket = NULL; 
  _wsEnabled = false; 
  _ev = NULL;
  _html_headers = CONTROLASSIST_HTML_HEADER;
  _html_body =  CONTROLASSIST_HTML_BODY;
  _html_footer = CONTROLASSIST_HTML_FOOTER;
  _clientsNum = 0;
  _port = 81;
}

ControlAssist::ControlAssist(uint16_t port)
: ControlAssist(){
  _port = port;
}
// Start websockets
void ControlAssist::begin(){
  if(!_wsEnabled){
    startWebSockets();
    _wsEnabled = true;
  }
}
// Stop websockets
void ControlAssist::close(){
  if(_wsEnabled){
    stopWebSockets();
    _wsEnabled = false;
  }
}
// Start the websocket server
void ControlAssist::startWebSockets(){
  if(_pWebSocket) return;
  _pWebSocket = new WebSocketsServer(_port);
  _pWebSocket->begin();
  _pWebSocket->onEvent(this->webSocketEvent);
  LOG_I("Started web sockets at port: %u\n", _port);
}
// Stop the websocket server
void ControlAssist::stopWebSockets(){
  if(_pWebSocket == NULL) return;
  _pWebSocket->close();
  delete _pWebSocket;
  _pWebSocket = NULL;
  LOG_I("Stoped web sockets at port: %u\n", _port);
}
// Get the val of a given key, Empty on not found
String ControlAssist::get(String key) {
  int keyPos = getKeyPos(key);
  if (keyPos >= 0) {
    return _ctrls[keyPos].val;        
  }
  return String(""); 
}
// Get the key of a channel
String ControlAssist::getKey(uint8_t channel){
  if(channel<=0 || channel >= (uint8_t)_chnToKeys.size() ) return "";
  return _chnToKeys[channel];
}

// Add vectors by key (name in confPairs)
size_t ControlAssist::add(String key, String val){      
  size_t cnt = _ctrls.size();
  ctrlPairs d = { key, "", cnt + 1, NULL, false };
  return add(d);
}   
// Add vectors pairs
size_t ControlAssist::add(ctrlPairs &c){
  _ctrls.push_back({c});  
  return _ctrls.size();
}
// Update the val at pos, (int)
bool ControlAssist::set(int keyPos, int val, bool forceSend) {
  return set(keyPos, String(val), forceSend);
}
// Update the val at pos, (string)
bool ControlAssist::set(int keyPos, String val, bool forceSend) {
  bool changed = false;
  if (keyPos >= 0 && (size_t)keyPos < _ctrls.size() ) {
    if( _ctrls[keyPos].val != val){
      LOG_V("Set [%i] = %s\n", keyPos, val.c_str()); 
      _ctrls[keyPos].val = val;
      changed = true;
    }
  }else{
    LOG_W("Set failed on pos: [%i] = %s\n", keyPos, val.c_str());
    return false;    
  } 
  // send message to client
  if(_clientsNum > 0 && (forceSend || changed)){
    String payload = String(_ctrls[keyPos].readNo) + "\t" + _ctrls[keyPos].val;
    if(_pWebSocket) _pWebSocket->broadcastTXT(payload);
    LOG_V("Send payload: %s\n", payload.c_str());
  } 
  return true; 
} 

// Update the val of key = val (int)
bool ControlAssist::put(String key, int val, bool forceSend, bool forceAdd) {
  return put(key, String(val), forceSend, forceAdd); 
} 
    
// Update the val of key = val (string) forceAdd to add it even if not exists, forceSend = false to send changes only
bool ControlAssist::put(String key, String val, bool forceSend, bool forceAdd) { 
  int keyPos = getKeyPos(key);
  bool changed = false;
  if (keyPos >= 0 && (size_t)keyPos < _ctrls.size() ) {
    if( _ctrls[keyPos].val != val){
      LOG_V("Put [%i] = %s\n", keyPos, val.c_str()); 
      _ctrls[keyPos].val = val;
      changed = true;
    }
  }else if(forceAdd) {
    keyPos = add(key, val);
    sort();
    changed = true;
  }else{
    LOG_W("Put failed on key: %s = %s\n", key.c_str(), val.c_str());
    return false;
  }
  //Send message to client
  if(_clientsNum > 0 && (forceSend || changed)){
    String payload = String(_ctrls[keyPos].readNo) + "\t" + _ctrls[keyPos].val;
    if(_pWebSocket) _pWebSocket->broadcastTXT(payload);
    LOG_V("Send payload: %s\n", payload.c_str());
  } 
  return true;
}
// Set the auto send on ws connection flag on key
bool ControlAssist::setAutoSendOnCon(String key, bool send) {
  int keyPos = getKeyPos(key);
  if (keyPos >= 0 && (size_t)keyPos < _ctrls.size() ) {
    _ctrls[keyPos].autoSendOnCon = send;
    return true;
  }
  LOG_E("Set auto send failed on key: %s\n", key.c_str());
  return false;
} 
// Set the auto send on ws connection flag on all keys
void ControlAssist::setAutoSendOnCon(bool send) {
  uint8_t row = 0;
  while (row++ < _ctrls.size()) { 
    _ctrls[row - 1].autoSendOnCon = send;
  }
}

// On websocket connection send the keys with auto send flag to clients
void ControlAssist::autoSendKeys(){
  uint8_t row = 0;
  while (row++ < _ctrls.size()) { 
    LOG_V("Checking row: %s ,%i\n", _ctrls[row - 1].key.c_str(), _ctrls[row - 1].autoSendOnCon );
    if(_ctrls[row - 1].autoSendOnCon){
      String payload = String(_ctrls[row - 1].readNo) + "\t" + _ctrls[row - 1].val;
      if(_pWebSocket) _pWebSocket->broadcastTXT(payload);
      LOG_D("Auto send payload: %s\n", payload.c_str());
    }
  }
}

// Get the location of given key to retrieve control
int ControlAssist::getKeyPos(String key) {
  if (_ctrls.empty()) return -1;
  auto lower = std::lower_bound(_ctrls.begin(), _ctrls.end(), key, [](
    const ctrlPairs &a, const String &b) { 
    return a.key < b;}
  );
  int keyPos = std::distance(_ctrls.begin(), lower); 
  if (key == _ctrls[keyPos].key) return keyPos;
  else LOG_V("Key %s not exist\n", key.c_str()); 
  return -1; // not found
}

// Return next key and val from configs on each call in key order
bool ControlAssist::getNextKeyVal(ctrlPairs &c) {
  static uint8_t row = 0;
  if (row++ < _ctrls.size()) { 
    c = _ctrls[row - 1];
    return true;
  }
  // end of vector reached, reset
  row = 0;
  return false;
}

// Sort vectors by key (name in confPairs)
void ControlAssist::sort(){
  std::sort(_ctrls.begin(), _ctrls.end(), [] (
    const ctrlPairs &a, const ctrlPairs &b) {
    return a.key < b.key;}
  );
}

// Dump config items
void ControlAssist::dump(){
  LOG_I("Dump configuration\n");
  for(size_t row = 0; row< _chnToKeys.size(); row++) {
    LOG_I("%i = %s\n", row, _chnToKeys[row].c_str());
  }

  ctrlPairs c;
  while (getNextKeyVal(c)){
    LOG_I("[%u]: %s \n", c.readNo, c.key.c_str());
  }
}

// Bind a html control with id = key to a control variable
int ControlAssist::bind(const char* key){
  int p = getKeyPos(key);
  if(p >= 0){
    LOG_E("Bind key: %s failed. Already exists!\n", key);
    return -1;
  }
  size_t cnt = _ctrls.size();
  ctrlPairs d = { key, "", cnt + 1, NULL, false};
  LOG_I("Binding key %s, %u\n", key, cnt); 
  _ctrls.push_back(d);
  sort();
  _chnToKeys.push_back(key);
  String keyHtml ( "id=\"" + String(key) + "\"" );
  /*if (strstr(_html_body, keyHtml.c_str()) == NULL) {
    LOG_E("Key: %s not found in html\n", key);
  }*/
  return (int)_ctrls.size();
}

// Bind a html control with id = key to a control variable and an event function
int ControlAssist::bind(const char* key, WebSocketServerEvent ev){
  size_t cnt = _ctrls.size();
  ctrlPairs d = { key, "", cnt + 1, ev, false };
  LOG_I("Binding key %s, %i\n", key, cnt); 
  _ctrls.push_back(d);
  sort();
  _chnToKeys.push_back(key);
  return getKeyPos(key);
}

// Add a global callback function to handle changes
void ControlAssist::setGlobalCallback(WebSocketServerEventG ev){
  _ev = ev;
}

// Response to a websocket event
void ControlAssist::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
      case WStype_PING:
      case WStype_PONG: 
          break;
      case WStype_DISCONNECTED:
          LOG_E("Websocket [%u] Disconnected!\n", num);
          _clientsNum--;
          break;
      case WStype_CONNECTED:
          {
            IPAddress ip = _pWebSocket->remoteIP(num);
            LOG_I("Websocket [%u] connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            _clientsNum++;
            // send message to client
            _pWebSocket->sendTXT(num, "0\tCon ");
            //Send keys selected
            autoSendKeys();
          }
          break;
      case WStype_TEXT:
          {
            char cNo = ( (char)payload[0]);
            int id = cNo - '0' - 1;
            if(id < 0) return;
            payload += 2;
            std::string val = reinterpret_cast<char *>(payload);
         
            int no = getKeyPos(_chnToKeys[id]);
            _ctrls[no].val = String(val.c_str());
            //Call control change handler
            if(_ctrls[no].ev) {
              _ctrls[no].ev();
            }
            //Call global change handler
            if(_ev){
              _ev(id);
            }
            LOG_V("[%u] cid: %i no: %i get Text: %s\n", num, id, no, val.c_str());
          }
          break;
      case WStype_BIN:
          LOG_D("[%u] get binary length: %u\n", num, length);
          // send message to client
          // webSocket.sendBIN(num, payload, length);
          break;
      case WStype_ERROR:			
      case WStype_FRAGMENT_TEXT_START:
      case WStype_FRAGMENT_BIN_START:
      case WStype_FRAGMENT:
      case WStype_FRAGMENT_FIN:
        LOG_E("[%u] WS error: %x\n", num, type);
        break;
    }
}

// Build the initialization java script 
String ControlAssist::getInitScript(){
 String ctlPort = "const port = " + String(_port)+";";
 String ctlByNo = "const ctlByNo = { ";
 String ctlbyName = "const ctlbyName = { ";
 for(uint8_t r = 0; r< _chnToKeys.size(); r++) { 
    int no = getKeyPos(_chnToKeys[r]);
    ctlByNo   += "'" + String(_ctrls[no].readNo) + "': '" + _ctrls[no].key + "', ";
    ctlbyName += "'" + String(_ctrls[no].key) + "': '" + _ctrls[no].readNo + "', ";
  }
  if(ctlByNo.endsWith(", ")) ctlByNo = ctlByNo.substring(0, (ctlByNo.length() - 2));
  if(ctlbyName.endsWith(", ")) ctlbyName = ctlbyName.substring(0, (ctlbyName.length() - 2));
  ctlByNo   += "};";
  ctlbyName += "};";
  return ctlPort + "\n" + ctlByNo +"\n" + ctlbyName;
}

// Render html to client
void ControlAssist::sendHtml(WEB_SERVER &server){
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendContent(_html_headers,strlen(_html_headers));
  String definitions = getInitScript();
  LOG_V("Script %s\n", definitions.c_str());
  String scripts = "<script type=\"text/javascript\">";
  scripts += definitions;
  scripts += CONTROLASSIST_SCRIPT_INIT;
  scripts += CONTROLASSIST_SCRIPT_WEBSOCKETS_CLIENT;  
  scripts += CONTROLASSIST_SCRIPT_INIT_CONTROLS;
  scripts += "</script>";
  server.sendContent(scripts);
  server.sendContent(_html_body,strlen(_html_body));
  server.sendContent(_html_footer,strlen(_html_footer));
}
