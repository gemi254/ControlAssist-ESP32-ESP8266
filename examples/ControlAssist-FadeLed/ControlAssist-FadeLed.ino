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
#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif


ControlAssist ctrl; //Control assist class

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

void handleRoot(){
  ctrl.sendHtml(server);
}

void lampLevel(){
  LOG_D("lampLevel: %li\n",ctrl["lampLevel"].toInt());
  analogWrite(LED_BUILTIN, 255 - ctrl["lampLevel"].toInt());
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
  ctrl.setHtmlBody(HTML_BODY);
  ctrl.bind("lampLevel", lampLevel);      
  ctrl.begin();
  
  //Setup led
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 1024);
  LOG_I("Setup Lamp Led for ESP8266 board\n"); 
}

void loop() {
  ctrl.loop();
  server.handleClient();
}


