
#if defined(ESP32)
  #include <ESPmDNS.h>
  #include <WebServer.h>
  #include "SPIFFS.h"
  #define STORAGE SPIFFS    // Storage SPIFFS
  #define WEB_SERVER WebServer
#else
  #include <ESP8266WebServer.h>
  #include <LittleFS.h>
  #include <ESP8266mDNS.h>
  #define STORAGE LittleFS  // Storage LittleFS
  #define WEB_SERVER ESP8266WebServer
#endif

// Setting to true will need to upload contents of /data
// directory to esp SPIFFS using image upload
#define USE_SPIFFS_FOR_PAGES false

#if defined(ESP32)
  #if not USE_SPIFFS_FOR_PAGES
    #define BODY_FILE_NAME "/src/ESPWroom32-Vis.html"
  #else
    #include "gpioPMemESP32.h"
  #endif
  #define TOTAL_PINS 40
#else
  #if not USE_SPIFFS_FOR_PAGES
    #include "gpioPMemESP8266.h"
  #else
    #define BODY_FILE_NAME "/src/ESP8266Wemos-Vis.html"
    //#define BODY_FILE_NAME "/src/test.html"
  #endif
  #define TOTAL_PINS 17
#endif

#define LOGGER_LOG_LEVEL 5
#include <ControlAssist.h>                  // Control assist class

WEB_SERVER server(80);
ControlAssist ctrl(81);                     // Web socket control on port 81

const char st_ssid[]="";                    // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                    // Put your wifi passowrd.
unsigned long pingMillis = millis();        // Ping millis

#ifndef LED_BUILTIN
  #define LED_BUILTIN 22                      // Define the pin tha the led is connected
#endif

// Toggle led on/off and send pin update
void setLed(bool ledState){
  if( ledState ){
    digitalWrite(LED_BUILTIN, LOW); // Turn LED ON
    LOG_D("Led on\n");
  }else{
    digitalWrite(LED_BUILTIN, HIGH); // Turn LED OFF
    LOG_D("Led off\n");
  }
}

// A key is changed by web sockets
void globalChangeHandler(uint8_t ndx){
  LOG_N("globalChangeHandler ndx: %02u, key: '%s', val: %s\n", ndx, ctrl[ndx].key.c_str(), ctrl[ndx].val.c_str());
  if(ctrl[ndx].key == "led"){ // Led clicked in web page
    // Incoming message, Toggle led and dont send update
    setLed(ctrl[ndx].val.toInt());
    LOG_I("Setting led to %i\n",(int)ctrl[ndx].val.toInt());
    // Set the connected led pin
    String ledKey = String(LED_BUILTIN);
    if(LED_BUILTIN<10) ledKey = "0" + ledKey;
    ctrl.put(ledKey.c_str(), !ctrl[ndx].val.toInt() );
  }else{ // Pin clciked in web page
    int pin = ctrl[ndx].key.toInt();
    LOG_I("Change pin GPIO%i: %s\n", pin, ctrl[ndx].val.c_str());
    // Set selected pin
    pinMode(pin, OUTPUT);
    digitalWrite(pin, ctrl[ndx].val.toInt());
    // On led toggle connected pin
    if(pin == LED_BUILTIN) ctrl.put("led", !ctrl[ndx].val.toInt() );
  }
}
#if defined(ESP32)
// Read a gpio pin state on ESP32
int readGPIO(int pinNo ) { //0-31
  gpio_num_t pin = (gpio_num_t)(pinNo & 0x1F);
  int state = 0;
  if (GPIO_REG_READ(GPIO_ENABLE_REG) & BIT(pin)){   //pin is output - read the GPIO_OUT_REG register
    state = (GPIO_REG_READ(GPIO_OUT_REG)  >> pin) & 1U;
  }else{                                            //pin is input - read the GPIO_IN_REG registe
    state = (GPIO_REG_READ(GPIO_IN_REG)  >> pin) & 1U;
  }
  return state;
}
#else
// Read a gpio pin on ESP8266
// Independent of the setting of port direction (input or output)
int readGPIO(int pinNo ) { //0-17
  return digitalRead(pinNo);
}
#endif

// Read the state of all available pins
void readAllGpio(){
  for(uint i=0; i<TOTAL_PINS; i++){
    int state = readGPIO(i);
    String pin = String(i);
    if (i<10) pin = "0" + pin;
    ctrl.put(pin.c_str(), state, true);

    /* Click on build in led pin
    if(pin.toInt() == LED_BUILTIN){
      setLed(!state);
      ctrl.put("led", !state);
    }
    */
  }
}
void listAllFilesInDir(String dir_path)
{
	Dir dir = LittleFS.openDir(dir_path);
	while(dir.next()) {
		if (dir.isFile()) {
			// print file names
			Serial.print("File: ");
			Serial.println(dir_path + dir.fileName() + " : " + dir.fileSize());
		}
		if (dir.isDirectory()) {
			// print directory names
			Serial.print("Dir: ");
			Serial.println(dir_path + dir.fileName() + "/");
			// recursive file listing inside new directory
			listAllFilesInDir(dir_path + dir.fileName() + "/");
		}
	}
}
void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  if (!STORAGE.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }else{
    LOG_I("Storage statred.\n");
  }
  LOG_I("Starting..\n");
  listAllFilesInDir("/");
  // Connect WIFI ?
  if(strlen(st_ssid)>0){
    LOG_D("Connect Wifi to %s.\n", st_ssid);
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
    if (MDNS.begin(hostName.c_str()))  LOG_I("AP MDNS responder Started\n");
  }

  // Use default CONTROLASSIST_HTML_HEADER and CONTROLASSIST_HTML_FOOTER
  #if USE_SPIFFS_FOR_PAGES
    ctrl.setHtmlBodyFile(BODY_FILE_NAME);
  #else
    ctrl.setHtmlBody(HTML_PAGE);
  #endif
  // Bind led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Turn LED OFF
  ctrl.bind("led", readGPIO(LED_BUILTIN));

  // Bind all pins
  for(int i=0; i<TOTAL_PINS; ++i){
    String pin = String(i);
    if (i<10) pin = "0" + pin;
    int val = readGPIO(pin.toInt());
    ctrl.bind(pin.c_str(), val);
  }
  // Auto send all vars values on connect
  ctrl.setAutoSendOnCon(true);
  // When a value is changed, globalChangeHandler will be called
  ctrl.setGlobalCallback(globalChangeHandler);
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
    server.sendContent("");
  });

  // Dump binded controls handler
  server.on("/d", []() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.sendContent("Serial dump\n");
    String res = "";
    while( ctrl.dump(res) ){
      server.sendContent(res);
    }
    server.sendContent("");
  });

  // Start webserver
  server.begin();
  LOG_V("HTTP server started\n");

}

void loop() {
  if (millis() - pingMillis >= 5000){
    // Toggle led
    setLed(!ctrl["led"].toInt());
    // Update config assist variable and send ws message
    ctrl.put("led", !ctrl["led"].toInt() );
    // Read the state of all gpio
    readAllGpio();
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

