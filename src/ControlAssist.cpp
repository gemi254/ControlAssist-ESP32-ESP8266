#include "ControlAssist.h"     // Control assist class
#include "controlAssistPMem.h" // Memory static valiables (html pages)

ControlAssist::ControlAssist() { 
  _server = NULL;
  _socket = NULL; 
  _wsEnabled = false; 
  _ev = NULL;
  _html_headers = CONTROLASSIST_HTML_HEADER;
  _html_body =  CONTROLASSIST_HTML_BODY;
  _html_footer = CONTROLASSIST_HTML_FOOTER;
  _html_headers_file = NULL;
  _html_body_file =  NULL;
  _html_footer_file = NULL;
  
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
// Setup webserver handlers
void ControlAssist::setup(WEB_SERVER &server, const char *uri){
  _server = &server;
  server.on(uri, [this] { this->sendHtml(); } );
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
  if(_socket) return;
  _socket = new WebSocketsServer(_port);
  _socket->onEvent( std::bind(&ControlAssist::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4) );  
  _socket->begin();
  LOG_I("Started web sockets at port: %u\n", _port);
}

// Stop the websocket server
void ControlAssist::stopWebSockets(){
  if(_socket == NULL) return;
  _socket->close();
  delete _socket;
  _socket = NULL;
  _clientsNum = 0;
  LOG_I("Stoped web sockets at port: %u\n", _port);
}

// Get the val of a given key, Empty on not found
String ControlAssist::getVal(String key) {
  int keyNdx = getKeyNdx(key);
  if (keyNdx >= 0) {
    return _ctrls[keyNdx].val;        
  }
  return String(""); 
}

// Add vectors by key (name in confPairs)
size_t ControlAssist::add(String key, String val){          
  ctrlPairs d = { key, "", NULL, false };
  return add(d);
}

// Add vectors pairs
size_t ControlAssist::add(ctrlPairs &c){
  _ctrls.push_back( {c} );
  _keysNdx.push_back( {c.key, _keysNdx.size()  } );
  sort();
  LOG_V("Added key: %s\n", c.key.c_str());  
  return _ctrls.size();
}

// Set the val at pos, (int)
bool ControlAssist::set(int keyNdx, int val, bool forceSend) {
  return set(keyNdx, String(val), forceSend);
}

// Set the val at pos, (string)
bool ControlAssist::set(int keyNdx, String val, bool forceSend) {
  bool changed = false;
  if (keyNdx >= 0 && (size_t)keyNdx < _ctrls.size() ) {
    if( _ctrls[keyNdx].val != val){
      LOG_V("Set [%i] = %s\n", keyNdx, val.c_str()); 
      _ctrls[keyNdx].val = val;
      changed = true;
    }
  }else{
    LOG_W("Set invalid ndx: [%i] = %s\n", keyNdx, val.c_str());
    return false;    
  } 
  // Send message to client
  if(_clientsNum > 0 && (forceSend || changed)){
    String payload = String(keyNdx + 1) + "\t" + val;
    if(_socket) _socket->broadcastTXT(payload);
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
  int keyNdx = getKeyNdx(key);  
  bool changed = false;
  if (keyNdx >= 0 && (size_t)keyNdx < _ctrls.size() ) {
    if( _ctrls[keyNdx].val != val){
      LOG_V("Put [%i] = %s\n", keyNdx, val.c_str()); 
      _ctrls[keyNdx].val = val;
      changed = true;
    }
  }else if(forceAdd) {
    keyNdx = add(key, val);
    changed = true;
  }else{
    LOG_W("Put failed on key: %s,  val: %s\n", key.c_str(), val.c_str());
    return false;
  }
  //Send message to client
  if(_clientsNum > 0 && (forceSend || changed)){
    String payload = String(keyNdx + 1) + "\t" + val;
    if(_socket) _socket->broadcastTXT(payload);
    LOG_V("Send payload: %s\n", payload.c_str());
  } 
  return true;
}

// Set the auto send on ws connection flag on key
bool ControlAssist::setAutoSendOnCon(String key, bool send) {
  int keyNdx = getKeyNdx(key);
  if (keyNdx >= 0 && (size_t)keyNdx < _ctrls.size() ) {
    _ctrls[keyNdx].autoSendOnCon = send;
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
void ControlAssist::autoSendKeys(uint8_t num){
  uint8_t row = 0;
  while (row++ < _ctrls.size()) { 
    LOG_V("Checking row: %s ,%i\n", _ctrls[row - 1].key.c_str(), _ctrls[row - 1].autoSendOnCon );
    if(_ctrls[row - 1].autoSendOnCon){
      String payload = String(row) + "\t" + _ctrls[row - 1].val;
      if(_socket) _socket->broadcastTXT(payload);
      LOG_D("Auto send to: %s, client num: %u, payload: %s\n",_ctrls[row - 1].key.c_str(), num, payload.c_str());
    }
  }
}

// Get the location of given key to retrieve control
int ControlAssist::getKeyNdx(String key) {
  if (_keysNdx.empty() || key == "") return -1; 
  //LOG_I("getKeyNdx, key: %s\n",key.c_str()); 
  auto lower = std::lower_bound(_keysNdx.begin(), _keysNdx.end(), key, [](
    const ctrlsNdx &a, const String &b) { 
    return a.key < b;}
  );
  int keyNdx = std::distance(_keysNdx.begin(), lower); 
  if (_keysNdx[keyNdx].ndx < _ctrls.size() && key == _ctrls[ _keysNdx[keyNdx].ndx ].key) return _keysNdx[keyNdx].ndx;
  else LOG_V("Key %s not exist\n", key.c_str()); 
  return -1; // not found
}

// Return next key and val from configs on each call in key order
bool ControlAssist::getNextPair(ctrlPairs &c) {
  static uint8_t row = 0;
  if (row++ < _ctrls.size()) { 
    c = _ctrls[row - 1];
    return true;
  }
  // end of vector reached, reset
  row = 0;
  return false;
}

// Sort vectors by key (name in logKeysNdx)
void ControlAssist::sort(){
  std::sort(_keysNdx.begin(), _keysNdx.end(), [] (
    const ctrlsNdx &a, const ctrlsNdx &b) {
    return a.key < b.key;}
  );
}

// Dump config items
void ControlAssist::dump(WEB_SERVER *server){
  char outBuff[256];  
  strcpy(outBuff, "Dump binded controls\n");  
  if(server){
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->sendContent(String(outBuff));    
  }else{
    LOG_I("%s", outBuff);
  }
  ctrlPairs c; 
  uint8_t ndx = 0;
  while (getNextPair(c)){
    int len =  sprintf(outBuff, "Ndx: %02u, chn: %02u, autoSend: %i, '%s': %s \n", ndx, ndx + 1,  c.autoSendOnCon, c.key.c_str(), c.val.c_str() );
    if(server) server->sendContent(outBuff, len);
    else LOG_I("%s", outBuff);
    ndx++;
  }

  strcpy(outBuff, "Dump indexes: \n");
  if(server) server->sendContent("\n" + String(outBuff));    
  else LOG_I("%s", outBuff);  
  size_t i = 0;  
  while( i < _keysNdx.size() ){
      int len =  sprintf(outBuff, "No: %02i, ndx: %02i, key: %s\n", i, _keysNdx[i].ndx, _keysNdx[i].key.c_str());      
      if(server) server->sendContent(outBuff, len);
      else LOG_I("%s", outBuff);
      i++;
  } 
}

// Bind a html control with id = key to a control variable
int ControlAssist::bind(const char* key){
  return bind(key, "");
}

// Bind a html control with id = key and local val to a control variable
int ControlAssist::bind(const char* key, const int val){  
  return bind(key, String(val).c_str(), NULL);
}

// Bind a html control with id = key and local val to a control variable
int ControlAssist::bind(const char* key, const char* val){
  return bind(key, val, NULL);
}

// Bind a html control with id = key to a local variable and an event function
int ControlAssist::bind(const char* key, WebSocketServerEvent ev){
  return bind(key, "", ev);
}
// Bind a html control with id = key and default int val to a local variable with an event function
int ControlAssist::bind(const char* key, const int val, WebSocketServerEvent ev){
   return bind(key, String(val).c_str(), ev);
}

// Bind a html control with id = key and default char val to a local variable with an event function
int ControlAssist::bind(const char* key, const char* val, WebSocketServerEvent ev){
  int p = getKeyNdx(key);
  if(p >= 0){
    LOG_E("Bind key: %s failed. Already exists in pos %i!\n", key,  p);
    return -1;
  }  
  ctrlPairs ctrl = { key, val, ev, false };
  LOG_D("Binding key: '%s', val:  %s, chn: %02i\n", key, val, _ctrls.size() + 1); 
  add(ctrl);
  return getKeyNdx(key);
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
          LOG_E("Client no: %02u Disconnected!\n", num);
          if(_clientsNum>0) _clientsNum--;
          break;
      case WStype_CONNECTED:
          {
            IPAddress ip = _socket->remoteIP(num);
            LOG_I("Client no: %02u connect from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            _clientsNum++;
            // Send message to client
            _socket->sendTXT(num, "0\tCon ");
            // Send keys selected
            autoSendKeys(num);
          }
          break;
      case WStype_TEXT:
          {
            uint8_t * pload = payload;
            // Get index first 3 max bytes until tab
            char chn[3];
            size_t i = 0;
            while((char)pload[i] != '\t' && i < 3){
              chn[i]=( (char)pload[i] );
              i++;
              if(i >= length) break;
            }
            if(i < length) pload += i + 1;
            
            uint8_t ndx = (uint8_t)atoi(chn);
            String val = reinterpret_cast<char *>(pload);

            LOG_N("Msg ndx: %u, val: %s\n", ndx, val.c_str());

            if(ndx == 0 ){ //System message
              if(val == "C"){ // Close connection message
                LOG_W("Received close connection msg. \n");
                _socket->disconnect();
              }else{
                LOG_V("Sys msg: %s \n",val.c_str());
              }
              return;
            }

            // Zero based index, index 0 is for system messages
            ndx = ndx - 1;
            if(ndx >= _ctrls.size() ) return;

            // Change val
            _ctrls[ndx].val = String(val.c_str());
   
            // Call control's change handler
            if(_ctrls[ndx].ev) {
              _ctrls[ndx].ev();
            }
            // Call global change handler
            if(_ev){
              _ev(ndx);
            }
            LOG_V("Ctrl msg, client: %02u, ndx: %02u, key: '%s', val: %s\n", num, ndx, _ctrls[ndx].key.c_str(), val.c_str());
            // Update all other connected clients
            if( _clientsNum > 1 ){
              for(uint8_t i = 0; i < _clientsNum; ++i){
                if( i != num ) _socket->sendTXT(i, payload);
              }
            }
          }
          break;
      case WStype_BIN:
          LOG_D("Client no: %02u, get binary length: %u\n", num, length);
          // send message to client
          // webSocket.sendBIN(num, payload, length);
          break;
      case WStype_ERROR:			
      case WStype_FRAGMENT_TEXT_START:
      case WStype_FRAGMENT_BIN_START:
      case WStype_FRAGMENT:
      case WStype_FRAGMENT_FIN:
        LOG_E("Client %02u, WS error: %x\n", num, type);
        break;
    }
}

// Build the initialization java script 
String ControlAssist::getInitScript(){ 
 String ctlPort = "const port = " + String(_port)+";";
 String keysToNdx = "const keysToNdx = { ";
 String ndxTokeys = "const ndxTokeys = { "; 
 for(uint8_t no = 0; no< _ctrls.size(); no++) {     
    keysToNdx  += "'" + String(_ctrls[no].key) + "': " + no + ", ";
    ndxTokeys  += "'" + String(no) + "': '" + String(_ctrls[no].key) + "', ";    
  }
  if(keysToNdx.endsWith(", ")) keysToNdx = keysToNdx.substring(0, (keysToNdx.length() - 2));
  if(ndxTokeys.endsWith(", ")) ndxTokeys = ndxTokeys.substring(0, (ndxTokeys.length() - 2));
  keysToNdx += "};";
  ndxTokeys += "};";
  return ctlPort + "\n" + keysToNdx + "\n" +  ndxTokeys + "\n";
}

// Load a file into a string
bool ControlAssist::loadText(String fPath, String &txt){
  File file = STORAGE.open(fPath, "r");
  if (!file || !file.size()) {
    LOG_E("Failed to load: %s, sz: %u B\n", fPath.c_str(), file.size());
    return false;
  }
  //Load text from file
  txt = "";
  while (file.available()) {
    txt += file.readString();
  } 
  LOG_D("Loaded: %s, sz: %u B\n" , fPath.c_str(), txt.length() );          
  file.close();
  return true;
}

// Send a file from spiffs to client    
void ControlAssist::streamFile(WEB_SERVER &server, const char *htmlFile){
  if(!STORAGE.exists(htmlFile)){
    LOG_E("Storage file missing: %s\n", htmlFile);
    server.sendContent("<p>Storage data file missing: <b>" + String(htmlFile) + "</b></p>");
    server.sendContent("<p>Please upload this file to ESP storage using `Sketch Data Upload` from your Arduino IDE</p>");
    return;
  }
  #if defined(ESP32)
  File f = STORAGE.open(htmlFile);
  #else
  File f = STORAGE.open(String(htmlFile),"r");
  #endif 
  byte chunk[STREAM_CHUNKSIZE];
  size_t chunksize;
  do {
      chunksize = f.read(chunk, STREAM_CHUNKSIZE);
      if(chunksize > 0) 
        server.sendContent((char *)chunk, chunksize);
  } while (chunksize != 0);
  f.close(); 
}

// Render html to client
void ControlAssist::sendHtml(WEB_SERVER &server){
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  if(_html_headers_file){
    streamFile(server, _html_headers_file);
  }else{
    server.sendContent(_html_headers, strlen(_html_headers));
  }

  String definitions = getInitScript();
  LOG_V("Script %s\n", definitions.c_str());
  String scripts = "<script type=\"text/javascript\">\n";
  scripts += definitions;
  scripts += CONTROLASSIST_SCRIPT_INIT;
  scripts += CONTROLASSIST_SCRIPT_WEBSOCKETS_CLIENT;  
  scripts += CONTROLASSIST_SCRIPT_INIT_CONTROLS;
  scripts += "</script>";
  server.sendContent(scripts); 

  if(_html_body_file){        
    streamFile(server, _html_body_file);
  }else{
    server.sendContent(_html_body, strlen(_html_body));
  }
  
  if(_html_footer_file){
    streamFile(server, _html_footer_file);    
  }else{
    server.sendContent(_html_footer, strlen(_html_footer));
  }
  server.sendContent(F("")); // this tells web client that transfer is done
}
