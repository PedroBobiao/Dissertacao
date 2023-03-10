/* In this program a json file is created with all available BLE devices as they are scanned.
  Then the json file containing all devices found is sent to the server */

#include "WiFi.h"
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

//Counter for BLE devices scanned
int number = 1;

int scanTime = 5; //In seconds
BLEScan* pBLEScan;

const char* ssid = "ola12345";
const char* password =  "olaola123";

// Domain Name with full URL Path for HTTP POST Request
const char* serverName = "http://ils.dsi.uminho.pt/ar-ware/S02/i2a/i2aSamples.php";


// THE DEFAULT TIMER IS SET TO 1 SECOND FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Set timer to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Timer set to 10 seconds (10000)
unsigned long timerDelay = 1000;

// Here the "static" part of the json file is initialized. It has several preset values that will be replaced as we scan the bluetooth networks.
String dataGrama = "scanData= {	\"tagName\": \"tagPedro\",	\"tagBSSID\": \"00:11:22:33:44:99\",	\"tagNetwork\": \"eduroam\",	\"dataType\": \"BLE\",	\"scanMode\": \"auto\",	\"BLEData\":";

// Here the "volatile" part of the json file is initialized. This part will be changing constantly
String adicionar;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    

    void onResult(BLEAdvertisedDevice advertisedDevice) {

      // The bluetooth data containing information about all the networks found has to be sent inside brackets
      adicionar += "{\"bssid\": \"bssid(i)\",      \"rssi\": \"rssi(i)\",      \"ssid\": \"ssid(i)\"}";
      Serial.print(number);
      Serial.print(") ");
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      Serial.print(" RSSI: ");
      Serial.println(advertisedDevice.getRSSI());
      // The preset value of bssid is replaced by the bssid of the device found
      adicionar.replace("bssid(i)", (advertisedDevice.getAddress().toString().c_str()));
      String rssiStr = String (advertisedDevice.getRSSI());
      // The preset value of rssi is replaced by the rssi of the device found
      adicionar.replace("rssi(i)", rssiStr);
      String nome = advertisedDevice.getName().c_str();
      // The preset value of ssid is replaced by the ssid of the device found
      adicionar.replace("ssid(i)", nome);
      number ++;
      adicionar += ", ";
      Serial.println("-----------------------");

    }



};


void connectToNetwork() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }

  Serial.println("Connected to network");

}

void setup() {
  Serial.begin(115200);

  //After connecting to a network it's information is printed
  connectToNetwork();
  Serial.println(WiFi.localIP());
  // In the datagram to be sent it is required to identify which network the device is connected to and it's ssid
  dataGrama.replace("eduroam", (WiFi.SSID()));
  Serial.println(WiFi.macAddress());
  //Serial.println(WiFi.BSSIDstr());
  // In the datagram to be sent it is required to identify which network the device is connected to and it's macAdress
  dataGrama.replace("00:11:22:33:44:99", (WiFi.macAddress()));

  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // put your main code here, to run repeatedly:
  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionar += "[";
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  // The wi-fi data containing information about all the networks found has to be sent inside brackets
  adicionar += "]}";
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  Serial.println("################################################################################");
  number = 1;
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);

  //Send an HTTP POST request every 1 second
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      //It is necessary to remove the last comma
      int lastComma = adicionar.lastIndexOf(',');
      adicionar.remove(lastComma , 1);

      // Send HTTP POST request
      String dataGramaFinal = dataGrama + adicionar;
      int httpResponseCode = http.POST(dataGramaFinal);
      String response = http.getString();


      Serial.println(dataGramaFinal);
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println(response);
      Serial.println("################################################################################");
      //We clear the content of the file so it can receive new data
      dataGramaFinal.remove(0);

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  //We clear the content of the file so it can receive new data
  adicionar.remove(0);

}
