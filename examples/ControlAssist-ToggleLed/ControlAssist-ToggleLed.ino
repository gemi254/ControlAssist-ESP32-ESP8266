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

const char st_ssid[]="";                // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                // Put your wifi passowrd.
unsigned long pingMillis = millis();    // Ping millis

bool ledState = false;
#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

ControlAssist ctrl;                 // Control assist class
WEB_SERVER server(80);              // Web server on port 80

PROGMEM const char HTML_BODY[] = R"=====(
<style>
:root {
--toggleActive: lightslategrey;
--toggleInactive: lightgray;
--sliderText: black;
--sliderThumb: lightgray;
--elmSize: 1.5rem;
--elmQuart: calc(var(--elmSize) / 4);
}
html, body {
	margin: 0;
	width: 100%;
	font-family: Arial;
}
.container{
  display: flex;
  justify-content: center;
}
#led {
  margin-left: 20px;
  width: calc(var(--elmSize) * 1.3);
  height: calc(var(--elmSize) * 1.3);
  background-color: lightGray;
  border-radius: calc(var(--elmSize) * 1.3);
}
#led.on {
	background-color: blue;
}

#conLed {
  margin-left: 20px;
  width: calc(var(--elmSize) * 0.4);
  height: calc(var(--elmSize) * 0.4);
  background-color: lightGray;
  border-radius: calc(var(--elmSize) * 0.4);
  position: relative;
  margin-left: 50%;
}

.switch {
  position: relative;
  display: inline-block;
  width: calc(var(--elmSize) * 1.9);
  height: calc(var(--elmSize) * 1.3);
}
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}
.switch-label {
  font-weight:900;
  font-size: 1.2rem;
  padding: 10px;
}
.slider {
  position: absolute;
  border-radius: var(--elmSize);
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: var(--toggleInactive);
  width: calc(var(--elmSize) * 2.3);
  transition: .4s;
}
.slider:before {
  position: absolute;
  border-radius: 50%;
  content: "";
  height: var(--elmSize);
  width: var(--elmSize);
  left: var(--elmQuart);
  top: calc(var(--elmQuart)*.5);
  background: WhiteSmoke;
  transition: .4s;
}
input:checked + .slider {
  background-color: var(--toggleActive);
}
input:focus + .slider {
  outline: auto;
}
input:checked + .slider:before {
  transform: translateX(calc(var(--elmSize) * 0.9));
}

.bottom {
    position: absolute;
    bottom: 5px;
    width: 100%;
    text-align: center;
}
</style>
<body>
    <div class="container">
    <h1>ControlAssist example page</h1>
</div>
<div class="container">
    <div class="switch-label">Toggle led</div>
</div>
<div class="container">
    <div class="switch">
        <input id="toggleLed" name="toggleLed" type="checkbox">
        <label class="slider" for="toggleLed"></label>
    </div>
    <div id="led"></div>
</div>
<br>
<div class="bottom">
  <div id="conLed" class="center"></div>
  <span id="wsStatus" style="display: none;"></span>
</div>
<script>
const led = document.getElementById("led"),
      toggleLed = document.getElementById("toggleLed");

const changeLed = (v) => {
  if(!v) led.classList.remove("on");
  else led.classList.add("on");
}

toggleLed.addEventListener("change", (event) => {
  changeLed(event.target.checked);
});

toggleLed.addEventListener("wsChange", (event) => {
  changeLed(event.target.checked);
});

document.getElementById("wsStatus").addEventListener("change", (event) => {
  if(event.target.innerHTML.startsWith("Connected: ")){
    event.target.style.color = "lightgreen"
    conLed.style.backgroundColor  = "lightgreen"
  }else if(event.target.innerHTML.startsWith("Error:")){
    event.target.style.color = "lightred"
    conLed.style.backgroundColor  = "lightred"
  }else{
    event.target.style.color = "lightgray"
    conLed.style.backgroundColor  = "lightgray"
  }
  conLed.title = event.target.innerHTML
});
</script>
</body>
)=====";

void ledChangeHandler(){
  ledState = ctrl["toggleLed"].toInt();
  LOG_D("ledChangeHandler state: %i\n", ledState);
  toggleLed(ledState);
}

void toggleLed(bool ledState){
  if( ledState ){
    digitalWrite(LED_BUILTIN, LOW); // Turn LED ON
    LOG_D("Led on\n");
  }else{
    digitalWrite(LED_BUILTIN, HIGH); // Turn LED OFF
    LOG_D("Led off\n");
  }
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
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_I("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
    if (MDNS.begin(hostName.c_str()))  LOG_V("AP MDNS responder Started\n");
  }

  // Setup control assist
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.bind("toggleLed", ledState, ledChangeHandler);
  // Auto send on connect
  ctrl.setAutoSendOnCon("toggleLed", true);
  ctrl.begin();
  String res = "";
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

  // Start webserver
  server.begin();
  LOG_V("HTTP server started\n");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Turn LED OFF

  // Dump binded controls to serial
  while(ctrl.dump(res)) Serial.print(res);

}


void loop() {
  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  ctrl.loop();

  if (millis() - pingMillis >= 5000){
    ledState = !ledState;
    toggleLed(ledState);
    // Set the ledState and send a websocket update
    ctrl.put("toggleLed", ledState );
    pingMillis = millis();
  }
}


