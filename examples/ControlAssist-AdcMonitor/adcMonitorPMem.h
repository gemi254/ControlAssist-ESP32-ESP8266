PROGMEM const char CONTROLASSIST_HTML_HEADER[] = R"=====(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
  <link rel="shortcut icon" href="data:" />
  <title>ControlAssist</title>
<style>
body {
    background-color: black;
    font-family: "Titillium Web", Helvetica, Arial, sans-serif;
}
.center {
    display: flex;
    align-items: center;
    justify-content: center;
}
h1{
    color: white;
    text-align: center;
}
#controls {
	display: flex;
	flex-direction: column;
	z-index: 1;
	background-color: #6666;
	color: white;
	padding: 15px;
	border-radius: 10px;
	margin: 15px;
	font-size: 1.2rem;
    width: 30%
}
.verticalChart {
  width:100%;
  padding: 15px;
  .singleBar {
    width:9%;
    float:left;
    margin-left:0.5%;
    margin-right:0.5%;
    .bar {
      position:relative;
      height:200px;
      background:rgba(255,255,255,0.2);
      overflow:hidden;
      .value {
        position:absolute;
        bottom:0;
        width:100%;
        background:#ffffff;
        color:black;
        font-size: .8rem;
        padding: 2px;
        span {
          position:absolute;
          bottom:0;
          width:100%;
          height:20px;
          display:none;
        }
      }
    }
    .title {
      margin-top:5px;
      text-align:center;
      color: #fff;
    }
  }
}
.on {
	background-color: #49cb61;
}
</style>
)=====";

PROGMEM const char CONTROLASSIST_HTML_BODY[] = R"=====(
</head>
<body>
<h1>Monitor all ESP adc pins</h1>
<div class="center">
    <div id="controls">
        <button id="on-off">Turn On</button>
        <label for="speed">Delay: <span id="speedValue">40</span> ms</label>
        <input id="speed" type="range" min="0" max="1500" step="1" value="40">
        <label for="gain">Input Gain: <span id="gainValue">1</span></label>
        <input id="gain" type="range" min="0" max="5" step="0.05" value="1">
    </div>
</div>
<div class="verticalChart center">)=====";

PROGMEM const char CONTROLASSIST_HTML_BAR[] = R"=====(
  <div class="singleBar" id="adc_{key}">
    <div class="bar">
      <div class="value" style="height: 0%;">
        <span style="display: inline;">0</span>
      </div>
    </div>
    <div class="title"></div>
  </div>)=====";

PROGMEM const char CONTROLASSIST_HTML_BAR_SCRIPT[] = R"=====(
var adc_{key} = new bar('adc_{key}', 0, 4096, '')
adc_{key}.ctrl.addEventListener("wsChange", (event) => {
    event.target.class.update(Math.ceil(event.target.value * gain))
    event.preventDefault();
    return false;
});
)=====";

PROGMEM const char CONTROLASSIST_HTML_FOOTER[] = R"=====(
</div>
</body>
<script>
let isPlaying = 0,
    gain = 1,
    speed = 40
const powerBtn = document.getElementById("on-off"),
    speedSlider = document.getElementById("speed"),
    gainSlider = document.getElementById("gain")

//Power on off button handlers
powerBtn.addEventListener("click", () => {
  if (isPlaying) {
    powerBtn.innerHTML = "Turn On";
  } else {
    powerBtn.innerHTML = "Turn Off";
  }
  powerBtn.classList.toggle("on");
  isPlaying = !isPlaying;
  // Manually update the key on-off
  updateKey('on-off',isPlaying ? 1 : 0 );
});

powerBtn.addEventListener("wsChange", (event) => {
  isPlaying = event.target.value == "0" ? false : true;
  if (isPlaying) {
    powerBtn.innerHTML = "Turn Off";
    powerBtn.classList.add("on");
  } else {
    powerBtn.innerHTML = "Turn On";
    powerBtn.classList.remove("on");
  }
});

// Gain slider handlers
gainSlider.addEventListener("input", (event) => {
  gain = event.target.value;
  document.getElementById("gainValue").innerHTML = gain;
});

// Speed slider handlers
speedSlider.addEventListener("input", (event) => {
  speed = event.target.value;
  document.getElementById("speedValue").innerHTML = speed;
});

speedSlider.addEventListener("wsChange", (event) => {
  speed = event.target.value;
  document.getElementById("speedValue").innerHTML = speed;
});


const scale = (number, [inMin, inMax], [outMin, outMax]) => {
    const res = (number - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
    if(res<outMin) return outMin
    else if(res>outMax) return outMax;
    return res;
}

class bar {
  constructor(id, min, max, units = "%") {
    this.ctrl = document.getElementById(id)
    //this.bar = this.ctrl.querySelector(".bar")
    this.val = this.ctrl.querySelector(".value")
    this.span = this.ctrl.querySelector("span");
    this.title = this.ctrl.querySelector(".title")
    this.title.innerHTML = id;
    this.min = min;
    this.max = max;
    this.units = units;
    this.ctrl.class = this;
    this.ctrl.type = 'Custom'
  }
  update(v){
    let perc = Math.ceil(scale(v, [this.min, this.max], [0,100]))
    this.span.innerHTML = v;
    this.val.style.height = perc +'%';
  }
}
/*{SUB_SCRIPT}*/
</script>
</html>
)=====";
