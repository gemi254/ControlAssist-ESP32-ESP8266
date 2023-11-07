#if defined(ESP32)
  #include <WebServer.h>
  WebServer server(80);  
  #include <ESPmDNS.h>  
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>  
  ESP8266WebServer  server(80);
  #include <ESP8266mDNS.h>
#endif

#define LOGGER_LOG_MODE  3    // Set default logging mode using external function
#define LOGGER_LOG_LEVEL 5    // Define log level for this module
void _log_printf(const char *format, ...);  // Custom log function, defined in weblogger.h
#include <ControlAssist.h>    // Control assist class
#include "webLogger.h"        // Web based log using web sockets

WebLogger weblog(84);         // The logger class on port 84 (!Must named weblog)

// Put your connection credentials here. 
// On empty an AP will be started
const char st_ssid[]="mdk3"; 
const char st_pass[]="2843028858";
unsigned long pingMillis = millis();  // Ping 

// Log debug info
void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_D("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_D("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  LOG_I("Starting..\n");  
  
  // Connect WIFI ?
  if(strlen(st_ssid)>0){
    LOG_D("Connect Wifi to %s.\n", st_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(st_ssid, st_pass);
    uint32_t startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000)  {
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
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_I("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());      
    if (MDNS.begin(hostName.c_str()))  LOG_V("AP MDNS responder Started\n");     
  }
  
  // Setup a weblogger on url "/log"
  weblog.setup(server);
  
  // Setup webserver  
  server.on("/", []() {
    server.send(200, "text/html", "<h1>This is root page</h1><br><a target='_new' href='/log'>View log</a>");
    weblog._ctrl.dump();
  });
  
  // Start webserver
  server.begin();
  LOG_V("HTTP server started\n");  
}

void loop() {
  if (millis() - pingMillis >= 1000){
    debugMemory("Loop");
    pingMillis = millis();
  }

  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  weblog.loop();
}

