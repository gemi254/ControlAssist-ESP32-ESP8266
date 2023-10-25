#if defined(ESP32)
  #include <WebServer.h>
  WebServer server(80);
#else
  #include <ESP8266WebServer.h>  
  ESP8266WebServer  server(80);
#endif

#define LOGGER_LOG_LEVEL 5
#include <controlAssist.h>  // Control assist class

unsigned long pingMillis = millis();  // Ping 
const char st_ssid[]=""; // Put connection info here
const char st_pass[]="";

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

  //Connect
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