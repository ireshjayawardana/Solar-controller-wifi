const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>

<div id="demo">
<h1>Solar  controller Wifi Interface</h1>
	<button type="button" onclick="sendData(1)">Inverter ON</button>
	<button type="button" onclick="sendData(0)">Inverter OFF</button><BR>
  <button type="button" onclick="sendData(2)">Grid ON</button>
  <button type="button" onclick="sendData(3)">Grid OFF</button><BR>
  <button type="button" onclick="sendData(4)">Full auto mode</button><BR>
</div>

<div>
	Battery Voltage Value is : <span id="ADCValue">0</span><br>
  Remote Operation State is : <span id="Inverterstate">NA</span><BR>
  System State is : <span id="SYSstate">NA</span>
</div>
<script>
function sendData(led) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("Inverterstate").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "setInverter?Inverterstate="+led, true);
  xhttp.send();
}

setInterval(function() {
  // Call a function repetatively with 2 Second interval
  getData();
  getDatasys();
}, 500); //2000mSeconds update rate

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("ADCValue").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "readADC", true);
  xhttp.send();
}
function getDatasys() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("SYSstate").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "readSYS", true);
  xhttp.send();
}
</script>
<br><br><a href="IRelectronics">IRelectronics</a>
</body>
</html>
)=====";
