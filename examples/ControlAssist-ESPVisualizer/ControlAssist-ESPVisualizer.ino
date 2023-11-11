#if defined(ESP32)
  #include <WebServer.h>
  WebServer server(80);  
  #include <ESPmDNS.h>  
  #include "gpioPMemESP32.h"
  #define TOTAL_PINS 40
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>  
  ESP8266WebServer  server(80);
  #include <ESP8266mDNS.h>
  #include "gpioPMemESP8266.h"
  #define TOTAL_PINS 17
#endif

const char st_ssid[]="";                    // Put connection SSID here. On empty an AP will be started
const char st_pass[]="";                    // Put your wifi passowrd.
unsigned long pingMillis = millis();        // Ping millis

#define LOGGER_LOG_LEVEL 5
#include <ControlAssist.h>                  // Control assist class

ControlAssist ctrl(81);                     // Web socket control on port 81

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
  for(uint i=00; i<TOTAL_PINS; i++){
    int state = readGPIO(i);
    String pin = String(i);
    if (i<10) pin = "0" + pin;
    ctrl.put(pin.c_str(), state, true);
    
    // Click on build in led pin
    if(pin.toInt() == LED_BUILTIN){
      setLed(!state);
      ctrl.put("led", !state);
    }
  }
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
    if (MDNS.begin(hostName.c_str()))  LOG_I("AP MDNS responder Started\n");     
  }

  // Setup control assist
  ctrl.setup(server, "/");
  ctrl.setHtmlBody(HTML_PAGE);
  pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, HIGH); // Turn LED OFF
  ctrl.bind("led", readGPIO(LED_BUILTIN));
  
  // Bind all pins
  for(int i=00; i<TOTAL_PINS; ++i){
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
  ctrl.dump();
  LOG_V("ControlAssist started.\n"); 
  
  // Setup webserver  
  server.on("/d", []() { // Dump controls
    server.send(200, "text/plain", "Serial dump");
    ctrl.dump();
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

