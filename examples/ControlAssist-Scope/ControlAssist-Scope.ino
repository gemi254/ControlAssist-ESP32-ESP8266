#if defined(ESP32)
  #include <ESPmDNS.h>
  #include <WebServer.h>
  #define WEB_SERVER WebServer
  #define ADC_PIN 36
#else
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
  #define WEB_SERVER ESP8266WebServer
  #define ADC_PIN A0
#endif

#define LOGGER_LOG_LEVEL 5            // Define log level for this module
#include <ControlAssist.h>            // Control assist class

const char st_ssid[]="";              // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";              // Put your wifi passowrd.
unsigned long pingMillis = millis();  // Ping millis
bool isPlaying = false;
unsigned long speed = 40;             // Read delay
static int adc_pos = -1;

#include "scopePMem.h"                // Program html code

ControlAssist ctrl;                   // Control assist class
WEB_SERVER server(80);                // Web server on port 80

// Web server root handler
void handleRoot(){
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  String res = "";
  res.reserve(CTRLASSIST_STREAM_CHUNKSIZE);
  while( ctrl.getHtmlChunk(res)){
    server.sendContent(res);
  }
  server.sendContent("");
}
// Change handlers
void changeOnOff(){
 LOG_V("changeOnOff  %li\n", ctrl["on-off"].toInt());
 isPlaying = ctrl["on-off"].toInt();
}
void speedChange(){
 LOG_V("speedChange  %s\n", ctrl["speed"].c_str());
 speed = ctrl["speed"].toInt();
}

// Setup function
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
  ctrl.setHtmlFooter(HTML_SCRIPT);
  // Bind controls
  ctrl.bind("on-off",isPlaying, changeOnOff);
  ctrl.bind("speed", speed, speedChange);
  // Auto send key values on connection
  ctrl.setAutoSendOnCon("on-off",true);
  ctrl.setAutoSendOnCon("speed",true);
  // Store key position on adc_val for speed
  // Only on last bind call the position will be valid!
  adc_pos = ctrl.bind("adc_val");
  // Start the server
  ctrl.begin();
  LOG_V("ControlAssist started.\n");

  server.on("/", handleRoot);
  // Dump binded controls handler
  server.on("/d", []() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.sendContent("Serial dump\n");
    String res = "";
    while( ctrl.dump(res) ){
      server.sendContent(res);
    }
  });

  server.begin();
  LOG_V("HTTP server started\n");
#if defined(ESP32)
  pinMode(ADC_PIN, INPUT);
#endif

  // Dump binded controls to serial
  String res;
  while(ctrl.dump(res)) Serial.print(res);
}

void loop() { // Run repeatedly

  if (millis() - pingMillis >= speed){
    // Set control at position to value
    if(isPlaying)
      ctrl.set(adc_pos, analogRead(ADC_PIN), true);
    pingMillis = millis();
  }

  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  ctrl.loop();
}


