QueueHandle_t queue;

String dataGrama = "scanData= { \"tagName\": \"tagPedro\", \"tagBSSID\": \"00:11:22:33:44:55\", \"tagNetwork\": \"eduroam\", \"dataType\": \"Wi-Fi\", \"scanMode\": \"auto\", \"WiFiData\":";


void setup() {
 
  Serial.begin(115200);
 
  queue = xQueueCreate( 2, sizeof(dataGrama) );
 
  if(queue == NULL){
    Serial.println("Error creating the queue");
  }
 
}
 
void loop() {
 
  if(queue == NULL)return;
 
  for(int i = 0; i<2; i++){
    xQueueSend(queue, &dataGrama, portMAX_DELAY);
  }
 
  int element;
 
  for(int i = 0; i<2; i++){
    xQueueReceive(queue, &dataGrama, portMAX_DELAY);
    Serial.print("|");
    Serial.print(dataGrama);
  }
 
  Serial.println();
  delay(1000);
}
