#include "WiFi.h"
#include <HTTPClient.h>

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

String dataGrama = "scanData= { \"tagName\": \"tagPedro\", \"tagBSSID\": \"00:11:22:33:44:55\", \"tagNetwork\": \"eduroam\", \"dataType\": \"Wi-Fi\", \"scanMode\": \"auto\", \"WiFiData\":";

void scanNetworks() {
  int linhaUm = 1;
  int numberOfNetworks = WiFi.scanNetworks();

  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);
  dataGrama += "[";

  for (int i = 0; i < numberOfNetworks; i++) {
   if (linhaUm == 0) { dataGrama +=", "; } else linhaUm = 0;
    Serial.print(i + 1);
    Serial.print(") ");

    dataGrama += "{\"bssid\": \"bssid(i)\",      \"rssi\": \"rssi(i)\",      \"ssid\": \"ssid(i)\",      \"encript\": \"encript(i)\"}";

    Serial.print("MAC address: ");
    Serial.println(WiFi.BSSIDstr(i));
    dataGrama.replace("bssid(i)", (WiFi.BSSIDstr(i)));

    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));
    String rssiStr = String (WiFi.RSSI(i));
    dataGrama.replace("rssi(i)", rssiStr);

    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));
    dataGrama.replace("ssid(i)", (WiFi.SSID(i)));


    Serial.print("Encryption type: ");
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    Serial.println(encryptionTypeDescription);
    dataGrama.replace("encript(i)", encryptionTypeDescription);
    Serial.println("-----------------------");

  }

  dataGrama += "]}";


}

void connectToNetwork() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }

  Serial.println("Connected to network");

}

void htmlPost() {

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

      

      // Send HTTP POST request
      int httpResponseCode = http.POST(dataGrama);
      String response = http.getString();

      Serial.print("                                                            ");
      Serial.println(dataGrama);
      Serial.print("                                                            HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.print(response);
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

void codeForTask1( void * parameter ) {
  for (;;) {
    Serial.print("Code is running on Core: "); Serial.println( xPortGetCoreID());
    scanNetworks();
  }
}

void codeForTask2( void * parameter ) {
  for (;;) {
    Serial.print("                                                            Code is running on Core: "); Serial.println( xPortGetCoreID());
    htmlPost();
  }
}

TaskHandle_t Task1, Task2;

void setup() {
  
  Serial.begin(115200);

  connectToNetwork();
  Serial.println(WiFi.localIP());
  dataGrama.replace("eduroam", (WiFi.SSID()));
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.BSSIDstr());
  dataGrama.replace("00:11:22:33:44:55", (WiFi.macAddress()));

  Serial.println("Timer set to 1 second (timerDelay variable), it will take 1 second before publishing the first reading.");

  pinMode(D9, OUTPUT);
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
  xTaskCreatePinnedToCore(    codeForTask1,    "ScanNetworks",    5000,    NULL,    2,    &Task1,    0);
  //delay(500);  // needed to start-up task1 */
  xTaskCreatePinnedToCore(    codeForTask2,    "HTMLPost",    5000,    NULL,    2,    &Task2,    1);

}

void loop() {

}
