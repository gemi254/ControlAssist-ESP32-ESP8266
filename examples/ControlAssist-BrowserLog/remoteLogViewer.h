/*  A web remote debugger using ControlAssist web sockets */
#if !defined(_REMOTE_LOG_VIEWER_H)
#define  _REMOTE_LOG_VIEWER_H 

PROGMEM const char LOGGER_VIEWER_HTML_BODY[] = R"=====(
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

PROGMEM const char LOGGER_VIEWER_HTML_SCRIPT[] = R"=====(
<script>
const logLine = document.getElementById("logLine"),
logText = document.getElementById("logText")
const root = getComputedStyle($(':root'));

// Event listener for web sockets messages
logLine.addEventListener("wsChange", (event) => {
  if(logText.innerHTML.length) logText.innerHTML += "<br>";
  let lines = event.target.value.split("\n");
  if(lines.length > 1){
    for (i in lines){
      logText.innerHTML += toColor( lines[i] );
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

#include <ControlAssist.h>

static class RemoteLogViewer *pLogView = NULL;  // Pointer to instance of log print viewer class
#define MAX_LOG_BUFFER_LEN 8192                 // Maximum size of log buffer to store when disconnected

// Remote log print viewer class
class RemoteLogViewer: public ControlAssist{
  public:
    RemoteLogViewer() {
      _logBuffer = "";
    }
    RemoteLogViewer(uint16_t port) { ControlAssist::setPort(port);  _logBuffer = ""; }
    virtual ~RemoteLogViewer() { }
  public:
    void setup(WEB_SERVER &server, const char *uri = "/log"){
      // Must not initalized
      if(pLogView){
        LOG_E("Already initialized with addr: %p\n", pLogView);
        return;
      }
      // Store this instance pointer to global
      pLogView = this; 
      // Setup control assist
      ControlAssist::setHtmlBody(LOGGER_VIEWER_HTML_BODY);
      ControlAssist::setHtmlFooter(LOGGER_VIEWER_HTML_SCRIPT);
      ControlAssist::setup(server, uri);
      ControlAssist::bind("logLine");      
    }  

    // Custom log print function 
    void print(String line){
      // Are clients connected?      
      if(ControlAssist::getClientsNum()>0){ 
        if(_logBuffer!=""){
            String logLine = "";
            int l = 0;
            while(_logBuffer.length() > 0){
                l = _logBuffer.indexOf("\n", 0);
                if(l < 0) break;
                logLine = _logBuffer.substring(0, l - 1 );
                ControlAssist::put("logLine", logLine );
                _logBuffer = _logBuffer.substring(l + 1 , _logBuffer.length() );                
            }
            _logBuffer = "";
        } 
        ControlAssist::put("logLine", line, true);
      }else{ // No clientsm store to _logBuffer
        // Contains \n by default
        _logBuffer += String(line);
        //if(_logBuffer.length()) _logBuffer += " (" + String(_logBuffer.length()) +")";
      } 
      // Truncate buffer on oversize
      if(_logBuffer.length() > MAX_LOG_BUFFER_LEN){
        int l = _logBuffer.indexOf("\n", 0);
        if(l >= 0) 
            _logBuffer = _logBuffer.substring(l + 1, _logBuffer.length() - 1);
      }
    }
  public:
    String _logBuffer;
};

// Log print arguments
#define MAX_LOG_FMT 256                     // Maximum size of log format
static char fmtBuf[MAX_LOG_FMT];            // Format buffer
static char outBuf[512];                    // Output buffer
static va_list arglist;

// Custom log print function.. Prints on pLogView instance
void _log_printf(const char *format, ...){  
  strncpy(fmtBuf, format, MAX_LOG_FMT);
  fmtBuf[MAX_LOG_FMT - 1] = 0;
  va_start(arglist, format);  
  vsnprintf(outBuf, MAX_LOG_FMT, fmtBuf, arglist);
  va_end(arglist);
  Serial.print(outBuf);
  if(pLogView) pLogView->print(outBuf);     
}

#endif