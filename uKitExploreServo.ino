#include <uKitExplore2.h>

int comdata = 0;

void setup() {
//  Initialization();
  Serial.begin(9600);
}

void loop() {
    if (Serial.available() > 0)
    {
      comdata = Serial.read();
      Serial.println(comdata);
      if (comdata == 89) {
        Serial.println("webcam sweeping started");
        while (comdata != 78)
        {
        setServoAngle(1,45,5000);    
        delay(1800);    
        setServoAngle(1,-45,5000);    
        delay(1800);
        comdata = Serial.read();
           if (comdata == 78) {
              Serial.println("webcam sweeping stopped");
              setServoStop(1);      
            }
       }
    }
  }
}
