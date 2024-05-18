PROGMEM const char HTML_HEADERS[] = R"=====(
<!DOCTYPE html>
<html lang="en" >
<head>
  <meta charset="UTF-8">
  <link rel="shortcut icon" href="data:" />
  <title>ESP Monitor</title>
<style>
.center {
    position: absolute;
    top: 25%;
    left: 50%;
    transform: translate(-50%, -50%);
    padding: 10px;
}
.container{
    display: flex;
}
.gauge {
    position: relative;
    border-radius: 50%/100% 100% 0 0;
    background-color: var(--color, #a22);
    overflow: hidden;
    margin: 5px;
}
.gauge:before{
    content: "";
    display: block;
    padding-top: 50%;   /* ratio of 2:1*/
    }
.gauge .chart {
    overflow: hidden;
}
.gauge .mask {
    position: absolute;
    left: 20%;
    right: 20%;
    bottom: 0;
    top: 40%;
    background-color: #fff;
    border-radius: 50%/100% 100% 0 0;
}
.gauge .percentage {
    position:  absolute;
    top: -1px;
    left: -1px;
    bottom: 0;
    right: -1px;
    background-color: var(--background, #aaa);
    transform:rotate(var(--rotation));
    transform-origin: bottom center;
    transition-duration: 600;
}
.gauge:hover {
    --rotation: 100deg;
}
.gauge .value {
    position:absolute; bottom:17%; left:0;
    width:100%;
    text-align: center;
    font-family:Verdana, Geneva, Tahoma, sans-serif;
    font-weight: bolder;
    font-size: 1.5rem;
}
.gauge  .title {
    position:absolute; bottom:0; left:0;
    width:100%;
    text-align: center;
    font-family:Verdana, Geneva, Tahoma, sans-serif;
    font-weight: bold;
    font-size: .8rem;
}
.gauge .min {
    position:absolute;
    bottom:0; left:5%;
}
.gauge .max {
    position:absolute;
    bottom:0; right:5%;
 }
</style>
</head>
)=====";

#if defined(ESP32)
PROGMEM const char HTML_BODY[] = R"=====(
<body translate="no">
<div style="width: 100%; text-align: center;"><h1>ESP32 system monitor</h1></div>
<div class="container center">
    <div class="gauge" id="rssi" style="width: 200px; --rotation:0deg; --color:#e4a229; --background:#e9ecef;">
        <div class="percentage"></div>
        <div class="mask"></div>
        <span class="value">0%</span>
        <span class="title">RSSI</span>
    </div>
    <div class="gauge" id="mem" style="width: 200px; --rotation:0deg; --color:#238da8; --background:#e9ecef;">
        <div class="percentage"></div>
        <div class="mask"></div>
        <span class="value">0%</span>
        <span class="title">Memory</span>
    </div>
    <div class="gauge" id="temp" style="width: 200px; --rotation:0deg; --color:#9d23a8; --background:#e9ecef;">
        <div class="percentage"></div>
        <div class="mask"></div>
        <span class="value">0%</span>
        <span class="title">Temperature</span>
    </div>
    <div class="gauge" id="hall" style="width: 200px; --rotation:0deg; --color:#23a84b; --background:#e9ecef;">
        <div class="percentage"></div>
        <div class="mask"></div>
        <span class="value">0%</span>
        <span class="title">HallRead</span>
    </div>
</div>
<script>
window.console = window.console || function(t) {};
const scale = (number, [inMin, inMax], [outMin, outMax]) => {
    const res = (number - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
    if(res<outMin) return outMin
    else if(res>outMax) return outMax;
    return res;
}
class gauge {
  constructor(id, min, max, units = "%") {
    this.ctrl = document.getElementById(id)
    this.min = min;
    this.max = max;
    this.units = units;
    this.ctrl.class = this;
    this.ctrl.type = 'Custom'
  }
  update(v){
    let deg = Math.ceil(scale(v, [this.min, this.max], [0,180]))
    this.ctrl.style.setProperty('--rotation', deg+'deg')
    for(var i in this.ctrl.children){
        if(this.ctrl.children[i].className=="value"){
            this.ctrl.children[i].innerHTML = v + this.units
        }
    }
  }
}
var rssi = new gauge('rssi', -100, 0)
rssi.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update(event.target.value)
    event.preventDefault();
    return false;
});

var mem = new gauge('mem', 0, 100)
mem.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update(event.target.value)
    event.preventDefault();
    return false;
});

var temp = new gauge('temp', 0, 100,"°C")
temp.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update(event.target.value)
    event.preventDefault();
    return false;
});

var hall = new gauge('hall', -500, 500,"")
hall.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update(event.target.value)
    event.preventDefault();
    return false;
});
</script>
</body>
)=====";
#else
PROGMEM const char HTML_BODY[] = R"=====(
<body translate="no">
<div style="width: 100%; text-align: center;"><h1>ESP8266 system monitor</h1></div>
<div class="container center">
    <div class="gauge" id="rssi" style="width: 200px; --rotation:0deg; --color:#e4a229; --background:#e9ecef;">
        <div class="percentage"></div>
        <div class="mask"></div>
        <span class="value">0%</span>
        <span class="title">RSSI</span>
    </div>
    <div class="gauge" id="mem" style="width: 200px; --rotation:0deg; --color:#238da8; --background:#e9ecef;">
        <div class="percentage"></div>
        <div class="mask"></div>
        <span class="value">0%</span>
        <span class="title">Memory</span>
    </div>
    <div class="gauge" id="vcc" style="width: 200px; --rotation:0deg; --color:#9d23a8; --background:#e9ecef;">
        <div class="percentage"></div>
        <div class="mask"></div>
        <span class="value">0 V</span>
        <span class="title">Vcc</span>
    </div>
</div>
<script>
window.console = window.console || function(t) {};
const scale = (number, [inMin, inMax], [outMin, outMax]) => {
    const res = (number - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
    if(res<outMin) return outMin
    else if(res>outMax) return outMax;
    return res;
}

class gauge {
  constructor(id, min, max, units = "%") {
    this.ctrl = document.getElementById(id)
    this.min = min;
    this.max = max;
    this.units = units;
    this.ctrl.class = this;
    this.ctrl.type = 'Custom'
  }
  update(v){
    let deg = Math.ceil(scale(v, [this.min, this.max], [0,180]))
    this.ctrl.style.setProperty('--rotation', deg+'deg')
    for(var i in this.ctrl.children){
        if(this.ctrl.children[i].className=="value"){
            this.ctrl.children[i].innerHTML = v + this.units
        }
    }
  }
}
var rssi = new gauge('rssi', -100, 0)
rssi.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update(event.target.value)
    event.preventDefault();
    return false;
});

var mem = new gauge('mem', 0, 100)
mem.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update(event.target.value)
    event.preventDefault();
    return false;
});

var vcc = new gauge('vcc', 0, 5,"V")
vcc.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update( (event.target.value/1000).toFixed(2) )
    event.preventDefault();
    return false;
});
</script>
</body>
)=====";
#endif
