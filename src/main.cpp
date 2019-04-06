/**
 * Chromatic tuner
 */
#include "Arduino.h"

int inputPin = A0;
int outputPin = 13;

int signalValue = 0;

void setup()
{
    pinMode(outputPin, OUTPUT);
    Serial.begin(9600);
}

void loop()
{
    signalValue = analogRead(inputPin);

    if (signalValue >= 5) {
        digitalWrite(outputPin, HIGH);
        delay(1000);
    } else {
        digitalWrite(outputPin, LOW);
        delay(1000);
    }

    Serial.print(signalValue);
    Serial.print(' ');
}
