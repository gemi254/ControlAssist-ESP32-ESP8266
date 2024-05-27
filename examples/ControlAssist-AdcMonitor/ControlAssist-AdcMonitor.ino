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

#include "adcMonitorPmem.h"                 // Html page Program mem

const char st_ssid[]="";                    // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                    // Put your wifi passowrd.
unsigned long pingMillis = millis();        // Ping millis

unsigned long speed = 40;
bool isPlaying = false;
#ifdef ESP32
// ADC channel 1
// ADC channel 2 canot be used while WiFi is on, pins 27,26,25 15,14,13,12,3,2,0
int adcPins[] ={ 39, 38, 37, 36, 35, 34, 33, 32};
#else
int adcPins[] ={ 0 };
#endif

ControlAssist ctrl;                 // Control assist class
WEB_SERVER server(80);              // Web server on port 80

static int ctrlsNdx[ sizeof(adcPins) / sizeof(int) ]; // Array of control indexes

// Init adc ports
void initAdcadcPins(){
  for (byte i = 0; i < sizeof(adcPins) / sizeof(int); i++) {
    LOG_I("Init pin no: %i\n", adcPins[i]);
    pinMode(adcPins[i], INPUT);
    ctrl.bind( ("adc_" + String(adcPins[i])).c_str() );
  }
  // Get positions of the variables
  for (byte i = 0; i < sizeof(adcPins) / sizeof(int); i ++) {
    ctrlsNdx[i] = ctrl.getKeyNdx(("adc_" + String(adcPins[i])).c_str());
  }
}
// Read adc ports
void readAdcadcPins(){
  for (byte i = 0; i < sizeof(adcPins) / sizeof(int); i++) {
    uint16_t v = analogRead( adcPins[i]);
    ctrl.set(ctrlsNdx[i], v);
    LOG_N("Read pin no: %i = %u\n", adcPins[i], v);
  }
}
// Change handler
void speedChange(){
 LOG_V("speedChange  %s\n", ctrl["speed"].c_str());
 speed = ctrl["speed"].toInt();
}
// Change handler
void changeOnOff(){
 LOG_V("changeOnOff  %li\n", ctrl["on-off"].toInt());
 isPlaying = ctrl["on-off"].toInt();
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
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_I("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
    if (MDNS.begin(hostName.c_str()))  LOG_V("AP MDNS responder Started\n");
  }

  // Generate body html and JavaScripts
  ctrl.setHtmlHeaders(CONTROLASSIST_HTML_HEADER);
  static String htmlBody(CONTROLASSIST_HTML_BODY);
  String barsScripts="";
  for (byte i = 0; i < sizeof(adcPins) / sizeof(int); i = i + 1) {
    String bar(CONTROLASSIST_HTML_BAR);
    bar.replace("{key}", String(adcPins[i]));
    htmlBody += bar;

    String barScript(CONTROLASSIST_HTML_BAR_SCRIPT);
    barScript.replace("{key}", String(adcPins[i]));
    barsScripts += barScript;
  }

  // Setup control assist
  ctrl.setHtmlBody(htmlBody.c_str());
  static String htmlFooter(CONTROLASSIST_HTML_FOOTER);
  htmlFooter.replace("/*{SUB_SCRIPT}*/",barsScripts);
  ctrl.setHtmlFooter(htmlFooter.c_str());

  // Bind controls
  ctrl.bind("on-off", isPlaying, changeOnOff);
  ctrl.bind("speed", speed, speedChange);
  // Handle web server root request and send html to client
  // Auto send all previous vars on ws connection
  ctrl.setAutoSendOnCon(true);
  // Init and bind all pins
  initAdcadcPins();

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
  // Dump binded controls handler
  server.on("/d", []() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.sendContent("Serial dump\n");
    String res = "";
    while( ctrl.dump(res) ){
      server.sendContent(res);
    }
  });
   // Start webserver
  server.begin();
  LOG_I("HTTP server started\n");
}

void loop() {
  if (millis() - pingMillis >= speed){
    if(isPlaying) readAdcadcPins();
    pingMillis = millis();
  }

  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); //Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  ctrl.loop();
}

