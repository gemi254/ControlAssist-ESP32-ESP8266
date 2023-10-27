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

#define LOGGER_LOG_LEVEL 5
#include <controlAssist.h>  // Control assist class

// Put connection info here. 
// On empty an AP will be started
const char st_ssid[]=""; 
const char st_pass[]="";
unsigned long pingMillis = millis();  // Ping 


ControlAssist ctrl; //Control assist class

//Handle web server root request and send html to client
void handleRoot(){
  ctrl.sendHtml(server);
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
  
  //Setup control assist
  ctrl.begin();
  ctrl.dump(); 
}

void loop() {
  ctrl.loop();
  server.handleClient();
}