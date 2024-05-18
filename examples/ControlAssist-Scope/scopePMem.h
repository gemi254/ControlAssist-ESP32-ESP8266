PROGMEM const char HTML_HEADERS[] = R"=====(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
  <link rel="shortcut icon" href="data:" />
  <title>ControlAssist</title>
<style>
html, body {
	margin: 0;
	width: 100%;
	font-family: Arial;
}
.center {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  padding: 10px;
}
#container{
  border: 1px solid;
  display: flex;
  flex-direction: column;
  width: 512px;
  text-align: center;
}
#canvas {
	background-color: #181818;
}
#controls {
	display: flex;
	flex-direction: column;
	z-index: 1;
	background-color: #6666;
	color: #fff;
	padding: 15px;
	border-radius: 10px;
	margin: 15px;
	font-size: 1.2rem;
}
#controls button {
	border: 0;
  font-size: 1.2rem;
  padding: 5px;
}
#controls label {
  margin-top: 5px;
	margin-bottom: 3px;
}
.on {
	background-color: #49cb61;
}
.bottom {
    position: relative;
    top: 5px;
    width: 100%;
    font-size: x-small;
    text-align: center;
}
</style>
</head>
)=====";

PROGMEM const char HTML_BODY[] = R"=====(
<body>
<div id="container" class="center">
  <h3>ControlAssist Monitor ADC port</h3>
  <div id="controls">
      <button id="on-off">Turn On</button>
      <input type="text" id="adc_val" value="0" style="display:none;">
      <label for="speed">Delay: <span id="speedValue">40</span> ms</label>
      <input id="speed" type="range" min="0" max="1500" step="1" value="40">
      <label for="gain">Input Gain: <span id="gainValue">1</span></label>
      <input id="gain" type="range" min="0" max="5" step="0.05" value="1">
  </div>
  <canvas id="canvas"></canvas>
  <div class="bottom">
    <div id="conLed" class="center"></div>
    <span id="wsStatus" style="display: none1;"></span>
  </div>
</div>
</body>
)=====";

PROGMEM const char HTML_SCRIPT[] = R"=====(
<script>
let isPlaying, pixelRatio, sizeOnScreen, segmentWidth
let gain = 1
const datBitCount = 512
const canvas = document.getElementById("canvas"),
    c = canvas.getContext("2d", { willReadFrequently: true }),
    powerBtn = document.getElementById("on-off"),
    speedSlider = document.getElementById("speed"),
    gainSlider = document.getElementById("gain"),
    adc_val =  document.getElementById("adc_val"),
    dataArray = new Uint16Array(datBitCount);

speed = speedSlider.value;
canvas.width = window.innerWidth;
canvas.height = window.innerHeight;
pixelRatio = window.devicePixelRatio;
sizeOnScreen = canvas.getBoundingClientRect();
canvas.width = sizeOnScreen.width * pixelRatio;
canvas.height = sizeOnScreen.height * pixelRatio;
canvas.style.width = canvas.width / pixelRatio + "px";
canvas.style.height = canvas.height / pixelRatio + "px";
c.fillStyle = "#181818";
c.fillRect(0, 0, canvas.width, canvas.height);
c.strokeStyle = "#33ee55";
c.beginPath();
c.moveTo(0, canvas.height / 2);
c.lineTo(canvas.width, canvas.height / 2);
c.stroke();
segmentWidth = canvas.width / datBitCount;

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
  if (isPlaying) draw();
});
// Listen to ws events
powerBtn.addEventListener("wsChange", (event) => {
  isPlaying = event.target.value == "0" ? false : true;
  if (isPlaying) {
    powerBtn.innerHTML = "Turn Off";
    powerBtn.classList.add("on");
  } else {
    powerBtn.innerHTML = "Turn On";
    powerBtn.classList.remove("on");
  }
  if (isPlaying) draw();
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

// Gain slider handlers
gainSlider.addEventListener("input", (event) => {
  gain = event.target.value;
  document.getElementById("gainValue").innerHTML = gain;
});

adc_val.addEventListener("wsChange", (event) => {
    if(dbg) console.log('change:',event.target.value);
    shiftRight(dataArray,event.target.value * gain)
    event.preventDefault();
    return false;
});

const shiftRight = (collection, value) => {
  collection.set(collection.subarray(0, -1), 1)
  collection.fill(value, 0, 1)
  return collection;
}

const scale = (number, [inMin, inMax], [outMin, outMax]) => {
    return (number - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
}

const draw = () => {
  if(dbg) console.time("Draw");
  c.fillRect(0, 0, canvas.width, canvas.height);
  c.beginPath();
  c.moveTo(-100, canvas.height / 2);

  if (isPlaying) {
    for (let i = 0; i < datBitCount; i += 1) {
      let x = i * segmentWidth;
      let y = canvas.height - scale(dataArray[i],[0,4095], [0,canvas.height])
      c.lineTo(x, y);
    }
  }
  c.lineTo(canvas.width + 100, canvas.height / 2);
  c.stroke();
  if(dbg) console.timeEnd("Draw");
  if (isPlaying) requestAnimationFrame(draw);
};
</script>
)=====";