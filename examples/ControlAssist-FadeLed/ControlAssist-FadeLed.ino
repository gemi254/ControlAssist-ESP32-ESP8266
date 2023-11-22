#define LOGGER_LOG_LEVEL 5            // Define log level for this module
#include <ControlAssist.h>            // Control assist class

#if defined(ESP32)
  WebServer server(80);  
  #include <ESPmDNS.h>  
#else
  ESP8266WebServer  server(80);
  #include <ESP8266mDNS.h>
#endif

const char st_ssid[]="";              // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";              // Put your wifi passowrd.
unsigned long pingMillis = millis();  // Ping millis

#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

int ledLevel = 0;
ControlAssist ctrl;                   // Control assist class

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
  margin: 10px;
}

#led {  
  width: calc(var(--elmSize) * 1.3);
  height: calc(var(--elmSize) * 1.3);
  #background-color: lightGray;
  background-color: blue;
  opacity: .1;
  border-radius: calc(var(--elmSize) * 1.3);
}

.slider {
  position: relative;
  display: inline-block;
}

.title {
  font-weight:900;
  font-size: 1.2rem;
  padding: 10px;
}

</style>
<body>
<div class="container">
    <h1>Control Assist sample page</h1>
</div>    
<div class="container">
    <div class="title">Fade led</div>
</div>    
<div class="container">
    <div class="slider">
      <input title="Control onboard lamp led brightness" type="range" id="lampLevel" min="0" max="255" value="0">
    </div>
</div>
<div class="container">
    <label for="lamp">Level: <span id="lampLevelValue">0</span></label>
</div>
<div class="container">
    <div id="led"></div>
</div>

<script>
const led = document.getElementById("led"),
      lampLevel = document.getElementById("lampLevel"),
      lampLevelValue = document.getElementById("lampLevelValue")
   
const fadeLed = (val) => {
  lampLevelValue.innerHTML = val;
  led.style.opacity = val / 255 + .05;
}
//Change listener    
lampLevel.addEventListener("change", (event) => {
    fadeLed(event.target.value)
});
//Mouse mode listener
lampLevel.addEventListener("input", (event) => {
    fadeLed(event.target.value)
});
//Websockets listener
lampLevel.addEventListener("wsChange", (event) => {
    fadeLed(event.target.value)
});
</script>
</body>
</html>
)=====";
// Set led brightness
void lampLevel(){
  ledLevel = ctrl["lampLevel"].toInt();
  LOG_D("lampLevel: %i\n", ledLevel);
  analogWrite(LED_BUILTIN, 255 - ledLevel);
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
  ctrl.bind("lampLevel", ledLevel, lampLevel);
  // Auto send on connect
  ctrl.setAutoSendOnCon("lampLevel", true);
  // Add a web server handler on url "/"
  ctrl.setup(server);
  // Start web sockets  
  ctrl.begin();
  LOG_V("ControlAssist started.\n");

  // Setup webserver
  server.begin();
  LOG_V("HTTP server started\n");

  // Setup led
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 1024);
  LOG_I("Setup Lamp Led for ESP8266 board\n"); 
}

void loop() {
  #if not defined(ESP32)
    if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
  #endif
  // Handler webserver clients
  server.handleClient();
  // Handle websockets
  ctrl.loop();;
}


