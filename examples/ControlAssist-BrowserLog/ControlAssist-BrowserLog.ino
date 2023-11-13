#if defined(ESP32)
  #include <WebServer.h>
  #include <ESPmDNS.h>  
  WebServer server(80);  
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>  
  #include <ESP8266mDNS.h>
  ESP8266WebServer  server(80);
#endif

#define LOGGER_LOG_MODE  3              // Set default logging mode using external function
#define LOGGER_LOG_LEVEL 5              // Define log level for this module
static void _log_printf(const char *format, ...);  // Custom log function, defined in weblogger.h
#include <ControlAssist.h>              // Control assist class
#include "remoteLogViewer.h"            // Web based remote log page using web sockets

RemoteLogViewer remoteLogView(85);      // The remote live log viewer page

const char st_ssid[]="";                // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                // Put your wifi passowrd.
unsigned long pingMillis = millis();    // Ping millis

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
  
  // Setup the remote web debugger in order to store log lines, url "/log"
  // When no connection is present store log lines in a buffer until connection
  remoteLogView.setup(server);

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
  
  // Start web lgo viewer sockets 
  remoteLogView.begin();  
  LOG_I("RemoteLogViewer started.\n"); 

  // Setup webserver  
  server.on("/", []() {
    server.send(200, "text/html", "<h1>This is root page</h1><br><a target='_new' href='/log'>View log</a>");
    remoteLogView.dump();
  });
  
  // Start webserver
  server.begin();
  LOG_V("HTTP server started\n");  
}

void loop() {
  if (millis() - pingMillis >= 2000){
    debugMemory("Loop");
    pingMillis = millis();
  }

  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  remoteLogView.loop();
}

