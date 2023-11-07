/*  A web logger using ControlAssist web sockets */
#if !defined(_WEB_LOGGER_H)
#define  _WEB_LOGGER_H 

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

#define MAX_LOG_BUFFER_LEN 8192             // Maximum size of log buffer to store when disconnected

class WebLogger{
  public:
    WebLogger() { _port = 81; _logBuffer = "";}
    WebLogger(uint16_t port) { _port = port; _logBuffer = "";}
    ~WebLogger() {}
  public:
    void setup(WEB_SERVER &server, const char *uri = "/log"){
      // Setup control assist
      _ctrl.setPort(_port);
      _ctrl.setHtmlBody(CONTROLASSIST_HTML_BODY);
      _ctrl.setHtmlFooter(CONTROLASSIST_HTML_SCRIPT);
      _ctrl.bind("logLine");
      _ctrl.begin();
      _ctrl.dump();
      LOG_V("WebLogger started at uri: %s\n", uri); 
      _ctrl.setup(server, uri);
    }
    uint8_t getClientsNum() { return _ctrl.getClientsNum(); }
    void loop(){ _ctrl.loop(); }     
    
    // Custom log print function 
    void print(String line){
      Serial.print(line);  
      if(getClientsNum()>0){ //Is clients connected?
        if(_logBuffer!=""){
        _ctrl.put("logLine",_logBuffer);
        _logBuffer = "";
        } 
        _ctrl.put("logLine",line,true);
      }else{ // No clients store to _logBuffer
        if(_logBuffer.length()) _logBuffer += "<br>";
        //if(_logBuffer.length()) _logBuffer += " (" + String(_logBuffer.length()) +")<br>";
        _logBuffer += String(line);
      } 
      if(_logBuffer.length() > MAX_LOG_BUFFER_LEN){
        int l = _logBuffer.indexOf("<br>");
        _logBuffer = _logBuffer.substring(l + 4, _logBuffer.length() - 1);
      }
    }
  public:
    ControlAssist _ctrl; //Control assist class

  private:
    uint16_t _port;
    String _logBuffer;

};
// Log print arguments
#define MAX_LOG_FMT 256                     // Maximum size of log format
static char fmtBuf[MAX_LOG_FMT];            // Format buffer
static char outBuf[512];                    // Output buffer
static va_list arglist;

extern WebLogger weblog;
void _log_printf(const char *format, ...){  
  strncpy(fmtBuf, format, MAX_LOG_FMT);
  fmtBuf[MAX_LOG_FMT - 1] = 0;
  va_start(arglist, format);  
  vsnprintf(outBuf, MAX_LOG_FMT, fmtBuf, arglist);
  va_end(arglist);
  weblog.print(outBuf); 
}


#endif