// Capture page changes
PROGMEM const char CONTROLASSIST_SCRIPT_INIT[] = R"=====(
const $ = document.querySelector.bind(document);
const $$ = document.querySelectorAll.bind(document);
var ndxToElm = []

/* * * * Control assist functions * * * */
function updateKeys(event){
  if(event.type=="wsChange") return;
  const e = event.target;
  var value = ""
  if(typeof(e.value) != 'undefined')
    value = e.value.trim();
  else if(typeof(e.innerHTML) != 'undefined')
    value = e.innerHTML;

  const et = event.target.type;

  if (e.type === 'checkbox'){
      updateKey(e.id, e.checked ? 1 : 0);
  }else {
      updateKey(e.id, value);
  }
}

function updateKey(key, value) {
  if(value == null ) return;
  sendWsTxt(keysToNdx[key] + 1 + "\t" + value);
}

function wsUpdatedKeys(event){
  if(dbg) console.log('wsUpdatedKeys: ', keysToNdx[ event.target.id], ', key: ', event.target.id);
}

function initWebSocketCtrls(){
  for(var i in ndxTokeys ) {
    const key = ndxTokeys[i];
    const elm = document.getElementById(key);
    if(elm){
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
    ndxToElm[i] = elm;
  }
}
)=====";

//Scripts to handle websockets
PROGMEM const char CONTROLASSIST_SCRIPT_WEBSOCKETS_CLIENT[] = R"=====(
/* * * * WebSockets functions * * * */
let webserver;
if(port)
  wsServer = "ws://" + document.location.host + ":" + port + "/";
else
  wsServer = "ws://" + document.location.host + ":81/";
const dbg = false;
let ws = null;
let hbTimer = null;
let refreshInterval = 15000;
let wsStatus = null;
let conLed = null;

// close web socket on leaving page
window.addEventListener('beforeunload', function (event) {
  if (ws) closeWS();
  const delay = 500;
  var start = new Date().getTime();
  while (new Date().getTime() < start + delay);
});

function setStatus(msg){
  if(!wsStatus) return;
  wsStatus.innerHTML = msg
  if(dbg) console.log(msg)
  var event = new Event('change');
  wsStatus.dispatchEvent(event);
}

// Websocket handling
function initWebSocket() {
  wsStatus = document.getElementById("wsStatus")
  conLed = document.getElementById("conLed")
  setStatus("Connecting to: " + wsServer);
  ws = new WebSocket(wsServer);
  ws.onopen = onWsOpen;
  ws.onclose = onWsClose;
  ws.onmessage = onWsMessage;
  ws.onerror = onWsError;
}

async function closeWS() {
  setStatus("Disconnected")
  ws.send('0\tC');
  const delay = 500;
  var start = new Date().getTime();
  while (new Date().getTime() < start + delay);
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
  setStatus("Connected: " + wsServer)
  wsHeartBeat();
}
function handleSysMessage(msg){
  if(msg=="C"){
    if(dbg) console.log("Rcv close conn")
    closeWS();
  }
}
// Handle websocket messages
function handleWsMessage(msg){
  //Split only first tab
  var p = msg.indexOf("\t", 0)
  const chn = parseInt( msg.substr(0, p) );
  if(chn < 0 || ndxToElm.length < 1) return;

  const val = msg.substr(p+1, msg.length - 1)
  if(chn == 0){
    handleSysMessage(val);
    return;
  }

  const ndx = chn - 1;
  const elm = ndxToElm[ ndx ];

  if(!elm){
    console.log("Control no: " , ndx, " not found")
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
  setStatus("Error: " + event.code)
}

function onWsClose(event) {
  setStatus("Disconnect: " + event.code + ' - ' + event.reason);
  ws = null;
  // event.codes:
  //   1006 if server not available, or another web page is already open
  //   1005 if closed from app
  if (event.code == 1006) {
    console.log("Closed ws as a newer conn was made");
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
PROGMEM const char CONTROLASSIST_HTML_HEADER[] =
R"=====(<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
  <link rel="shortcut icon" href="data:" />
  <title>ControlAssist</title>
</head>
)=====";
//Template for body of the html page
PROGMEM const char CONTROLASSIST_HTML_BODY[] = R"=====(
<body>
<h1>No html page code defined!</h1>
<h2>Define you html code here</h2>
</body>
)=====";
//Template for footer of the html page
PROGMEM const char CONTROLASSIST_HTML_FOOTER[] = R"=====(
</html>)=====";
