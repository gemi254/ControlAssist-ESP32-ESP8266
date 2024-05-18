PROGMEM const char HTML_HEADERS[] = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="user-scalable=no">
    <link rel="shortcut icon" href="data:" />
    <title>ESP joystick</title>
</head>
<style>
body {
    position: fixed;
    font-family: 'Gill Sans', 'Gill Sans MT', Calibri, 'Trebuchet MS', sans-serif;
    color:rgb(128, 128, 128);
    font-size: xx-large;
    user-select: none;
}

span {
    display: inline-block;
    min-width: 78px;
    text-align: left;
}
</style>
)=====";

PROGMEM const char HTML_BODY[] = R"=====(
<body>
    <h1 style="text-align:center">
        ESP joystick </h1>
    <p style="text-align: center;">
        X: <span id="x_coordinate"></span>&nbsp;
        Y: <span id="y_coordinate"></span>&nbsp;
        Speed: <span id="speed"></span>&nbsp;
        Angle: <span id="angle"></span>
    </p>
    <canvas id="canvas" name="joystick"></canvas>
        <script>
        var canvas, ctx;

        window.addEventListener('load', () => {

            canvas = document.getElementById('canvas');
            ctx = canvas.getContext('2d');
            resize();

            document.addEventListener('mousedown', startDrawing);
            document.addEventListener('mouseup', stopDrawing);
            document.addEventListener('mousemove', Draw);

            document.addEventListener('touchstart', startDrawing);
            document.addEventListener('touchend', stopDrawing);
            document.addEventListener('touchcancel', stopDrawing);
            document.addEventListener('touchmove', Draw);
            window.addEventListener('resize', resize);

            setText('x_coordinate', "0")
            setText('y_coordinate', "0")
            setText('speed', "0")
            setText('angle', "0")
        });

        var width, height, radius, x_orig, y_orig;
        function resize() {
            width = window.innerWidth;
            radius = 50;
            height = radius * 7.5;
            ctx.canvas.width = width;
            ctx.canvas.height = height;
            background();
            joystick(width / 2, height / 3);
        }

        function background() {
            x_orig = width / 2;
            y_orig = height / 3;

            ctx.beginPath();
            ctx.arc(x_orig, y_orig, radius + 60, 0, Math.PI * 2, true);
            ctx.fillStyle = '#ECE5E5';
            ctx.fill();
        }

        function joystick(width, height) {
            ctx.beginPath();
            ctx.arc(width, height, radius, 0, Math.PI * 2, true);
            ctx.fillStyle = '#F08080';
            ctx.fill();
            ctx.strokeStyle = '#F6ABAB';
            ctx.lineWidth = 8;
            ctx.stroke();
        }

        let coord = { x: 0, y: 0 };
        let paint = false;

        function getPosition(event) {
            var mouse_x = event.clientX || event.touches[0].clientX;
            var mouse_y = event.clientY || event.touches[0].clientY;
            coord.x = mouse_x - canvas.offsetLeft;
            coord.y = mouse_y - canvas.offsetTop;
        }

        function is_it_in_the_circle() {
            var current_radius = Math.sqrt(Math.pow(coord.x - x_orig, 2) + Math.pow(coord.y - y_orig, 2));
            if (radius >= current_radius) return true
            else return false
        }

        function setText(key, val) {
            const elm = document.getElementById(key);
            if(elm.innerText != val) {
                elm.innerText = val;
                if(key=="speed")
                    elm.innerText += " %"
                else if(key=="angle")
                    elm.innerText += " \u00B0"
                var event = new Event('change');
                elm.dispatchEvent(event);
            }

        }

        function startDrawing(event) {
            paint = true;
            getPosition(event);
            if (is_it_in_the_circle()) {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                background();
                joystick(coord.x, coord.y);
                Draw();
            }
        }

        function stopDrawing() {
            paint = false;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            background();
            joystick(width / 2, height / 3);

            setText('x_coordinate', "0")
            setText('y_coordinate', "0")
            setText('speed', "0")
            setText('angle', "0")
        }

        function Draw(event) {
            if (paint) {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                background();
                var angle_in_degrees,x, y, speed;
                var angle = Math.atan2((coord.y - y_orig), (coord.x - x_orig));

                if (Math.sign(angle) == -1) {
                    angle_in_degrees = Math.round(-angle * 180 / Math.PI);
                }
                else {
                    angle_in_degrees =Math.round( 360 - angle * 180 / Math.PI);
                }


                if (is_it_in_the_circle()) {
                    joystick(coord.x, coord.y);
                    x = coord.x;
                    y = coord.y;
                }
                else {
                    x = radius * Math.cos(angle) + x_orig;
                    y = radius * Math.sin(angle) + y_orig;
                    joystick(x, y);
                }


                getPosition(event);

                var speed =  Math.round(100 * Math.sqrt(Math.pow(x - x_orig, 2) + Math.pow(y - y_orig, 2)) / radius);

                var x_relative = Math.round(x - x_orig);
                var y_relative = Math.round(y - y_orig);

                setText('x_coordinate', x_relative)
                setText('y_coordinate', y_relative)
                setText('speed', speed)
                setText('angle', angle_in_degrees)
            }
        }
    </script>
</body>
)=====";

PROGMEM const char HTML_FOOTER[] = R"=====(
</html>
)=====";
