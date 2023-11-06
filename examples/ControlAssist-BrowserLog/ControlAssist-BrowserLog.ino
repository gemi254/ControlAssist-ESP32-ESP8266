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

PROGMEM const char CONTROLASSIST_HTML_BODY[] = R"=====(
<style>
:root { 
  --errColor: red;
  --warnColor: orange;
  --infoColor: green;
  --dbgColor: blue;
  --vrbColor: gray;
}
body{
  font-family: monospace;
  font-size: 0.7rem;
}
</style>
<body>
<input type="text" id="logLine" style="Display: None;">
<span id="logText"></span>
</body>
)=====";

PROGMEM const char CONTROLASSIST_HTML_SCRIPT[] = R"=====(
<script>
const logLine = document.getElementById("logLine"),
logText = document.getElementById("logText")
const root = getComputedStyle($(':root'));

// Event listener for web sockets messages
logLine.addEventListener("wsChange", (event) => {
  if(logText.innerHTML.length) logText.innerHTML += "<br>";
  let lines = event.target.value.split("<br>");
  if(lines.length > 1){
    for (i in lines){
      logText.innerHTML += toColor(lines[i] );
      if( i < lines.length - 1 ) logText.innerHTML += "<br>"
    }
  }else{
    logText.innerHTML += toColor( event.target.value );
  }
  
  if(isScrollBottom())
    window.scrollTo({ left: 0, top: document.body.scrollHeight, behavior: "smooth" });
});

const isScrollBottom = () => {
  let documentHeight = document.body.scrollHeight;
  let currentScroll = window.scrollY + window.innerHeight;
  let snap = 10; 
  if(currentScroll + snap > documentHeight) {
     return true;
  }
  return false;
}

const toColor = (line) => {
  // Color dbg line according to its type
  let colorVar = "";
  if (line.includes(" E @")) colorVar = "errColor";
  else if (line.includes("W @ ")) colorVar = "warnColor";
  else if (line.includes("I @ ")) colorVar = "infoColor";
  else if (line.includes("D @ ")) colorVar = "dbgColor";
  else if (line.includes("V @ ")) colorVar = "vrbColor";

  if (colorVar.length > 0) {
    const color = root.getPropertyValue('--' + colorVar);
    return "<font color=" + color + ">" + line + "</font>";
  } else return line;
}
</script>
</html>
)=====";

#define LOGGER_LOG_MODE  3                  // Set default logging mode using external function
void _log_printf(const char *format, ...);  // Custom log function
#define LOGGER_LOG_LEVEL 5
#include <ControlAssist.h>  // Control assist class

// Put connection info here. 
// On empty an AP will be started
const char st_ssid[]=""; 
const char st_pass[]="";
unsigned long pingMillis = millis();  // Ping 

ControlAssist ctrl; // Controler class

// Log print arguments
#define MAX_LOG_BUFFER_LEN 8192             // Maximum size of log buffer
#define MAX_LOG_FMT 256                     // Maximum size of log format
static String logBuffer="";
static char fmtBuf[MAX_LOG_FMT];
static char outBuf[512];
static va_list arglist;

// Custom log print function 
void _log_printf(const char *format, ...){
  strncpy(fmtBuf, format, MAX_LOG_FMT);
  fmtBuf[MAX_LOG_FMT - 1] = 0;
  va_start(arglist, format);  
  vsnprintf(outBuf, MAX_LOG_FMT, fmtBuf, arglist);
  va_end(arglist);
  Serial.print(outBuf);  
  if(ctrl.getClientsNum()>0){ // Is clients connected?
    if(logBuffer!=""){
      ctrl.put("logLine",logBuffer);
      logBuffer = "";
    } 
    ctrl.put("logLine",outBuf,true);
  }else{ // No clients store to logBuffer
    if(logBuffer.length()) logBuffer += "<br>";
    // if(logBuffer.length()) logBuffer += " (" + String(logBuffer.length()) +")<br>";
    logBuffer += String(outBuf);
  } 
  if(logBuffer.length() > MAX_LOG_BUFFER_LEN){
    int l = logBuffer.indexOf("<br>");
    logBuffer = logBuffer.substring(l+4, logBuffer.length()-1);
  }  
}

// Log debug info
void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_D("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_D("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  LOG_I("Starting..\n");  
  
  // Connect WIFI ?
  if(strlen(st_ssid)>0){
    LOG_D("Connect Wifi to %s.\n", st_ssid);
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
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_I("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());      
    if (MDNS.begin(hostName.c_str()))  LOG_V("AP MDNS responder Started\n");     
  }
  
  // Setup control assist
  ctrl.setHtmlBody(CONTROLASSIST_HTML_BODY);
  ctrl.setHtmlFooter(CONTROLASSIST_HTML_SCRIPT);
  ctrl.bind("logLine");
  ctrl.setup(server);
  ctrl.begin();
  LOG_V("ControlAssist started.\n");
  
  // Start webserver
  server.begin();
  LOG_V("HTTP server started\n");  
}

void loop() {
  if (millis() - pingMillis >= 1000){
    debugMemory("Loop");
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

