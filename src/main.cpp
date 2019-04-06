/**
 * Chromatic tuner
 */
#include "Arduino.h"

int inputPin = A0;
int outputPin = 13;

int signalValue = 0;

void setupADC() {
  ADCSRA = 0;
  ADCSRB = 0;
  ADMUX |= (1 << REFS0) | (1 << ADLAR); 
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0) | (1 << ADATE); 
  ADCSRA |= (1 << ADIE) | (1 << ADEN) | (1 << ADSC); 
}

ISR(ADC_vect) {
    signalValue = ADCH;
}

void setup(){
    pinMode(outputPin, OUTPUT);
    Serial.begin(9600);
}

void loop(){
    Serial.println(signalValue);
}
