/* structure that hold data*/
typedef struct{
  int sender;
  int counter;
}Data;

/* this variable hold queue handle */
xQueueHandle xQueue;

void setup() {

  Serial.begin(112500);
  /* create the queue which size can contains 5 elements of Data */
  xQueue = xQueueCreate(5, sizeof(Data));
  xTaskCreate(
      sendTask1,           /* Task function. */
      "sendTask1",        /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      2,                        /* priority of the task */
      NULL);                    /* Task handle to keep track of created task */
  xTaskCreate(
      sendTask2,           /* Task function. */
      "sendTask2",        /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      2,                        /* priority of the task */
      NULL);                    /* Task handle to keep track of created task */
  xTaskCreate(
      receiveTask,           /* Task function. */
      "receiveTask",        /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      1,                        /* priority of the task */
      NULL);                    /* Task handle to keep track of created task */
}

void loop() {

}

void sendTask1( void * parameter )
{
  /* keep the status of sending data */
  BaseType_t xStatus;
  /* time to block the task until the queue has free space */
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  /* create data to send */
  Data data;
  /* sender 1 has id is 1 */
  data.sender = 1;
  data.counter = 1;
  for(;;){
    Serial.println("sendTask1 is sending data");
    /* send data to front of the queue */
    xStatus = xQueueSendToFront( xQueue, &data, xTicksToWait );
    /* check whether sending is ok or not */
    if( xStatus == pdPASS ) {
      /* increase counter of sender 1 */
      data.counter = data.counter + 1;
    }
    /* we delay here so that receiveTask has chance to receive data */
    delay(1000);
  }
  vTaskDelete( NULL );
}
/* this task is similar to sendTask1 */
void sendTask2( void * parameter )
{
  BaseType_t xStatus;
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  Data data;
  data.sender = 2;
  data.counter = 1;
  for(;;){
    Serial.println("sendTask2 is sending data");
    xStatus = xQueueSendToFront( xQueue, &data, xTicksToWait );
    if( xStatus == pdPASS ) {
      data.counter = data.counter + 1;
    }
    delay(1000);
  }
  vTaskDelete( NULL );
}
void receiveTask( void * parameter )
{
  /* keep the status of receiving data */
  BaseType_t xStatus;
  /* time to block the task until data is available */
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  Data data;
  for(;;){
    /* receive data from the queue */
    xStatus = xQueueReceive( xQueue, &data, xTicksToWait );
    /* check whether receiving is ok or not */
    if(xStatus == pdPASS){
      /* print the data to terminal */
      Serial.print("receiveTask got data: ");
      Serial.print("sender = ");
      Serial.print(data.sender);
      Serial.print(" counter = ");
      Serial.println(data.counter);
    }
  }
  vTaskDelete( NULL );
}
