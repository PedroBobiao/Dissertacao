/* In this program two json files are created with all available BLE devices and Wi-Fi networks as they are scanned.
  Then the json files containing all devices and networks found are sent to the server */

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

Preferences preferences;

QueueHandle_t queueWIFI;
QueueHandle_t queueBLE;

TaskHandle_t Task1, Task2, Task3, Task4;

//Counter for BLE devices scanned
int number = 1;

BLEScan* pBLEScan;

//TIMERS
int WIFIinterval = 0;
int BLEinterval = 0;

int response200Wifi = 0;
int response200Ble = 0;
int responseOtherWifi = 0;
int responseOtherBle = 0;

const char* ssid = "ZON-C510";
const char* password =  "Boas1234";

// Domain Name with full URL Path for HTTP POST Request
const char* serverName = "http://ils.dsi.uminho.pt/ar-ware/S02/i2a/i2aSamples.php";


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

void connectToNetwork() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    debugln("Establishing connection to WiFi..");
  }

  debugln("Connected to network");

}

//Get the Information from the server
void getInfo() {

  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

    HTTPClient http;

    http.begin("http://192.168.1.6/"); //Specify the URL (https://serverfault.com/questions/209150/how-to-access-localhost-by-ip-address#:~:text=To%20access%20the%20server%20from,)%20by%20running%20hostname%20%2DI%20.)
    int httpCode = http.GET();                                        //Make the request

    if (httpCode > 0) { //Check for the returning code

      String payload = http.getString();
      String indice = payload;
      Serial.println(httpCode);
      Serial.println(payload);

      int indice1;
      int indice2;
      int indice3;
      int indice4;
      int indice5;
      int indice6;
      int indice7;
      int indice8;
      int indice9;
      int indice10;
      int indice11;
      int indice12;

      indice1 = (indice.indexOf("ssid") + 8);
      indice2 = (indice.indexOf("password") - 6);

      indice3 = (indice.indexOf("password") + 12);
      indice4 = (indice.indexOf("serverName") - 6);

      indice5 = (indice.indexOf("serverName") + 14);
      indice6 = (indice.indexOf("tagName") - 6);

      indice7 = (indice.indexOf("tagName") + 11);
      indice8 = (indice.indexOf("WIFIinterval") - 6);

      indice9 = (indice.indexOf("WIFIinterval") + 15);
      indice10 = (indice.indexOf("BLEinterval") - 5);

      indice11 = (indice.indexOf("BLEinterval") + 14);
      indice12 = (indice.lastIndexOf("}"));

      String ssidGot;
      ssidGot = payload.substring(indice1, indice2);
      Serial.println(ssidGot);

      String passwordGot;
      passwordGot = payload.substring(indice3, indice4);
      Serial.println(passwordGot);

      String serverNameGot;
      serverNameGot = payload.substring(indice5, indice6);
      Serial.println(serverNameGot);

      String tagNameGot;
      tagNameGot = payload.substring(indice7, indice8);
      Serial.println(tagNameGot);

      String WIFIintervalGot;
      WIFIintervalGot = payload.substring(indice9, indice10);
      Serial.println(WIFIintervalGot);

      String BLEintervalGot;
      BLEintervalGot = payload.substring(indice11, indice12);
      Serial.println(BLEintervalGot);
    }

    else {
      Serial.println("Error on HTTP request");
    }

    http.end(); //Free the resources
  }

  /*preferences.begin("credentials", false);
    preferences.putString("serverName", payload.get);

    Serial.println("Network Credentials Saved using Preferences");

    preferences.end();
  */

}

void codeForTask1( void * parameter ) {
  for (;;) {

    scanBLE();

    if (BLEinterval > 0) {
      delay(BLEinterval);
    }
  }
}

void codeForTask2( void * parameter ) {
  for (;;) {
    postBLE();
  }
}

void codeForTask3( void * parameter ) {
  for (;;) {
    scanNetworks();

    if (WIFIinterval > 0) {
      delay(WIFIinterval);
    }
  }
}

void codeForTask4( void * parameter ) {
  for (;;) {
    postWIFI();
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

  //After connecting to a network it's information is printed
  connectToNetwork();

  //Get the Information from the server
  getInfo();

  // In the datagram to be sent it is required to identify which network the device is connected to and it's ssid
  dataGramaWIFI.replace("eduroam", (WiFi.SSID()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGramaWIFI.replace("00:11:22:33:44:55", (WiFi.macAddress()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's ssid
  dataGramaBLE.replace("eduroam", (WiFi.SSID()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGramaBLE.replace("00:11:22:33:44:99", (WiFi.macAddress()));

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

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
  xTaskCreatePinnedToCore(    codeForTask1,    "scanBLE",    4000,    NULL,    2,    &Task1,    0);
  xTaskCreatePinnedToCore(    codeForTask2,    "postBLE",    4000,    NULL,    4,    &Task2,    0);
  xTaskCreatePinnedToCore(    codeForTask3,    "scanNetworks",    4000,    NULL,    2,    &Task3,    1);
  xTaskCreatePinnedToCore(    codeForTask4,    "postWIFI",    4000,    NULL,   4,    &Task4,    1);
}

//The task which runs setup() and loop() is created on core 1 with priority 1.
void loop() {

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

  //vTaskDelete(NULL);
}
// to delete that task and free its resources because it isn't planed to use it at all.