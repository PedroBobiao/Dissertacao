//https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/

/* This program starts by acessing a webserver and withdrawing it's information. Then the information is parsed and stored in the permanent storage of the ESP32. Then, the "permanent" file of the ESP32 is acessed to retrieve the information. After that two json files are created with all available BLE devices and Wi-Fi networks as they are scanned. Afterwards the json files containing all devices and networks found are sent to the webserver */

#include <Arduino.h>
#include <AsyncTCP.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <WiFi.h>

// https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
Preferences preferences;

// https://www.youtube.com/watch?v=--KxxMaiwSE
#define DEBUG 1

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#define debugf(x , y) Serial.printf(x , y)
#else
#define debug(x)
#define debugln(x)
#define debugf(x , y)
#endif

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssidC = "TP-Link_60AE";
const char* passwordC = "64620572";

const char* PARAM_SSID = "inputSsid";
const char* PARAM_PASSW = "inputPassw";
const char* PARAM_SERVER = "inputServer";
const char* PARAM_TAG = "inputTag";
const char* PARAM_WIFI = "inputWifi";
const char* PARAM_BLE = "inputBle";
const char* PARAM_URL = "inputUrl";
const char* PARAM_STATE = "inputState";

String ssid;
String password;
String serverName;
String tagName;
String wifiInterval;
String bleInterval;
String jsonURL;
String state = "1";

int stateCounter = 0;

// https://techtutorialsx.com/2017/08/20/esp32-arduino-freertos-queues/
QueueHandle_t queueWIFI;
QueueHandle_t queueBLE;

TaskHandle_t Task1, Task2, Task3, Task4;

//Counter for BLE devices scanned
int number = 1;

unsigned long totalScanWifi;

//Can be changed, in seconds
int scanTimeBle = 3;

BLEScan* pBLEScan;

String translateEncryptionType(wifi_auth_mode_t encryptionType) {

  switch (encryptionType) {
    case (WIFI_AUTH_OPEN):
      return "Open";
    case (WIFI_AUTH_WEP):
      return "WEP";
    case (WIFI_AUTH_WPA_PSK):
      return "WPA_PSK";
    case (WIFI_AUTH_WPA2_PSK):
      return "WPA2_PSK";
    case (WIFI_AUTH_WPA_WPA2_PSK):
      return "WPA_WPA2_PSK";
    case (WIFI_AUTH_WPA2_ENTERPRISE):
      return "WPA2_ENTERPRISE";
  }
}

// Here the "static" part of the json file is initialized. It has several preset values that will be replaced as we scan the bluetooth networks.
String dataGramaBLE = "scanData= {  \"tagName\": \"tagPedro\",  \"tagBSSID\": \"00:11:22:33:44:99\",  \"tagNetwork\": \"eduroam\",  \"dataType\": \"BLE\",  \"scanMode\": \"auto\", \"BLEData\":";

// Here the "volatile" part of the json file is initialized. This part will be changing constantly
String adicionarBLE;

String adicionarBLECopy;

// Here the "static" part of the json file is initialized. It has several preset values that will be replaced as we scan the wi-fi networks.
String dataGramaWIFI = "scanData= { \"tagName\": \"tagPedro\", \"tagBSSID\": \"00:11:22:33:44:55\", \"tagNetwork\": \"eduroam\", \"dataType\": \"Wi-Fi\", \"scanMode\": \"auto\", \"WiFiData\":";

String adicionarWIFICopy;

void replaceAll() {
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  serverName = preferences.getString("serverName", "");
  tagName = preferences.getString("tagName", "");
  wifiInterval = preferences.getString("WIFIinterval", "");
  bleInterval = preferences.getString("BLEinterval", "");
  jsonURL = preferences.getString("jsonURL", "");

  // In the datagram to be sent it is required to identify which network the device is connected to and it's ssid
  dataGramaWIFI.replace("eduroam", (WiFi.SSID()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGramaWIFI.replace("00:11:22:33:44:55", (WiFi.macAddress()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGramaWIFI.replace("tagPedro", tagName);

  // In the datagram to be sent it is required to identify which network the device is connected to and it's ssid
  dataGramaBLE.replace("eduroam", (WiFi.SSID()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGramaBLE.replace("00:11:22:33:44:99", (WiFi.macAddress()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGramaBLE.replace("tagPedro", tagName);
}

void connectToNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidC, passwordC);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    debugln("WiFi Failed!");
    return;
  }
  debugln();
  debug("IP Address: ");
  debugln(WiFi.localIP());
}

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
    function myFunction() {
      document.getElementById("demo").innerHTML = "Click on ILS Server to visualize the data";
    }
  </script></head><body>
  <h2>ESP32 Input Form</h2>
  <p>If you click on the "Update" buttons, the form-data will be sent to the ESP32 SPIFFS System.</p>
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
  </form><br>
  <p>If you click on the "Start" button, the tag will start scanning and posting data to the ILS Server.</p>
  <form action="/get" target="hidden-form">
    <input type="submit" value="Start" name="inputState" onclick="myFunction()">
  </form>
  <p id="demo"></p>
  <br>
  <form action="http://ils.dsi.uminho.pt/viewData/">
    <input type="submit" value="ILS Server">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path) {
  //debugf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    //debugln("- empty file or failed to open file");
    return String();
  }
  //debugln("- read from file:");
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  file.close();
  //debugln(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  //debugf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file) {
    //debugln("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //debugln("- file written");
  } else {
    //debugln("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var) {
  //debugln(var);
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
  else if (var == "inputState") {
    return readFile(SPIFFS, "/inputState.txt");
  }
  return String();
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    void onResult(BLEAdvertisedDevice advertisedDevice) {

      // The bluetooth data containing information about all the networks found has to be sent inside brackets
      adicionarBLE += "{\"bssid\": \"bssid(i)\",      \"rssi\": \"rssi(i)\",      \"ssid\": \"ssid(i)\"}";

      // The preset value of bssid is replaced by the bssid of the device found
      adicionarBLE.replace("bssid(i)", (advertisedDevice.getAddress().toString().c_str()));
      String rssiStr = String (advertisedDevice.getRSSI());
      // The preset value of rssi is replaced by the rssi of the device found
      adicionarBLE.replace("rssi(i)", rssiStr);
      String nome = advertisedDevice.getName().c_str();
      // The preset value of ssid is replaced by the ssid of the device found
      adicionarBLE.replace("ssid(i)", nome);
      number ++;
      adicionarBLE += ", ";
    }
};

void scanBLE() {

  // put your main code here, to run repeatedly:
  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionarBLE += "[";
  BLEScanResults foundDevices = pBLEScan->start(scanTimeBle, false);
  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionarBLE += "]}";
  number = 1;
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

  //It is necessary to remove the last comma
  int lastComma = adicionarBLE.lastIndexOf(',');
  adicionarBLE.remove(lastComma , 1);
  adicionarBLECopy = adicionarBLE;
  adicionarBLE.remove(0);
  xQueueOverwrite(queueBLE, &adicionarBLECopy);
}

void scanWIFI() {

  unsigned long scanWifiA = millis();
  // Here the "volatile" part of the json file is initialized. This part will be changing constantly
  String adicionarWIFI;
  int linhaUm = 1;
  int numberOfNetworks = WiFi.scanNetworks();

  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionarWIFI += "[";

  for (int i = 0; i < numberOfNetworks; i++) {
    if (linhaUm == 0) {
      adicionarWIFI += ", ";
    } else linhaUm = 0;

    adicionarWIFI += "{\"bssid\": \"bssid(i)\",      \"rssi\": \"rssi(i)\",      \"ssid\": \"ssid(i)\",      \"encript\": \"encript(i)\"}";

    // The preset value of bssid is replaced by the bssid of the network number i found
    adicionarWIFI.replace("bssid(i)", (WiFi.BSSIDstr(i)));

    String rssiStr = String (WiFi.RSSI(i));
    // The preset value of rssi is replaced by the rssi of the network number i found
    adicionarWIFI.replace("rssi(i)", rssiStr);

    // The preset value of ssid is replaced by the ssid of the network number i found
    adicionarWIFI.replace("ssid(i)", (WiFi.SSID(i)));

    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));

    // The preset value of encryption is replaced by the encryption of the network number i found
    adicionarWIFI.replace("encript(i)", encryptionTypeDescription);

  }

  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionarWIFI += "]}";

  adicionarWIFICopy = adicionarWIFI;

  adicionarWIFI.remove(0);

  xQueueOverwrite(queueWIFI, &adicionarWIFICopy);

  unsigned long scanWifiB = millis();
  totalScanWifi = scanWifiB - scanWifiA;
}

//https://randomnerdtutorials.com/esp32-http-get-post-arduino/#http-post

void postBLE() {

  xQueueReceive(queueBLE, &adicionarBLECopy, portMAX_DELAY);

  if ( adicionarBLECopy.length() > 0) {

    // Send HTTP POST request
    // Here we concatenate both parts to originate the final datagram to be sent
    String dataGramaBLEFinal = dataGramaBLE + adicionarBLECopy;

    int tamanhoBLE = dataGramaBLEFinal.length();

    int connectionCountBLE = 0;

    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient clientBle;
      HTTPClient httpBle;

      // Your Domain name with URL path or IP address with path
      if (connectionCountBLE == 0) {
        httpBle.begin(clientBle, serverName);
      } connectionCountBLE++;

      // Specify content-type header
      httpBle.addHeader("Cache-Control", "no-cache");
      httpBle.addHeader("Content-Type", "application/x-www-form-urlencoded", "Content-Length", tamanhoBLE);
      //http.addHeader("Content-Length", tamanhoBLE);
      httpBle.addHeader("Accept", "*/*");
      httpBle.addHeader("Accept-Encoding", "gzip, deflate, br");
      httpBle.addHeader("Connection", "keep-alive");

      int httpResponseCodeBLE = httpBle.POST(dataGramaBLEFinal);

      String responseBLE = httpBle.getString();

      debug("[BLE] HTTP Response code: ");
      debugln(httpResponseCodeBLE);
      debugln(responseBLE);
      debugln("------------------------------------------------------------");


      //httpBle.end();
    }
  }
}

//https://randomnerdtutorials.com/esp32-http-get-post-arduino/#http-post

void postWIFI() {

  xQueueReceive(queueWIFI, &adicionarWIFICopy, portMAX_DELAY);

  if ( adicionarWIFICopy.length() > 0) {

    // Send HTTP POST request
    // Here we concatenate both parts to originate the final datagram to be sent
    String dataGramaWIFIFinal = dataGramaWIFI + adicionarWIFICopy;

    int tamanhoWIFI = dataGramaWIFIFinal.length();

    int connectionCountWIFI = 0;

    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient clientWifi;
      HTTPClient httpWifi;

      // Your Domain name with URL path or IP address with path
      if (connectionCountWIFI == 0) {
        httpWifi.begin(clientWifi, serverName);
      } connectionCountWIFI++;

      // Specify content-type header
      httpWifi.addHeader("Cache-Control", "no-cache");
      httpWifi.addHeader("Content-Type", "application/x-www-form-urlencoded", "Content-Length", tamanhoWIFI);
      //http.addHeader("Content-Length", tamanhoWIFI);
      httpWifi.addHeader("Accept", "*/*");
      httpWifi.addHeader("Accept-Encoding", "gzip, deflate, br");
      httpWifi.addHeader("Connection", "keep-alive");

      int httpResponseCodeWIFI = httpWifi.POST(dataGramaWIFIFinal);

      String responseWifi = httpWifi.getString();


      debug("[Wi-Fi] HTTP Response code: ");
      debugln(httpResponseCodeWIFI);
      debugln(responseWifi);
      debugln("------------------------------------------------------------");

      //httpWifi.end();
    }
  }
}

void boas() {

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
      preferences.putString("ssid", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(PARAM_PASSW)) {
      inputMessage = request->getParam(PARAM_PASSW)->value();
      writeFile(SPIFFS, "/inputPassw.txt", inputMessage.c_str());
      preferences.putString("password", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(PARAM_SERVER)) {
      inputMessage = request->getParam(PARAM_SERVER)->value();
      writeFile(SPIFFS, "/inputServer.txt", inputMessage.c_str());
      preferences.putString("serverName", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_TAG)) {
      inputMessage = request->getParam(PARAM_TAG)->value();
      writeFile(SPIFFS, "/inputTag.txt", inputMessage.c_str());
      preferences.putString("tagName", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_WIFI)) {
      inputMessage = request->getParam(PARAM_WIFI)->value();
      writeFile(SPIFFS, "/inputWifi.txt", inputMessage.c_str());
      preferences.putString("WIFIinterval", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_BLE)) {
      inputMessage = request->getParam(PARAM_BLE)->value();
      writeFile(SPIFFS, "/inputBle.txt", inputMessage.c_str());
      preferences.putString("BLEinterval", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_URL)) {
      inputMessage = request->getParam(PARAM_URL)->value();
      writeFile(SPIFFS, "/inputUrl.txt", inputMessage.c_str());
      preferences.putString("jsonURL", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_STATE)) {
      inputMessage = request->getParam(PARAM_STATE)->value();
      writeFile(SPIFFS, "/inputState.txt", "0");
      preferences.putString("state", "0");
    }
    else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/text", inputMessage);
  });

  server.onNotFound(notFound);
  server.begin();

  //server.on("/", HTTP_Delete, [](AsyncWebServerRequest * request));
}

void codeForTask1( void * parameter ) {
  for (;;) {
    state = preferences.getString("state", "");
    if (state == "0") {
      if (stateCounter = 0) {

        delay(100);
        replaceAll();
        delay(100);
        server.end();
        delay(100);
        SPIFFS.end();
        delay(100);
        stateCounter++;
      }

      else {

        if (bleInterval.toInt() >= 0) {
          scanBLE();

          int waitBle;
          waitBle = bleInterval.toInt() - (scanTimeBle * 1000);

          if (waitBle >= 0 && waitBle <= bleInterval.toInt()) {
            delay(waitBle);
          }
          else {
            delay(100);
          }
        }

        else {
          debugln("BLE Disabled");
          debugln("------------------------------------------------------------");
          vTaskDelete(Task1);
          vTaskDelete(Task2);
        }
      }
    }
    else {
      delay(100);
    }
  }
}

void codeForTask2( void * parameter ) {
  for (;;) {
    if (state == "0") {
      if (wifiInterval.toInt() >= 0) {
        scanWIFI();

        unsigned long waitWifi;
        waitWifi = wifiInterval.toInt() - totalScanWifi;

        if (waitWifi >= 0 && waitWifi <= wifiInterval.toInt()) {
          delay(waitWifi);
          //if no intervals are used it is needed to suspend wifiScans in order for the posts to be successful but there can also be concurring suspends and resumes
        }
        else {
          delay(100);
        }
      }

      else {
        debugln("Wi-Fi Disabled");
        debugln("------------------------------------------------------------");
        vTaskDelete(Task3);
        vTaskDelete(Task4);
      }
    }
    else {
      delay(100);
    }
  }
}

void codeForTask3( void * parameter ) {
  for (;;) {
    if (state == "0") {
      postBLE();
      //https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time
      delay(100);
    }
    else {
      delay(100);
    }
  }
}

void codeForTask4( void * parameter ) {
  for (;;) {
    if (state == "0") {
      postWIFI();
      //https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time
      delay(100);
    }
    else {
      delay(100);
    }
  }
}

void setup() {
  Serial.begin(115200);

  connectToNetwork();

  preferences.begin("credentials", false);
  preferences.putString("state", "1");

  if (!SPIFFS.begin(true)) {
    debugln("An Error has occurred while mounting SPIFFS");
    return;
  }

  writeFile(SPIFFS, "/inputState.txt", "1");

  boas();

  queueWIFI = xQueueCreate( 1, 20 );
  queueBLE = xQueueCreate( 1, 20 );

  if (queueWIFI == NULL) {
    debugln("Error creating the queue");
    debugln("------------------------------------------------------------");
  }

  if (queueBLE == NULL) {
    debugln("Error creating the queue");
    debugln("------------------------------------------------------------");
  }

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  //https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
  pinMode(D9, OUTPUT);
  /* create Mutex */
  /*Syntax for assigning task to a core:
    xTaskCreatePinnedToCore(
                    TaskFunc,     // Function to implement the task
                    "TaskLabel",  // Name of the task
                    10000,        // Stack size in bytes
                    NULL,         // Task input parameter
                    0,            // Priority of the task
                    &TaskHandle,  // Task handle.
                    TaskCore);    // Core where the task should run
  */
  //Both tasks run on the same core so different priorities are attributed to each
  xTaskCreatePinnedToCore(    codeForTask1,    "scanBLE",    10000,    NULL,    1,    &Task1,    0);
  delay(100);
  xTaskCreatePinnedToCore(    codeForTask2,    "scanWIFI",    10000,    NULL,   1,    &Task2,    1);
  delay(100);
  xTaskCreatePinnedToCore(    codeForTask3,    "postBLE",    10000,    NULL,    1,    &Task3,    0);
  delay(100);
  xTaskCreatePinnedToCore(    codeForTask4,    "postWIFI",    10000,    NULL,   1,    &Task4,    1);
  delay(100);
}

void loop() {
  //The task which runs setup() and loop() is created on core 1 with priority 1.
  vTaskDelete(NULL);
  // to delete that task and free its resources because it isn't planed to use it at all.
}
