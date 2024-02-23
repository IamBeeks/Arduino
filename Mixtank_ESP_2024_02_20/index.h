const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<style type="text/css">
.button {
  background-color: #4CAF50; /* Green */
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
</style>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP8266 Data Receiver</title>
</head>
<body style="background-color: #3399ff ">
<div id="receivedValues"></div>
    <script>
        function fetchData() {

        fetchData();
    </script>
<center>
<div>
<h1>BeekerTank and MixTank</h1>
  <button class="button" onclick="send(1)">LED ON</button>
  <button class="button" onclick="send(0)">LED OFF</button><BR>
</div>
 <br>
<div><h2>
  Ambient: <span id="adc_val">0</span><br><br>
  
  LED State: <span id="state">NA</span>
</h2>
</div>
<script>
function send(led_sts) 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("state").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "led_set?state="+led_sts, true);
  xhttp.send();
}
setInterval(function() 
{
  getData();
}, 2000); 
function getData() {
             const url = "http://10.216.1.235/data"; // Replace with your ESP8266 IP and endpoint
            const xhr = new XMLHttpRequest();

            xhr.open("GET", url, true);
            xhr.onreadystatechange = function () {
                if (xhr.readyState === 4 && xhr.status === 200) {
                    const response = xhr.responseText;
                    // Assuming the response is a JSON string with four variables
                    const data = JSON.parse(response);

                    // Display the received values
                    document.getElementById("receivedValues").innerHTML =
                        `Temp 1 (Ambient): ${data.mixtemp1}<br>
                         Temp 2 (Mix Tank): ${data.mixtemp2}<br>
                         Uptime Hours: ${data.mixuphrs}<br>
                         Uptime Minutes: ${data.mixupmins}`;
                }
            };
            xhr.send();
}
</script>
</center>
</body>
</html>
)=====";