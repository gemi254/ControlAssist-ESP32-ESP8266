#if defined(ESP32)
  #include <WebServer.h>
  WebServer server(80);
  #define ADC_PIN 36
#else
  #include <ESP8266WebServer.h>  
  ESP8266WebServer  server(80);  
  #define ADC_PIN A0
#endif

//Define application name
#define APP_NAME "ControlAssist-Simple"
#define INI_FILE "/ControlAssist-Simple.ini"

#define LOGGER_LOG_LEVEL 5
#include <controlAssist.h>  // Control assist class

unsigned long pingMillis = millis();  // Ping 
const char st_ssid[]=""; // Put connection info here
const char st_pass[]="";
char chBuff[128];
static bool buttonState = false;

ControlAssist ctrl; //Control assist class

PROGMEM const char HTML_BODY[] = R"=====(
<body>
<h1>Control Assist sample page</h1>
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
  
  //Connect WIFI
  LOG_E("Connect WIFI.\n");
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
  
  //Control assist setup
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.bind("span_ctrl");
  ctrl.bind("input_ctrl");
  ctrl.bind("text_ctrl");
  ctrl.bind("check_ctrl");  
  ctrl.bind("range_ctrl");
  ctrl.bind("button_ctrl");
  //Every time a variable changed changeHandler will be called   
  ctrl.setGlobalCallback(changeHandler);
  ctrl.begin();

  pinMode(ADC_PIN, INPUT);
#if defined(ESP32)
  analogReadResolution(12);
#endif  
}

void loop() {
  if(WiFi.status() != WL_CONNECTED ) return;
  //Change html control values
  if (millis() - pingMillis >= 3000){  
    ctrl.put("span_ctrl", analogRead(ADC_PIN) );
    ctrl.put("input_ctrl", String(ESP.getCycleCount()));
    
#if defined(ESP32)    
    ///ctrl.put("input_ctrl", String((temprature_sens_read() - 32) / 1.8 ) + " °C");
    sprintf(chBuff, "Memory Free: heap %u, block: %u, pSRAM %u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
#else
    sprintf(chBuff,"Memory free heap: %u, stack: %u ,block: %u", ESP.getFreeHeap(), ESP.getFreeContStack(), ESP.getMaxFreeBlockSize());
#endif    
    ctrl.put("text_ctrl",  chBuff);
    ctrl.put("check_ctrl", buttonState );
    ctrl.put("range_ctrl", WiFi.RSSI() );    
    ctrl.put("button_ctrl", buttonState );
    buttonState = !buttonState;
    pingMillis = millis();
  }
  ctrl.loop();
  server.handleClient();
}
