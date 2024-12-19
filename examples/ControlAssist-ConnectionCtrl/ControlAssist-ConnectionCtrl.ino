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

const char st_ssid[]="";                 // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                 // Put your wifi passowrd.
unsigned long pingMillis = millis();     // Ping millis
unsigned long disconnMillis = millis();  // Wifi disconnect millis

ControlAssist ctrl;                   // Control assist class
WEB_SERVER server(80);                // Web server on port 80

time_t disconTime = 10 * 1000;         // Wifi disconnection duration

PROGMEM const char HTML_BODY[] = R"=====(
<style>
  body {text-align: center;}

  #conLed {
    width: 10px;
    height: 10px;
    background-color: lightGray;
    border-radius: 10px;
    margin-left: 95%;
  }
 #conLed.on {
    background-color: lightGreen;
    border: 1px solid #4CAF50;
  }

  th{ text-align: right;}
  </style>
  <body>
    <div style="text-align:left;display:inline-block;min-width:340px;">
      <div style="text-align:center;color:#eaeaea;">
        <h1>ControlAssist Websockets connection</h1>
      </div>
      <div>
        <table border="0" style="width: 100%;" cellspacing="10">
        <tr>
          <th>MacAddress</th>
          <td><span id="mac_address">AA:DD:CC:EE:FF:KK</span></td>
        </tr>
        <tr>
          <th>Wifi RSSI </th>
          <td><input title="Range control" type="range" id="wifi_rssi" name="wifi_rssi" min="-120" max="0" value="0"></td>
        </tr>
        <tr>
          <th>Disconnect</th>
          <td>
            <button type="button" title="Disconnect WiFi" id="disconnect_button">Disconnect</button>&nbsp;WiFi for
            <input type="text" title="Enter wifi disconnect seconds" id="sleep_time" value="" style="width: 30px">&nbsp;Seconds
          </td>
        </tr>
        <tr><td>&nbsp;</td></tr>
        <tr>
          <td><div id="conLed">&nbsp;</td>
          <td></div><span id="wsStatus" style="display: none1;">Disconnected</span></td>
        </tr>
        </table>
      </div>
  </div>
  </body>

  <script>
  document.getElementById("wsStatus").addEventListener("change", (event) => {
  conLed = document.getElementById("conLed")
  if(event.target.innerHTML.startsWith("Connected: ")){
    conLed.classList.add("on");
  }else{
    conLed.classList.remove("on");
  }
  conLed.title = event.target.innerHTML
});
</script>
)=====";

// Disconnect WiFi
void disconnect(){
  LOG_D("Disconnect WiFi for %s seconds\n", String(disconTime / 1000L).c_str());
  ctrl.put("wifi_rssi",  -120 );
  ctrl.loop();
  ctrl.sendSystemMsg("C");
  time_t s = millis();
  while(millis() - s < 100 )
    ctrl.loop();
  WiFi.disconnect();
  disconnMillis = millis();
}

// Change handler to handle websockets changes
void changeHandler(uint8_t ndx){
  String key = ctrl[ndx].key;
  if(key == "sleep_time" )
    disconTime = ctrl[key].toInt() * 1000;
  else if(key == "disconnect_button")
    disconnect();

  LOG_D("changeHandler: ndx: %02i, key: %s = %s\n",ndx, key.c_str(), ctrl[key].c_str());
}
// Connect WiFi
void connect(){
  if(WiFi.status() == WL_CONNECTED ) return;
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
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  LOG_I("Starting..\n");
  // Connect WiFi
  connect();

  // Control assist setup
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.bind("mac_address", WiFi.macAddress().c_str() );
  ctrl.bind("wifi_rssi");
  ctrl.bind("sleep_time", disconTime / 1000);
  ctrl.bind("disconnect_button");
  ctrl.setAutoSendOnCon(true);
  // Every time a variable changed changeHandler will be called
  ctrl.setGlobalCallback(changeHandler);

  // Start web sockets
  ctrl.begin();
  //ctrl.put("sleep_time", disconTime / 1000,true);
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

  // Start webs server
  server.begin();
  LOG_V("HTTP server started\n");
}

void loop() {
  // Change html control values
  if (millis() - pingMillis >= 3000){
    // Update control assist variables
    ctrl.put("wifi_rssi", WiFi.RSSI() );

    pingMillis = millis();
  }
  // Change html control values
  if ( WiFi.status() != WL_CONNECTED && millis() - disconnMillis >= disconTime){
    // Re connect
    connect();
    disconnMillis = millis();
  }
  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  ctrl.loop();
}