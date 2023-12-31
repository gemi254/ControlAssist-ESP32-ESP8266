#define LOGGER_LOG_LEVEL 5            // Define log level for this module
#include <ControlAssist.h>            // Control assist class

#if defined(ESP32)
  WebServer server(80);  
  #include <ESPmDNS.h>  
#else
  ESP8266WebServer  server(80);
  #include <ESP8266mDNS.h>
#endif

const char st_ssid[]="";                // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                // Put your wifi passowrd.
unsigned long pingMillis = millis();    // Ping millis

bool ledState = false;
#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif
ControlAssist ctrl;                     // Control assist class

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
</style>
<body>
    <div class="container">
    <h1>Control Assist sample page</h1>
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
<script>
const led = document.getElementById("led"),
      toggleLed = document.getElementById("toggleLed")

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

 
  // Setup control assist
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.bind("toggleLed", ledState, ledChangeHandler);
  // Auto send on connect
  ctrl.setAutoSendOnCon("toggleLed", true);
  // Add a web server handler on url "/"
  ctrl.setup(server);
  ctrl.begin();
  ctrl.dump(); 
  LOG_V("ControlAssist started.\n");
  
  server.on("/d", []() { // Dump controls
    server.send(200, "text/plain", "Serial dump");
    ctrl.dump();
  });

  // Start webserver  
  server.begin();
  LOG_V("HTTP server started\n");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Turn LED OFF
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


