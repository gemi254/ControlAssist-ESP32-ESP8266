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
const char st_pass[]="";              // Put your wifi password.
unsigned long pingMillis = millis();  // Ping millis

char chBuff[128];
static bool buttonState = false;

ControlAssist ctrl;                   // Control assist class
WEB_SERVER server(80);                // Web server on port 80

PROGMEM const char HTML_BODY[] = R"=====(
<body>
<h1>ControlAssist sample page</h1>
<table>
<tr>
  <td>ADC value</td>
  <td>Span control</td>
  <td><span id="span_ctrl">0</span></td>
</tr>
<tr>
  <td>Cycle count</td>
  <td>Input control</td>
  <td><input type="text" id="input_ctrl"></td>
</tr>
<tr>
  <td>Memory</td>
  <td>Text control</td>
  <td><textarea id="text_ctrl" name="text_ctrl" style="width: 371px; height: 34px;"></textarea></td>
</tr>
<tr>
  <td>Check Flag</td>
  <td>Check control</td>
  <td>
    <input title="Simple checkbox" id="check_ctrl" name="check_ctrl" type="checkbox">
  </td>
</tr>
<tr>
  <td>Wifi RSSI </td>
  <td>Range control</td>
  <td><input title="Range control" type="range" id="range_ctrl" name="range_ctrl" min="-120" max="0" value="0"></td>
</tr>
<tr>
  <td>User Button</td>
  <td>Button control</td>
  <td>
    <button type="button" title="Button 1" id="button_ctrl">Button 1</button>
  </td>
</tr>
</table>
</body>
)=====";

// Change handler to handle websockets changes
void changeHandler(uint8_t ndx){
  String key = ctrl[ndx].key;
  if(key == "check_ctrl" )
    buttonState = ctrl["check_ctrl"].toInt();
  LOG_D("changeHandler: ndx: %02i, key: %s = %s\n",ndx, key.c_str(), ctrl[key].c_str());
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


  // Control assist setup
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.bind("span_ctrl");
  ctrl.bind("input_ctrl");
  ctrl.bind("text_ctrl");
  ctrl.bind("check_ctrl");
  ctrl.bind("range_ctrl");
  ctrl.bind("button_ctrl");
  // Every time a variable changed changeHandler will be called
  ctrl.setGlobalCallback(changeHandler);

  // Start web sockets
  ctrl.begin();
  LOG_V("ControlAssist started.\n");
  // Setup webserver
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
  // Start webs server
  server.begin();
  LOG_V("HTTP server started\n");

  pinMode(ADC_PIN, INPUT);
}

void loop() {
  // Change html control values
  if (millis() - pingMillis >= 3000){
    // Update control assist variables
    ctrl.put("span_ctrl", analogRead(ADC_PIN) );
    ctrl.put("input_ctrl", String(ESP.getCycleCount()));
    ctrl.put("text_ctrl",  chBuff);
    ctrl.put("check_ctrl", buttonState );
    ctrl.put("range_ctrl", WiFi.RSSI() );
    ctrl.put("button_ctrl", buttonState );
#if defined(ESP32)
    //ctrl.put("input_ctrl", String((temprature_sens_read() - 32) / 1.8 ) + " °C");
    sprintf(chBuff, "Memory Free: heap %u, block: %u, pSRAM %u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
#else
    sprintf(chBuff,"Memory free heap: %u, stack: %u ,block: %u", ESP.getFreeHeap(), ESP.getFreeContStack(), ESP.getMaxFreeBlockSize());
#endif
    buttonState = !buttonState;
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