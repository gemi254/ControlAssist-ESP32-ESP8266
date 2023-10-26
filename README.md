# ControlAssist
Multi-platform library for controlling html elements in a esp webpage at runtime using websockets.

## Description
A library allowing linking of **html elements** to **sketch variables** on pages hosted on **esp32/esp8266** devices. It uses a **websocket server** on the esp **device** and a JavaScript **webocket client** implementation on the web **page** allowing bi-directional real-time communication between **device** and **page**. 

In a typical webpage, html **elements** like ``input box``, ``textbox``, ``range``, ``checkbox`` can be **binded** with ControlAssist internal variables using their unique **html ids** in order to associate their values. A **vectors** list will be generated to hold all associated elements keys and their values.

Every time an element is changing its value in the web page, a websocket message will be send to server and ControlAssist will update its internal value. Also if you change a ControlAssist value inside your sketch, a message will be automatically send to client and the value of the associated html element will be updated.

ControlAssist will automatically add JavaScript **onChange** handlers to the web page html code, so the binded elements will transmit their changes automatically to the server. It will also add code to handle incoming websockets messages to update the values of the html elements.

Esp device can also transmit changes to the html page elements using control channels in order to change the values of html page.

## Features
* Automate **variables** and html **elements** in a typical ``ESP32/ESP8266`` project using websockets communication.
* Auto **synchronize** ESP32/ESP8266 internal **variables** with webpage elements.
* Automatically generate required **webpage scripts** to handle connections and changes.
* Support bi-direction hi-speed communication
* Allow **mult-client** applications.
* Support websockets over **AP** connections.

## How it works
Define your internal page html code.

```
PROGMEM const char HTML_HEADERS[] = R"=====(<!DOCTYPE HTML>)=====";
PROGMEM const char HTML_BODY[] = R"=====(<body></body>)=====";
PROGMEM const char HTML_FOOTER[] = R"=====(</htmll>)=====";
```

## ControlAssist init functions
Define and Initialize you class 
+ include the **controlAssist**  class
  - `#include <controlAssist.h>  //ControlAssist class`

+ Define your static instance
  - `ControlAssist ctrl;              //Default port 81 `
  - `ControlAssist ctrl(port);        //Use port `

+ in your setup function you must initialize control class by setting your webpage html code.
  - `ctrl.setHtmlFooter(HTML_SCRIPT);`
  - `ctrl.setHtmlHeaders(HTML_HEADERS);`
  - `ctrl.setHtmlBody(HTML_BODY);`

+ in your setup function you must bind the html elements you want to control
  - `ctrl.bind("html_id");` to link the html element
  - `ctrl.bind("html_id", changeFunction);` if you need also to handle changes
  
+ Define a web server handler to host your webpage 
  - `void handleRoot(){ ctrl.sendHtml(server); }`

+ If you want to use a global callback function 
  - `ctrl.setGlobalCallback(globalChangeFuncion);`

+ Start websockets server and listen for connections
  - `ctrl.begin();`


## ControlAssist control functions
Controlling your elements 
+ Change the values of html elements
  - `ctrl.put("html_id", value );`

+ Read current value of html element
  - `html_val = ctrl["html_id"]`

+ Inside your main loop() function Handle websockets server connections
  - `ctrl.loop();`


## JavaScript handlers inside your webpage
A JavaScript event ``wsChange`` will be automatically send to each html element when the esp device changes it's value. You can add a JavaScript event listener to this event at your web page if you want to perform custom tasks when element value is updated by websockets.

```
html_id.addEventListener("wsChange", (event) => {
    //Get the changed value
    value = event.target.value
    event.preventDefault();
    return false;
});
```

## Logging and debugging with log level
In you application you use **LOG_E**, **LOG_W**, **LOG_I**, **LOG_D** macros instead of **Serial.prinf** to print your messages. **ControlAssist** displays these messages with **timestamps** 

You can define log level for each module
```#define LOGGER_LOG_LEVEL 4```
```
#define _LOG_LEVEL_NONE      (0)
#define _LOG_LEVEL_ERROR     (1)
#define _LOG_LEVEL_WARN      (2)
#define _LOG_LEVEL_INFO      (3)
#define _LOG_LEVEL_DEBUG     (4)
#define _LOG_LEVEL_VERBOSE   (5)
```

## Compile
Download library files and place them on ./libraries directory under ArduinoProjects
Then include the **controlAssist.h** in your application and compile..

+ compile for arduino-esp3 or arduino-esp8266.
+ In order to compile you must install **WebSocketsServer** library.


###### If you get compilation errors on arduino-esp32 you need to update your arduino-esp32 library in the IDE using Boards Manager

