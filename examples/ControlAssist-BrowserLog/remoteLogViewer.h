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

#conLed {
  width: 10px;
  height: 10px;
  background-color: lightGray;
  border-radius: 10px;
  
}
#conLed.on {
  background-color: lightGreen;
  border: 1px solid #4CAF50;
}
</style>
<body>
<table><tr>
  <td><div id="conLed">&nbsp;</td>
  <td><span id="wsStatus">Disconnected</span></td>
  </tr>
</table>

<input type="text" id="logLine" style="Display: None;">
<span id="logText"></span>

<script>
  document.getElementById("wsStatus").addEventListener("change", (event) => {
  conLed = document.getElementById("conLed")
  if(event.target.innerHTML.startsWith("Connected: ")){
    conLed.classList.add("on");
  }else{
    conLed.classList.remove("on");
  }
  conLed.title = event.target.innerHTML
});
</script>

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
// Log print arguments
#define MAX_LOG_BUFFER_LEN 8192             // Maximum size of log buffer to store when disconnected
#define MAX_LOG_FMT 256                     // Maximum size of log format
static char fmtBuf[MAX_LOG_FMT];            // Format buffer
static char outBuf[512];                    // Output buffer
static va_list arglist;
static File log_file;
// Logger file open/close
// Must called before LOG_I function to write to file and
// closed on application shutdown
#define LOGGER_OPEN_LOG()  if(!log_file) log_file = STORAGE.open(LOGGER_LOG_FILENAME, "a+")
#define LOGGER_CLOSE_LOG() if(log_file) log_file.close()
static class RemoteLogViewer *_pLogView = NULL;  // Pointer to instance of log print viewer class
void wsStatusChange();                           // Handles connect / disconnect

// Remote log print viewer class
class RemoteLogViewer: public ControlAssist{
  public:
    RemoteLogViewer() {
      _logBuffer = "";
    }
    RemoteLogViewer(uint16_t port) { ControlAssist::setPort(port);  _logBuffer = ""; }
    virtual ~RemoteLogViewer() { }
  public:
    void init(){
      // Must not initalized
      if(_pLogView){
        LOG_E("Already initialized with addr: %p\n", _pLogView);
        return;
      }
      // Store this instance pointer to global
      _pLogView = this;
      // Setup control assist
      ControlAssist::setHtmlBody(LOGGER_VIEWER_HTML_BODY);
      ControlAssist::setHtmlFooter(LOGGER_VIEWER_HTML_SCRIPT);
      ControlAssist::bind("wsStatus","Disconnected", wsStatusChange);
      ControlAssist::bind("logLine");
    }
    // Delete the log file
    void resetLogFile(){
      if(!STORAGE.exists(LOGGER_LOG_FILENAME)) return;
      LOG_W("Reseting log\n");
      LOGGER_CLOSE_LOG();
      STORAGE.remove(LOGGER_LOG_FILENAME);
      // Reopen log file to store log lines
      LOGGER_OPEN_LOG();
    }

    // Send server responce on /log
    void handleLog(WEB_SERVER &server){
      String res = "";
      while( getHtmlChunk(res) ){
        server.sendContent(res);
      }
      server.sendContent("");
    }
    // Reset or display log file
    void handleLogFile(WEB_SERVER &server){
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      if(server.hasArg("reset")){
        resetLogFile();
        server.sendContent("Reseted log");
        return;
      }
      if(!STORAGE.exists(LOGGER_LOG_FILENAME)){
        server.sendContent("Logging file not exists");
        return;
      }
      LOGGER_CLOSE_LOG();
      // Open for reading
      File file = STORAGE.open(LOGGER_LOG_FILENAME, "r");
      // Send log file contents
      String res = "";
      while( file.available() ){
        res = file.readStringUntil('\n') + "\n";
        server.sendContent(res);
      }
      file.close();
      server.sendContent("");
      // Reopen log file to store log lines
      LOGGER_OPEN_LOG();
    }
    void sendBuffer(){
      if(_logBuffer == "") return;
      String logLine = "";
      int l = 0;
      // Send log buffer lines
      while(_logBuffer.length() > 0){
          l = _logBuffer.indexOf("\n", 0);
          if(l < 0) break;
          logLine = _logBuffer.substring(0, l );
          ControlAssist::put("logLine", logLine );
          _logBuffer = _logBuffer.substring(l + 1 , _logBuffer.length() );
      }
    }

    // Custom log print function
    void print(String line){
      // Are clients connected?
      if(ControlAssist::getClientsNum()>0){
        if(_logBuffer != ""){
             sendBuffer();
            _logBuffer = "";
        }
        // Send current line
        ControlAssist::put("logLine", line, true);
      }else{ // No clients store line to _logBuffer
        // Contains \n by default
        _logBuffer += String(line);
      }
      // Truncate buffer on oversize
      if(_logBuffer.length() > MAX_LOG_BUFFER_LEN){
        int l = _logBuffer.indexOf("\n", 0);
        if(l >= 0)
            _logBuffer = _logBuffer.substring(l + 1, _logBuffer.length());
      }
    }
  public:
    String _logBuffer;                      // String buffer to hold log lines
};

// Custom log print function.. Prints on _pLogView instance
void _log_printf(const char *format, ...){
  strncpy(fmtBuf, format, MAX_LOG_FMT);
  fmtBuf[MAX_LOG_FMT - 1] = 0;
  va_start(arglist, format);
  vsnprintf(outBuf, MAX_LOG_FMT, fmtBuf, arglist);
  va_end(arglist);
  Serial.print(outBuf);
  if(_pLogView) _pLogView->print(outBuf);
  if(log_file){
    log_file.print(outBuf);
    log_file.flush();
  }
}
// Handle websocket connections
void wsStatusChange(){
  if(!_pLogView) return;
  String wsStatus = _pLogView->getVal("wsStatus");
  if(wsStatus.startsWith("Connected:"))
    _pLogView->sendBuffer();
}

#endif