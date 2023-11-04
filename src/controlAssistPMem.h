// Capture page changes
PROGMEM const char CONTROLASSIST_SCRIPT_INIT[] = R"=====(
const $ = document.querySelector.bind(document);
const $$ = document.querySelectorAll.bind(document);
var ctrlNames = []

/* * * * Control assist functions * * * */
function updateKeys(event){
  if(event.type=="wsChange") return;
  const e = event.target;
  
  const value = e.value.trim();
  const et = event.target.type;
  
  if (e.type === 'checkbox'){ 
      updateKey(e.id, e.checked ? 1 : 0);
  }else {
      updateKey(e.id, value);
  }

}
function updateKey(key, value) {          
  if(value == null ) return;  
  sendWsTxt(ctlbyName[key] + "\t" + value);
}

function wsUpdatedKeys(event){
  if(dbg) console.log('wsUpdatedKeys:', event.target.name)
}

function initWebSocketCtrls(){
  for(var key in ctlbyName) {
    const elm = document.getElementById(key);
    if(!elm){
      ctrlNames.push([key, null]);
    }else{
      if(elm.type === "button"){  //|| elm.type === "submit"){        
        elm.addEventListener("click", updateKeys);     
      }else if(elm.type === "checkbox"){        
        elm.addEventListener("change", updateKeys);
      }else{
        elm.addEventListener("change", updateKeys);
        elm.addEventListener("input", function (event) {
          updateKeys(event)
        });        
      }
      // Websocket events
      elm.addEventListener("wsChange", wsUpdatedKeys);
    }
    ctrlNames.push([key, elm]); 
  }
}
)=====";

//Scripts to handle websockets
PROGMEM const char CONTROLASSIST_SCRIPT_WEBSOCKETS_CLIENT[] = R"=====(
/* * * * WebSockets functions * * * */
//const wsServer = "ws://10.1.0.169:81/";
let webserver;
if(port)
  wsServer = "ws://" + document.location.host + ":" + port + "/";
else
  wsServer = "ws://" + document.location.host + ":81/";
const dbg = false;
let ws = null;
let hbTimer = null;
let refreshInterval = 15000;

// close web socket on leaving page
window.addEventListener('beforeunload', function (event) {
  if (ws) closeWS();
}); 

// Websocket handling
function initWebSocket() {  
  console.log("Connect to: " + wsServer);
  ws = new WebSocket(wsServer);
  ws.onopen = onWsOpen;
  ws.onclose = onWsClose;
  ws.onmessage = onWsMessage; 
  ws.onerror = onWsError;
}
async function closeWS() {
  ws.send('0:C');
  await sleep(500);
  ws.close();
}
function sendWsTxt(reqStr) {
  ws.send(reqStr);
  if(dbg) console.log("> " + reqStr);
}
function resetHbTimer(){
  clearTimeout(hbTimer);
  hbTimer = setTimeout(wsHeartBeat, refreshInterval);
}
// Check that connection is still up 
function wsHeartBeat() {
  if (!ws) return;
  if (ws.readyState !== 1) return;
  sendWsTxt("0");
  resetHbTimer();
}
// Connected
function onWsOpen(event) {
  if(dbg) console.log("onWsOpen");
  wsHeartBeat();
}
// Handle websocket messages
function handleWsMessage(msg){   
  var itm = msg.split("\t");
  const key = ctlByNo[ itm[0] ];
  const val = itm[1] 
  const n = parseInt(itm[0]) - 1;
  if(n < 0 || ctrlNames.length < 1) return;
  const elm = ctrlNames[n][1];
  if(!elm){
    console.log("Control not found: ", n, ctrlNames)
    return;
  } 
  if(elm.type === undefined){
    elm.innerHTML = val;
  }else if (elm.type === 'checkbox'){
    elm.checked = (val=="0" ? false : true);
  }else{
    elm.value = val;
  }
  var event = new Event('wsChange');  
  elm.dispatchEvent(event);
}

// Received WS message
function onWsMessage(messageEvent) {
  resetHbTimer();
  if(dbg) console.log("< ",messageEvent.data);
  if (messageEvent.data.startsWith("{")) {
    // json data
    updateData = JSON.parse(messageEvent.data);
    updateTable(updateData); // format received config json into html table
  } else {
    handleWsMessage(messageEvent.data);
  }
}
function onWsError(event) {
  console.log("WS Error: " + event.code);
}

function onWsClose(event) {
  console.log("Discon: " + event.code + ' - ' + event.reason);
  ws = null;
  // event.codes:
  //   1006 if server not available, or another web page is already open
  //   1005 if closed from app
  if (event.code == 1006) { 
    console.log("Closed websocket as a newer connection was made"); 
    initWebSocket();
  }else if (event.code != 1005) initWebSocket(); // retry if any other reason
}

/*{SUB_SCRIPT}*/
)=====";

PROGMEM const char CONTROLASSIST_SCRIPT_INIT_CONTROLS[] = R"=====(
document.addEventListener('DOMContentLoaded', function (event) {
  initWebSocket();
  initWebSocketCtrls();
  
  /*{SUB_SCRIPT}*/
})
)=====";
//Template for header of the html page
PROGMEM const char CONTROLASSIST_HTML_HEADER[] = R"=====(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
  <title>Control Assist</title>                        
</head>
)=====";
//Template for body of the html page
PROGMEM const char CONTROLASSIST_HTML_BODY[] = R"=====(
<body>
<h1>Put your html code here</h1>
</body>
)=====";
//Template for footer of the html page
PROGMEM const char CONTROLASSIST_HTML_FOOTER[] = R"=====(
</html>
)=====";
