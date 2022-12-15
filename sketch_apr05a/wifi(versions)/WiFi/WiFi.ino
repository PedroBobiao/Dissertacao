#include "WiFi.h"
#include <HTTPClient.h>

const char* ssid = "ORIENTATE";
const char* password =  "Orientate2021";

// Domain Name with full URL Path for HTTP POST Request
const char* serverName = "http://ils.dsi.uminho.pt/ar-ware/S02/i2a/i2aSamples.php";


// THE DEFAULT TIMER IS SET TO 1 SECOND FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Set timer to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Timer set to 10 seconds (10000)
unsigned long timerDelay = 1000;



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

void scanNetworks() {

  int numberOfNetworks = WiFi.scanNetworks();

  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);

  for (int i = 0; i < numberOfNetworks; i++) {

    Serial.print(i + 1);
    Serial.print(") ");

    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));

    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));

    Serial.print("MAC address: ");
    Serial.println(WiFi.BSSIDstr(i));

    Serial.print("Encryption type: ");
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    Serial.println(encryptionTypeDescription);
    Serial.println("-----------------------");

  }
}

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
  Serial.println("Timer set to 1 second (timerDelay variable), it will take 1 second before publishing the first reading.");
}

int countLoop = 0;

void loop() {

  //scanNetworks();
  //Serial.println("#####################################################");
  //delay(3000);

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
      //httpRequestData = "scanData= { \"tagName\": \"tagPedro\", \"tagBSSID\": \"00:11:22:33:44:55\", \"tagNetwork\": \"eduroam\", \"dataType\": \"Wi-Fi\", \"scanMode\": \"auto\", \"WiFiData\": [ {      \"bssid\": \"88:9E:33:30:05:65\",      \"rssi\": \"-49\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"92:2A:A8:C4:B4:87\",      \"rssi\": \"-70\",      \"ssid\": \"Impressoras\",      \"encript\": \"WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}]}";
      httpRequestData = "scanData= { \"tagName\": \"tagPedro\", \"tagBSSID\": \"00:11:22:33:44:55\", \"tagNetwork\": \"eduroam\", \"dataType\": \"Wi-Fi\", \"scanMode\": \"auto\", \"WiFiData\": [ {      \"bssid\": \"88:9E:33:30:05:65\",      \"rssi\": \"-49\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"92:2A:A8:C4:B4:87\",      \"rssi\": \"-70\",      \"ssid\": \"Impressoras\",      \"encript\": \"WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"}, {      \"bssid\": \"3C:51:0E:2E:C4:21\",      \"rssi\": \"-91\",      \"ssid\": \"\",      \"encript\": \"WPA_WPA2_PSK\"} ]}";
      if (countLoop == 0)
        Serial.println(httpRequestData);
      countLoop++;

      // Send HTTP POST request
      Serial.println("Post Started");
      int time1 = millis();
      int httpResponseCode = http.POST(httpRequestData);
      String response = http.getString();

      Serial.println(millis() - time1);

      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.print(response);

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

}
