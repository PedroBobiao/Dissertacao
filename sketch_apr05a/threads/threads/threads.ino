#include "esp_task_wdt.h"

int count = 0;
portMUX_TYPE mmux = portMUX_INITIALIZER_UNLOCKED;

void setup() {
  Serial.begin(112500);
  /* create Mutex */
  xTaskCreate(
      lowPriorityTask,           /* Task function. */
      "lowPriorityTask",        /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      1,                        /* priority of the task */
      NULL);                    /* Task handle to keep track of created task */
  /* let lowPriorityTask run first */ 
  delay(500);
  /* let lowPriorityTask run first then create highPriorityTask */
  xTaskCreate(
      highPriorityTask,           /* Task function. */
      "highPriorityTask",        /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      4,                        /* priority of the task */
      NULL);                    /* Task handle to keep track of created task */
}

void loop() {

}
void lowPriorityTask( void * parameter )
{
  for(;;){

    Serial.println("lowPriorityTask lock section");
    /* if using shceduler stopping way then 
    un-comment/comments lines below accordingly */
    //vTaskSuspendAll();
    taskENTER_CRITICAL(&mmux);
    /* stop scheduler until this loop is finished */
    for(count=0; count<1000; count++){
      /* in general the watch dog timer willl be feed by RTOS 
      but we disable scheduler so we have to feed it */
      esp_task_wdt_feed();
    }
    /* we resume the scheduler so highPriorityTask will run again */
   // xTaskResumeAll();
   taskEXIT_CRITICAL(&mmux);
    Serial.println("lowPriorityTask leave section");

  }
  vTaskDelete( NULL );
}

void highPriorityTask( void * parameter )
{
  for(;;){
    /* highPriorityTask is resumed, we will check the counter should be 1000 */
    Serial.print("highPriorityTask is running and count is ");
    Serial.println(count);
    /* delay so that lowPriorityTask has chance to run */
    delay(50);
  }
  vTaskDelete( NULL );
}
