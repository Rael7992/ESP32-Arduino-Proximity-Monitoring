#include <WiFi.h>
#include <WebServer.h>

HardwareSerial MySerial(2);

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

String modeInfo = "Waiting...";
String distanceInfo = "Waiting...";
String stateInfo = "Waiting...";

String webpage()
{
  return R"rawliteral(
<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>ESP32 Dashboard</title>

<style>

body{
    font-family:Arial;
    background:#0f172a;
    color:white;
    text-align:center;
    padding:20px;
}

h1{
    color:#38bdf8;
}

.card{
    background:#1e293b;
    padding:20px;
    margin:15px auto;
    border-radius:12px;
    width:260px;
}

.value{
    font-size:22px;
    font-weight:bold;
    margin-top:10px;
}

</style>

</head>

<body>

<h1>ESP32 Live Dashboard</h1>

<div class="card">
    Mode
    <div class="value" id="mode">Loading...</div>
</div>

<div class="card">
    Distance
    <div class="value" id="distance">Loading...</div>
</div>

<div class="card">
    State
    <div class="value" id="state">Loading...</div>
</div>

<script>

function updateData()
{
    fetch("/data?t=" + Date.now())
    .then(response => response.json())
    .then(data =>
    {
        document.getElementById("mode").textContent = data.mode;
        document.getElementById("distance").textContent = data.distance + " cm";
        document.getElementById("state").textContent = data.state;
    })
    .catch(error => console.log(error));
}

updateData();
setInterval(updateData, 500);

</script>

</body>

</html>
)rawliteral";
}

void handleRoot()
{
    server.send(200, "text/html", webpage());
}

void handleData()
{
    String json = "{";
    json += "\"mode\":\"" + modeInfo + "\",";
    json += "\"distance\":\"" + distanceInfo + "\",";
    json += "\"state\":\"" + stateInfo + "\"";
    json += "}";

    server.send(200, "application/json", json);
}

void setup()
{
    Serial.begin(115200);

    // Arduino TX -> ESP32 RX16
    // Arduino RX -> ESP32 TX17
    MySerial.begin(115200, SERIAL_8N1, 16, 17);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    server.on("/", handleRoot);
    server.on("/data", handleData);

    server.begin();

    Serial.println();
    Serial.println("====================================");
    Serial.println("      ESP32 DASHBOARD READY");
    Serial.print("      http://");
    Serial.println(WiFi.localIP());
    Serial.println("====================================");
}

void loop()
{
    server.handleClient();

    if (MySerial.available())
    {
        String msg = MySerial.readStringUntil('\n');
        msg.trim();

        int modeStart = msg.indexOf("[MODE]: ");
        int distStart = msg.indexOf(" | DIST: ");
        int stateStart = msg.indexOf(" | STATE: ");

        if (modeStart != -1 && distStart != -1 && stateStart != -1)
        {
            modeInfo = msg.substring(modeStart + 8, distStart);
            distanceInfo = msg.substring(distStart + 8, stateStart);
            stateInfo = msg.substring(stateStart + 10);
        }
    }
}
