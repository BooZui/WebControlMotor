#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <string.h>

// Replace with your network credentials
const char *ssid = "BooZuiESP";
const char *password = "24042004";

bool ledState = 0;
const int moter1Pin1 = 2;
const int moter1Pin2 = 3;
const int moter1Pin3 = 4;
const int moter1Pin4 = 5;
const int moter1Pin = 6;

const int moter2Pin = 7;
int moter1 = 0;
int moter2 = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    .container {
      padding: 0 5vw 0 5vw;
      height: 100vh;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

     input[type=range] {
            -webkit-appearance: none; /* Remove default styling */
            width: 300px; /* Set the width of the track */
            height: 50px; /* Set the height of the track */
            background: #ddd; /* Background color of the track */
            border-radius: 20px; /* Rounded corners */
            outline: none; /* Remove the outline */
            opacity: 0.7; /* Slightly transparent */
            transition: opacity 0.2s; /* Smooth transition on hover */
        }

        /* Style the range input thumb */
        input[type=range]::-webkit-slider-thumb {
            -webkit-appearance: none; /* Remove default styling */
            appearance: none; /* Remove default styling */
            width: 70px; /* Set the width of the thumb */
            height: 70px; /* Set the height of the thumb */
            background: #4CAF50; /* Background color of the thumb */
            cursor: pointer; /* Change cursor to pointer */
            border-radius: 9999px; /* Rounded thumb */
        }

        input[type=range]::-moz-range-thumb {
            width: 25px; /* Set the width of the thumb */
            height: 25px; /* Set the height of the thumb */
            background: #4CAF50; /* Background color of the thumb */
            cursor: pointer; /* Change cursor to pointer */
            border-radius: 9999px; /* Rounded thumb */
        }

        /* Change the track color when hovered */
        input[type=range]:hover {
            opacity: 1; /* Full opacity */
        }
  </style>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="container">
      <div class="move">
        <input type="range" id="slider1" min="-100" max="100" value="0" class="slider1"></input>
        <p id="progress-text1">0</p>
      </div>

      <div class="direction">
        <input type="range" id="slider2" min="0" max="100" value="0" class="slider2"></input>
        <p id="progress-text2">0</p>
      </div>
  </div>
<script>
  const progressText1 = document.getElementById("progress-text1");
  const progressText2 = document.getElementById("progress-text2");
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  var data1 = 0;
  var data2 = 0;

  window.addEventListener("load", onLoad);

  function initWebSocket() {
    console.log("Trying to open a WebSocket connection...");
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }

  function onOpen(event) {
    console.log("Connection opened");
  }

  function onClose(event) {
    console.log("Connection closed");
    setTimeout(initWebSocket, 2000);
  }

  function onMessage(event) {
    var state;
    if (event.data == "1") {
      state = "ON";
    } else {
      state = "OFF";
    }
    document.getElementById("state").innerHTML = state;
  }

  function onLoad(event) {
    initWebSocket();
    initSlider();
  }

  function initSlider() {
    const slider1 = document.getElementById("slider1");
    slider1.addEventListener("input", () => {
      const value = slider1.value;
      progressText1.textContent = value;
      data1 = value;
      toggle()
    });

    const slider2 = document.getElementById("slider2");
    slider2.addEventListener("input", () => {
      const value = slider2.value;
      progressText2.textContent = value;
      data2 = value;
      toggle();
    });

  }

  function toggle() {
    websocket.send(`${data1} ${data2}`);
  }
</script>
</body>
</html>
)rawliteral";

void notifyClients() {
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String target = String((char *)data);
    sscanf(target.c_str(), "%d %d", &moter1, &moter2);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String &var) {
  Serial.println(var);
  if (var == "STATE") {
    if (ledState) {
      return "ON";
    } else {
      return "OFF";
    }
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(921600);

  pinMode(moter1Pin, OUTPUT);
  pinMode(moter1Pin1, OUTPUT);
  pinMode(moter1Pin2, OUTPUT);
  pinMode(moter1Pin3, OUTPUT);
  pinMode(moter1Pin4, OUTPUT);
  pinMode(moter2Pin, OUTPUT);

  analogWrite(moter1Pin, 0);
  digitalWrite(moter1Pin1, LOW);
  digitalWrite(moter1Pin2, LOW);
  digitalWrite(moter1Pin3, LOW);
  digitalWrite(moter1Pin4, LOW);

  WiFi.softAP(ssid, password);

  Serial.println(WiFi.localIP());

  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.begin();
}

void loop() {
  ws.cleanupClients();

  int moter1Value = 0;

  if (moter1 > 0) {
    moter1Value = map(moter1, 0, 100, 0, 255);
    digitalWrite(moter1Pin1, HIGH);
    digitalWrite(moter1Pin2, HIGH);
    digitalWrite(moter1Pin3, LOW);
    digitalWrite(moter1Pin4, LOW);
  } else {
    moter1Value = map(-moter1, 0, 100, 0, 255);
    digitalWrite(moter1Pin1, LOW);
    digitalWrite(moter1Pin2, LOW);
    digitalWrite(moter1Pin3, HIGH);
    digitalWrite(moter1Pin4, HIGH);
  }

  analogWrite(moter1Pin, moter1Value);

  int moter2Value = map(moter2, 0, 100, 0, 255);
  analogWrite(moter2Pin, moter2Value);

  delay(1);
}
