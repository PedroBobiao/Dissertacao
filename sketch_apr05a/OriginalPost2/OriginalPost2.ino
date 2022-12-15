/*
  Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp32-http-post-ifttt-thingspeak-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "ola12345";
const char* password = "olaola123";

// Domain Name with full URL Path for HTTP POST Request
const char* serverName = "http://ils.dsi.uminho.pt/ar-ware/S02/i2a/i2aSamples.php";


// THE DEFAULT TIMER IS SET TO 1 SECOND FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Set timer to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Timer set to 10 seconds (10000)
unsigned long timerDelay = 1000;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 1 second (timerDelay variable), it will take 1 second before publishing the first reading.");

}

void loop() {
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
      httpRequestData = "scanData= { \"tagName\": \"tagPedro\", \"tagBSSID\": \"00:11:22:33:44:55\", \"tagNetwork\": \"eduroam\", \"dataType\": \"Wi-Fi\", \"scanMode\": \"auto\", \"WiFiData\": [ {      \"bssid\": \"88:9E:33:30:05:65\",      \"rssi\": \"-49\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"92:2A:A8:C4:B4:87\",      \"rssi\": \"-70\",      \"ssid\": \"Impressoras\",      \"encript\": \"WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}]}";

      Serial.print(httpRequestData);

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      String response = http.getString();



      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println(response);

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
