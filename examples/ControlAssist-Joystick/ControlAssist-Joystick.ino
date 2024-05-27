#if defined(ESP32)
  #include <ESPmDNS.h>
  #include <WebServer.h>
  #define WEB_SERVER WebServer
#else
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
  #define WEB_SERVER ESP8266WebServer
#endif

#define LOGGER_LOG_LEVEL 5            // Define log level for this module
#include <ControlAssist.h>            // Control assist class

const char st_ssid[]="";              // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";              // Put your wifi passowrd.
unsigned long pingMillis = millis();  // Ping millis

#include "joystickPMem.h"

ControlAssist ctrl;                   // Control assist class
WEB_SERVER server(80);                // Web server on port 80

// Change handlers
void xChange(){
  LOG_I("X pos: %lu\n", ctrl["x_coordinate"].toInt());
}
void yChange(){
  LOG_I("Y pos: %lu\n", ctrl["y_coordinate"].toInt());
}
void speedChange(){
  LOG_I("Speed: %lu %%\n", ctrl["speed"].toInt());
}
void angleChange(){
  LOG_I("Angle: %lu °\n", ctrl["angle"].toInt());

}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  LOG_I("Starting..\n");

  // Connect WIFI ?
  if(strlen(st_ssid)>0){
    LOG_E("Connect Wifi to %s.\n", st_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(st_ssid, st_pass);
    uint32_t startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000)  {
      Serial.print(".");
      delay(500);
      Serial.flush();
    }
    Serial.println();
  }

  // Check connection
  if(WiFi.status() == WL_CONNECTED ){
    LOG_I("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", st_ssid, WiFi.localIP().toString().c_str());
  }else{
    LOG_E("Connect failed.\n");
    LOG_I("Starting AP.\n");
    String mac = WiFi.macAddress();
    mac.replace(":","");
    String hostName = "ControlAssist_" + mac.substring(6);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_I("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
    if (MDNS.begin(hostName.c_str()))  LOG_V("AP MDNS responder Started\n");
  }

  // Control assist setup
  ctrl.setHtmlHeaders(HTML_HEADERS);
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.setHtmlFooter(HTML_FOOTER);

  // Bind span controls
  ctrl.bind("x_coordinate","0", xChange);
  ctrl.bind("y_coordinate","0", yChange);
  ctrl.bind("speed", speedChange);
  ctrl.bind("angle", angleChange);

  // Start web sockets
  ctrl.begin();
  LOG_I("ControlAssist started.\n");

  // Add a web server handler on url "/"
  server.on("/", []() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    String res = "";
    res.reserve(CTRLASSIST_STREAM_CHUNKSIZE);
    while( ctrl.getHtmlChunk(res)){
      server.sendContent(res);
    }
  });

  // Start web server
  server.begin();
  LOG_I("HTTP server started\n");
}

void loop() {
  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  ctrl.loop();
}