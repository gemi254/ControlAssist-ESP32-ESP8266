#if defined(ESP32)
  #include <ESPmDNS.h>
  #include <WebServer.h>
  #define WEB_SERVER WebServer
#else
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
  #define WEB_SERVER ESP8266WebServer
#endif

const char st_ssid[]="";                // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                // Put your wifi passowrd.
unsigned long pingMillis = millis();    // Ping millis

#define LOGGER_LOG_MODE  3              // Set default logging mode using external function
#define LOGGER_LOG_LEVEL 5              // Define log level for this module
static void _log_printf(const char *format, ...);  // Custom log function, defined in weblogger.h

#define LOG_TO_FILE true                // Set to false to disable log file

#if (LOG_TO_FILE)
    #define LOGGER_OPEN_LOG()  if(!log_file) log_file = STORAGE.open(LOGGER_LOG_FILENAME, "a+")
    #define LOGGER_CLOSE_LOG() if(log_file) log_file.close()
#endif

#include <ControlAssist.h>              // Control assist class
#include "remoteLogViewer.h"            // Web based remote log page using web sockets

WEB_SERVER server(80);                  // Web server on port 80
RemoteLogViewer remoteLogView(85);      // The remote live log viewer page

uint32_t loopNo = 0;

void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_D("%s %i> Free: heap %u, block: %u, pSRAM %u\n", caller, ++loopNo, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_D("%s %i > Free: heap %u\n", caller, ++loopNo, ESP.getFreeHeap());
  #endif
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

#if (LOG_TO_FILE)
  if (!STORAGE.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }else{
    Serial.println("Storage statred.");
  }
  LOGGER_OPEN_LOG();
#endif

  // Setup the remote web debugger in order to store log lines, url "/log"
  // When no connection is present store log lines in a buffer until connection
  remoteLogView.setup();

  LOG_I("Starting..\n");
  // Connect WIFI ?
  if(strlen(st_ssid)>0){
    LOG_D("Connect Wifi to %s.\n", st_ssid);
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
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_I("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
    if (MDNS.begin(hostName.c_str()))  LOG_V("AP MDNS responder Started\n");
  }

  // Start web log viewer sockets
  remoteLogView.begin();
  LOG_I("RemoteLogViewer started.\n");

  // Setup webserver
  server.on("/", []() {
    server.send(200, "text/html", "<h1>This is root page</h1><br><a target='_new' href='/log'>View log</a>");
  });

  // Setup log handler
  server.on("/log", []() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    String res = "";
    while( remoteLogView.getHtmlChunk(res) ){
      server.sendContent(res);
    }
    server.sendContent("");
  });
#if (LOG_TO_FILE)
  // Setup log file vire handler
  server.on("/logFile", []() {
    if(server.hasArg("reset")){
      LOG_W("Reseting log\n");
      LOGGER_CLOSE_LOG();
      STORAGE.remove(LOGGER_LOG_FILENAME);
      server.sendContent("Reseted log");
      // Reopen log file to store log lines
      LOGGER_OPEN_LOG();
      return;
    }
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    LOGGER_CLOSE_LOG();
    // Open for reading
    File file = STORAGE.open(LOGGER_LOG_FILENAME, "r");
    // Send log file contents
    String res = "";
    while( file.available() ){
      res = file.readStringUntil('\n') + "\n";
      server.sendContent(res);
    }
    file.close();
    server.sendContent("");
    // Reopen log file to store log lines
    LOGGER_OPEN_LOG();
  });
#endif
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

