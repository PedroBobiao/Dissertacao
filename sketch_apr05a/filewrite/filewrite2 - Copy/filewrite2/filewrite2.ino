#include "SPIFFS.h"

void setup() {

   Serial.begin(115200);

   if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
   }

    

    //---------- Read file
    File fileToRead = SPIFFS.open("/test.txt");

    if(!fileToRead){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("File Content:");

    while(fileToRead.available()){

        Serial.write(fileToRead.read());
    }

    fileToRead.close();
}

void loop() {}
