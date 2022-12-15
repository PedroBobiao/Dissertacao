/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include "WiFi.h"
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

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


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    void onResult(BLEAdvertisedDevice advertisedDevice) {

      Serial.print(number);
      Serial.print(") ");
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      Serial.print(" BSSID: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());
      Serial.print(" RSSI: ");
      Serial.println(advertisedDevice.getRSSI());
      String nome = advertisedDevice.getName().c_str();
      Serial.print(" Name: ");
      Serial.println(nome);
      number ++;
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

  connectToNetwork();
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

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
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
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

      // Data to send with HTTP POST
      String httpRequestData;
      httpRequestData = "scanData= {	\"tagName\": \"tagPedro\",	\"tagBSSID\": \"00:11:22:33:44:99\",	\"tagNetwork\": \"eduroam\",	\"dataType\": \"BLE\",	\"scanMode\": \"auto\",	\"BLEData\": [{		\"bssid\": \"03:6a:01:53:f1:1d\",		\"rssi\": -52,		\"name\": \"\"	}, {		\"bssid\": \"11:b8:98:54:3e:a5\",		\"rssi\": -87,		\"name\": \"\"	}, {		\"bssid\": \"76:56:ab:b6:09:f9\",		\"rssi\": -78,		\"name\": \"\"	}, {		\"bssid\": \"77:68:86:10:c1:f6\",		\"rssi\": -61,		\"name\": \"\"	}, {		\"bssid\": \"f5:77:cb:13:6f:7e\",		\"rssi\": -58,		\"name\": \"\"	}]}";

      Serial.println(httpRequestData);

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      String response = http.getString();



      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println(response);
      Serial.println("################################################################################");

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

}
