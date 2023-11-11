#if defined(ESP32)
  #include <WebServer.h>
  WebServer server(80);  
  #define ADC_PIN 36
  #include <ESPmDNS.h>  
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>  
  ESP8266WebServer  server(80);
  #define ADC_PIN A0
  #include <ESP8266mDNS.h>
#endif

#define LOGGER_LOG_LEVEL 5
#include <ControlAssist.h>            // Control assist class

const char st_ssid[]="";              // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";              // Put your wifi passowrd.
unsigned long pingMillis = millis();  // Ping millis
bool isPlaying = false;
unsigned long speed = 40;             // Read delay
static int adc_pos = -1;

ControlAssist ctrl;

#include "scopePMem.h"
void handleRoot(){
  ctrl.sendHtml(server);
}
// Change handler
void changeOnOff(){
 LOG_V("changeOnOff  %li\n", ctrl["on-off"].toInt());
 isPlaying = ctrl["on-off"].toInt();
}
void speedChange(){
 LOG_V("speedChange  %s\n", ctrl["speed"].c_str());
 speed = ctrl["speed"].toInt();
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
  // Add a web server handler on url "/"
  ctrl.setup(server);
  // Start the server
  ctrl.begin();
  ctrl.dump();
  LOG_V("ControlAssist started.\n");

  // Setup webserver  
  server.on("/d", []() {
    server.send(200, "text/plain", "Serial dump");
    ctrl.dump();           
  });  
  server.begin();
  LOG_V("HTTP server started\n");
#if defined(ESP32)
  pinMode(ADC_PIN, INPUT);
#endif
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


