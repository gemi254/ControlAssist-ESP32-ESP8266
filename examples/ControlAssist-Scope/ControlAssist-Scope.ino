#if defined(ESP32)
  #include <WebServer.h>
  WebServer server(80);  
  #define ADC_PIN 36
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>  
  ESP8266WebServer  server(80);
  #define ADC_PIN A0
#endif

#define LOGGER_LOG_LEVEL 4
#include <controlAssist.h>  // Control assist class

unsigned long pingMillis = millis();  // Ping 
const char st_ssid[]=""; // Put connection info here
const char st_pass[]="";

static unsigned int speed = 40;
static int adc_pos = -1;

ControlAssist ctrl;

#include "scopePMem.h"
void handleRoot(){
  ctrl.sendHtml(server);
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
  
  //Connect WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(st_ssid, st_pass);
  uint32_t startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000)  {
    Serial.print(".");
    delay(500);
    Serial.flush();
  }  
  Serial.println();
  
  //Check connection
  if(WiFi.status() == WL_CONNECTED ){
    LOG_I("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", st_ssid, WiFi.localIP().toString().c_str()); 
  }else{
    LOG_E("Connect failed.\n");
    return;
  }
  
  //Setup webserver
  server.on("/", handleRoot);
  server.on("/d", []() {
    server.send(200, "text/plain", "Serial dump");
    ctrl.dump();           
  });  
  server.begin();
  LOG_I("HTTP server started\n");

  //Control assist setup
  ctrl.setHtmlHeaders(HTML_HEADERS);
  ctrl.setHtmlBody(HTML_BODY);  
  ctrl.setHtmlFooter(HTML_SCRIPT);
  ctrl.bind("speed", speedChange);
  ctrl.bind("adc_val");
  //Store key position
  adc_pos = ctrl.getKeyPos("adc_val");
  
  ctrl.begin();
  ctrl.dump();

#if defined(ESP32)
  pinMode(ADC_PIN, INPUT);
  //analogReadResolution(10);
#endif
}

void loop() {
  if(WiFi.status() != WL_CONNECTED ) return;
  
  //Handler webserver clients
  server.handleClient();
  //Handle websockets
  ctrl.loop();

  // Run repeatedly:
  if (millis() - pingMillis >= speed){
    //Set control at position to value  
    ctrl.set(adc_pos, analogRead(ADC_PIN), true);
    pingMillis = millis();
  }
}


