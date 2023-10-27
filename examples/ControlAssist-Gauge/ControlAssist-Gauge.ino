#if defined(ESP32)
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();
  #include <WebServer.h>
  WebServer server(80);  
  #include <ESPmDNS.h>  

#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>  
  ESP8266WebServer  server(80);
  #include <ESP8266mDNS.h>
  ADC_MODE(ADC_VCC);
#endif


#define LOGGER_LOG_LEVEL 5
#include <controlAssist.h>  // Control assist class

unsigned long pingMillis = millis();  // Ping 
const char st_ssid[]=""; // Put connection info here
const char st_pass[]="";
char chBuff[128];
static bool buttonState = false;
#define DELAY_MS 1000   //Measurements delay
ControlAssist ctrl;     //Control assist class

#include "gaugePMem.h"

void handleRoot(){
  ctrl.sendHtml(server);
}

//Change handler to handle websockets changes
void changeHandler(uint8_t no){
  String key = ctrl.getKey(no);
  if(key == "check_ctrl" ) 
    buttonState = ctrl["check_ctrl"].toInt();
  LOG_D("changeHandler: %i %s = %s\n",no, key.c_str(), ctrl[key].c_str());
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  LOG_I("Starting..\n");  
  
   //Connect WIFI?
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
  
  //Check connection
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
    if (MDNS.begin(hostName.c_str()))  LOG_I("AP MDNS responder Started\n");     
  }

  //Setup webserver
  server.on("/", handleRoot); 
  server.begin();
  LOG_I("HTTP server started\n");
  
  //Control assist setup
  ctrl.setHtmlHeaders(HTML_HEADERS);
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.bind("rssi");
  ctrl.bind("mem");
#if defined(ESP32)  
  ctrl.bind("temp");
  ctrl.bind("hall");  
#else
  ctrl.bind("vcc");
  ctrl.bind("hall");  
#endif  
  //Every time a variable changed changeHandler will be called   
  ctrl.setGlobalCallback(changeHandler);
  ctrl.begin();

}
#if defined(ESP32)
int getMemPerc(){
  uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
  float percentageHeapUsed = 100 - freeHeapBytes * 100.0f / (float)totalHeapBytes;
  return round(percentageHeapUsed);
}
#else
int getMemPerc(){
  uint32_t freeHeapBytes = ESP.getFreeHeap();  
  uint32_t totalHeapBytes = 96000; //1060000; //ESP.getFlashChipSizeByChipId();
  float percentageHeapUsed = 100 - freeHeapBytes * 100.0f / (float)totalHeapBytes;
  return round(percentageHeapUsed);
}
#endif
void loop() {
  //Update html control values
  if (millis() - pingMillis >= DELAY_MS){  
    ctrl.put("rssi", String( WiFi.RSSI()) );
    ctrl.put("mem", String( getMemPerc()) );    
#if defined(ESP32)    
    ctrl.put("temp", String( ((temprature_sens_read() - 32) / 1.8), 1 )); 
    ctrl.put("hall", String( hallRead()) );
#else
    ctrl.put("vcc", String( ESP.getVcc() ));    
#endif    
    buttonState = !buttonState;
    pingMillis = millis();
  }
  ctrl.loop();
  server.handleClient();
}
