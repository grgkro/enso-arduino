#include "servo_functions.h"
#include <Servo.h>

static const int servoPin = 4;

Servo servo1;

void openDoorWithServo() {
  servo1.attach(servoPin);
  
    for(int posDegrees = 0; posDegrees <= 180; posDegrees++) {
        servo1.write(posDegrees);
        Serial.println(posDegrees);
        delay(20);
    }

    for(int posDegrees = 180; posDegrees >= 0; posDegrees--) {
        servo1.write(posDegrees);
        Serial.println(posDegrees);
        delay(20);
    }
}
