//This program has multicore-threading, two tasks run on different cores with different priorities

#include "WiFi.h"
#include <HTTPClient.h>

#define DEBUG 1

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

QueueHandle_t queue;

const char* ssid = "ola12345";
const char* password =  "olaola123";

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

// Here the "static" part of the json file is initialized. It has several preset values that will be replaced as we scan the wi-fi networks.
String dataGrama = "scanData= { \"tagName\": \"tagPedro\", \"tagBSSID\": \"00:11:22:33:44:55\", \"tagNetwork\": \"eduroam\", \"dataType\": \"Wi-Fi\", \"scanMode\": \"auto\", \"WiFiData\":";

// Here the "volatile" part of the json file is initialized. This part will be changing constantly
String adicionar;

String adicionarCopy;


void scanNetworks() {
  int linhaUm = 1;
  int numberOfNetworks = WiFi.scanNetworks();

  debug("Number of networks found: ");
  debugln(numberOfNetworks);
  debugln("##################################################################################################################################################################################################");

  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionar += "[";

  for (int i = 0; i < numberOfNetworks; i++) {
    if (linhaUm == 0) {
      adicionar += ", ";
    } else linhaUm = 0;

    adicionar += "{\"bssid\": \"bssid(i)\",      \"rssi\": \"rssi(i)\",      \"ssid\": \"ssid(i)\",      \"encript\": \"encript(i)\"}";


    // The preset value of bssid is replaced by the bssid of the network number i found
    adicionar.replace("bssid(i)", (WiFi.BSSIDstr(i)));


    String rssiStr = String (WiFi.RSSI(i));
    // The preset value of rssi is replaced by the rssi of the network number i found
    adicionar.replace("rssi(i)", rssiStr);


    // The preset value of ssid is replaced by the ssid of the network number i found
    adicionar.replace("ssid(i)", (WiFi.SSID(i)));



    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));

    // The preset value of encryption is replaced by the encryption of the network number i found
    adicionar.replace("encript(i)", encryptionTypeDescription);


  }

  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionar += "]}";

  adicionarCopy = adicionar;


  adicionar.remove(0);

  xQueueOverwrite(queue, &adicionarCopy);

}

void connectToNetwork() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    debugln("Establishing connection to WiFi..");
  }

  debugln("Connected to network");
  debugln("##################################################################################################################################################################################################");

}

void htmlPost() {

  unsigned long tempo;

  xQueueReceive(queue, &adicionarCopy, portMAX_DELAY);

  if ( adicionarCopy.length() > 0) {


    // Here we concatenate both parts to originate the final datagram to be sent
    String dataGramaFinal = dataGrama + adicionarCopy;
    debugln(dataGramaFinal);
    debugln("##################################################################################################################################################################################################");

    //Send an HTTP POST request every 1 second
    /* if ((millis() - lastTime) > timerDelay) { */
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;
      tempo = millis();
      Serial.print("a: ");
      Serial.println(tempo);

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");



      // Send HTTP POST request
      
      tempo = millis();
      Serial.print("b: ");
      Serial.println(tempo);
      
      int httpResponseCode = http.POST(dataGramaFinal);
      
      tempo = millis();
      Serial.print("c: ");
      Serial.println(tempo);
      
      String response = http.getString();
      
      tempo = millis();
      Serial.print("d: ");
      Serial.println(tempo);

      debug("HTTP Response code: ");
      debugln(httpResponseCode);
      tempo = millis();
      Serial.print("e: ");
      Serial.println(tempo);
      debugln(response);
      debugln("##################################################################################################################################################################################################");

      // Free resources
      http.end();
    }
    else {
      debugln("WiFi Disconnected");
    }

  }


}

void codeForTask1( void * parameter ) {
  for (;;) {
    scanNetworks();
  }

}

void codeForTask2( void * parameter ) {
  for (;;) {
    htmlPost();
  }

}

TaskHandle_t Task1, Task2;

void setup() {

  Serial.begin(115200);

  queue = xQueueCreate( 1, 20 );

  if (queue == NULL) {
    debugln("Error creating the queue");
  }

  //After connecting to a network it's information is printed
  connectToNetwork();

  // In the datagram to be sent it is required to identify which network the device is connected to and it's ssid
  dataGrama.replace("eduroam", (WiFi.SSID()));

  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGrama.replace("00:11:22:33:44:55", (WiFi.macAddress()));



  pinMode(D9, OUTPUT);

  //Both tasks run on the same core so different priorities are attributed to each
  xTaskCreatePinnedToCore(    codeForTask1,    "ScanNetworks",    5000,    NULL,    4,    &Task1,    0);

  xTaskCreatePinnedToCore(    codeForTask2,    "HTMLPost",    5000,    NULL,    2,    &Task2,    1);

}

void loop() {

}
