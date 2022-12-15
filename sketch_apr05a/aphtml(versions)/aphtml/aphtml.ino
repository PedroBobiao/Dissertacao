//https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssidC = "ZON-C510";
const char* passwordC = "Boas1234";

const char* PARAM_SSID = "inputSsid";
const char* PARAM_PASSW = "inputPassw";
const char* PARAM_SERVER = "inputServer";
const char* PARAM_TAG = "inputTag";
const char* PARAM_WIFI = "inputWifi";
const char* PARAM_BLE = "inputBle";
const char* PARAM_URL = "inputUrl";

// HTML web page to handle 7 input fields
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP32 Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP32 SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <h2>ESP32 Input Form</h2>
  <form action="/get" target="hidden-form">
    inputSsid (current value %inputSsid%): <input type="text" name="inputSsid">
    <input type="submit" value="Update" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputPassw (current value %inputPassw%): <input type="text" name="inputPassw">
    <input type="submit" value="Update" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputServer (current value %inputServer%): <input type="text" name="inputServer">
    <input type="submit" value="Update" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputTag (current value %inputTag%): <input type="text" name="inputTag">
    <input type="submit" value="Update" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputWifi (current value %inputWifi%): <input type="number " name="inputWifi">
    <input type="submit" value="Update" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputBle (current value %inputBle%): <input type="number " name="inputBle">
    <input type="submit" value="Update" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputUrl (current value %inputUrl%): <input type="text" name="inputUrl">
    <input type="submit" value="Update" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
  <p>If you click the "Submit" button, the form-data will be sent to the ESP32 SPIFFS System.</p>
  <p>If you click the "Start" button, the tag will start scanning and posting data to the ILS Server.</p>
  <form action="http://ils.dsi.uminho.pt/viewData/">
    <input type="submit" value="Start" />
</form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var) {
  //Serial.println(var);
  if (var == "inputSsid") {
    return readFile(SPIFFS, "/inputSsid.txt");
  }
  else if (var == "inputPassw") {
    return readFile(SPIFFS, "/inputPassw.txt");
  }
  else if (var == "inputServer") {
    return readFile(SPIFFS, "/inputServer.txt");
  }
  else if (var == "inputTag") {
    return readFile(SPIFFS, "/inputTag.txt");
  }
  else if (var == "inputWifi") {
    return readFile(SPIFFS, "/inputWifi.txt");
  }
  else if (var == "inputBle") {
    return readFile(SPIFFS, "/inputBle.txt");
  }
  else if (var == "inputUrl") {
    return readFile(SPIFFS, "/inputUrl.txt");
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidC, passwordC);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_SSID)) {
      inputMessage = request->getParam(PARAM_SSID)->value();
      writeFile(SPIFFS, "/inputSsid.txt", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(PARAM_PASSW)) {
      inputMessage = request->getParam(PARAM_PASSW)->value();
      writeFile(SPIFFS, "/inputPassw.txt", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(PARAM_SERVER)) {
      inputMessage = request->getParam(PARAM_SERVER)->value();
      writeFile(SPIFFS, "/inputServer.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_TAG)) {
      inputMessage = request->getParam(PARAM_TAG)->value();
      writeFile(SPIFFS, "/inputTag.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_WIFI)) {
      inputMessage = request->getParam(PARAM_WIFI)->value();
      writeFile(SPIFFS, "/inputWifi.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_BLE)) {
      inputMessage = request->getParam(PARAM_BLE)->value();
      writeFile(SPIFFS, "/inputBle.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_URL)) {
      inputMessage = request->getParam(PARAM_URL)->value();
      writeFile(SPIFFS, "/inputUrl.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {}
