/* This is the second section of the program. At first, the "permanent" file of the ESP32 is acessed to retrieve the information. Then, two json files are created with all available BLE devices and Wi-Fi networks as they are scanned. Afterwards the json files containing all devices and networks found are sent to the server */

#include "WiFi.h"
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <Preferences.h>
#include <BLEAdvertisedDevice.h>

#define DEBUG 1

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

// https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
Preferences preferences;

String ssid;
String password;
String serverName;
String tagName;
String WIFIinterval;
String BLEinterval;

// https://techtutorialsx.com/2017/08/20/esp32-arduino-freertos-queues/
QueueHandle_t queueWIFI;
QueueHandle_t queueBLE;

TaskHandle_t Task1, Task2, Task3, Task4;

//Counter for BLE devices scanned
int number = 1;

BLEScan* pBLEScan;

int response200Wifi = 0;
int response200Ble = 0;
int responseOtherWifi = 0;
int responseOtherBle = 0;

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

  int scanTime = 5; //In seconds
  // put your main code here, to run repeatedly:
  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionarBLE += "[";
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionarBLE += "]}";
  //debug("[BLE] Devices found: ");
  //debugln(foundDevices.getCount());
  number = 1;
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

  //It is necessary to remove the last comma
  int lastComma = adicionarBLE.lastIndexOf(',');
  adicionarBLE.remove(lastComma , 1);
  adicionarBLECopy = adicionarBLE;
  adicionarBLE.remove(0);
  xQueueOverwrite(queueBLE, &adicionarBLECopy);
}

void scanNetworks() {

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

  //debug("[WI-FI] networks found: ");
  //debugln(numberOfNetworks);
}

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
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      if (connectionCountBLE == 0) {
        http.begin(client, serverName);
      } connectionCountBLE++;

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded", "Content-Length", tamanhoBLE);
      //http.addHeader("Content-Length", tamanhoBLE);
      http.addHeader("Accept", "*/*");
      http.addHeader("Accept-Encoding", "gzip, deflate, br");
      http.addHeader("Connection", "keep-alive");

      vTaskSuspend(Task3);
      int httpResponseCodeBLE = http.POST(dataGramaBLEFinal);
      vTaskResume(Task3);

      String responseBLE = http.getString();

      if (httpResponseCodeBLE == 200) {
        /*debug("[BLE] HTTP Response code: ");
          debugln(httpResponseCodeBLE);
          debugln(responseBLE);
        */
        response200Ble++;
      }
      else {
        debug("[BLE] HTTP Response code: ");
        debugln(httpResponseCodeBLE);
        debugln(responseBLE);
        responseOtherBle++;
      }

    }
  }
}



void postWIFI() {

  xQueueReceive(queueWIFI, &adicionarWIFICopy, portMAX_DELAY);

  if ( adicionarWIFICopy.length() > 0) {

    // Here we concatenate both parts to originate the final datagram to be sent
    String dataGramaWIFIFinal = dataGramaWIFI + adicionarWIFICopy;
    int tamanhoWIFI = dataGramaWIFIFinal.length();

    int connectionCountWIFI = 0;

    //Send an HTTP POST request every 1 second

    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      if (connectionCountWIFI == 0) {
        http.begin(client, serverName);
      } connectionCountWIFI++;

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded", "Content-Length", tamanhoWIFI);
      //http.addHeader("Content-Length", tamanhoWIFI);
      http.addHeader("Accept", "*/*");
      http.addHeader("Accept-Encoding", "gzip, deflate, br");
      http.addHeader("Connection", "keep-alive");

      // Send HTTP POST request

      vTaskSuspend(Task3);
      int httpResponseCodeWIFI = http.POST(dataGramaWIFIFinal);
      vTaskResume(Task3);

      String responseWifi = http.getString();

      if (httpResponseCodeWIFI == 200) {
        /*debug("[WIFI] HTTP Response code: ");
          debugln(httpResponseCodeWIFI);
          debugln(responseWifi);
        */
        response200Wifi++;
      }
      else {
        debug("[WIFI] HTTP Response code: ");
        debugln(httpResponseCodeWIFI);
        debugln(responseWifi);
        responseOtherWifi++;
      }

    }
  }
}

void codeForTask1( void * parameter ) {
  for (;;) {

    scanBLE();

    if (BLEinterval.toInt() > 0) {
      vTaskDelay(BLEinterval.toInt());
    }
  }
}

void codeForTask2( void * parameter ) {
  for (;;) {
    postBLE();
    //https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time
    vTaskDelay(500);
  }
}

void codeForTask3( void * parameter ) {
  for (;;) {
    scanNetworks();

    if (WIFIinterval.toInt() > 0) {
      vTaskDelay(WIFIinterval.toInt());
    }
  }
}

void codeForTask4( void * parameter ) {
  for (;;) {
    postWIFI();
    //https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time
    vTaskDelay(500);
  }

}

void setup() {

  Serial.begin(115200);

  queueWIFI = xQueueCreate( 1, 20 );
  queueBLE = xQueueCreate( 1, 20 );

  if (queueWIFI == NULL) {
    debugln("Error creating the queue");
  }

  if (queueBLE == NULL) {
    debugln("Error creating the queue");
  }

  preferences.begin("credentials", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  serverName = preferences.getString("serverName", "");
  tagName = preferences.getString("tagName", "");
  WIFIinterval = preferences.getString("WIFIinterval", "");
  BLEinterval = preferences.getString("BLEinterval", "");

  if (ssid == "" || password == "" || serverName == "" || tagName == "" || WIFIinterval == "" || BLEinterval == "") {
    Serial.println("No values saved for credentials");
  }
  else {
    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
    }
    debugln("Connected to network");
  }

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

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
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
  xTaskCreatePinnedToCore(    codeForTask1,    "scanBLE",    4000,    NULL,    4,    &Task1,    0);
  delay(500);
  xTaskCreatePinnedToCore(    codeForTask2,    "postBLE",    4000,    NULL,    2,    &Task2,    0);
  delay(500);
  xTaskCreatePinnedToCore(    codeForTask3,    "scanNetworks",    4000,    NULL,    4,    &Task3,    1);
  delay(500);
  xTaskCreatePinnedToCore(    codeForTask4,    "postWIFI",    4000,    NULL,   2,    &Task4,    1);
  delay(500);
}

//The task which runs setup() and loop() is created on core 1 with priority 1.
void loop() {

  // https://www.norwegiancreations.com/2017/12/arduino-tutorial-serial-inputs/
  String command;

  if (Serial.available()) {
    command = Serial.readStringUntil('\n');

    if (command.equals("Results")) {
      debugln("------------------------------------------------------------");
      debug("200 Wi-Fi Responses: ");
      debugln(response200Wifi);
      debug("Other Wi-Fi Responses: ");
      debugln(responseOtherWifi);
      debug("200 BLE Responses: ");
      debugln(response200Ble);
      debug("Other BLE Responses: ");
      debugln(responseOtherBle);
      debugln("------------------------------------------------------------");
    }

    else {
      debugln("------------------------------------------------------------");
      Serial.println("Invalid command");
      debugln("------------------------------------------------------------");
    }
  }

  vTaskDelay(1000);

  //vTaskDelete(NULL);
}
// to delete that task and free its resources because it isn't planed to use it at all.
