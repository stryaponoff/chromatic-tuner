#include "Arduino.h"

#define INTERRUPT_FREQUENCY 38462

byte newData = 0;
byte prevData = 0;

// Storages for events timing and slopes
int timer[10];
int slope[10];

unsigned int time = 0; // keeps time and sends vales to store in timer[] occasionally
unsigned int totalTimer; // used to calculate period
unsigned int period; // storage for period of wave

byte index = 0; // current storage index
int maxSlope = 0; // used to calculate max slope as trigger point
int newSlope; // storage for incoming slope data
float frequency; // storage for frequency calculations

// variables for deciding whether you have a match
byte noMatch = 0; // counts how many non-matches you've received to reset variables if it's been too long
byte slopeTolerance = 3; // slope tolerance- adjust this if you need
int timerTol = 10; // timer tolerance- adjust this if you need

// variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30; // raise if you have a very noisy signal

const float frequencyChart[] = {
    16.35, // C0
    17.32, // C#0
    18.35, // D0
    19.45, // D#0
    20.60, // E0
    21.83, // F0
    23.12, // F#0
    24.50, // G0
    25.96, // G#0
    27.50, // A0
    29.14, // A#0
    30.87, // B0
};

const String noteNames[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// Cleaning out calculation buffer. Called if there are no matches for so long
void reset() {
  index = 0;
  noMatch = 0;
  maxSlope = 0;
}

String getNote(float frequency) {
    float remainder;
    float remainderMin;
    unsigned int noteIndex;
    unsigned int octaveIndex;

    for (int j = 0; j < 10; j++) { // chromatic sequence
        for (int i = 0; i < 12; i++) { // notes in octave
            remainder = fmod(frequency, frequencyChart[i] * pow(2, j));

            if (i == 0 && j == 0 && noteIndex > 0) {
                remainderMin = remainder;
                noteIndex = i;
            } else {
                if (remainderMin < remainder) {
                    remainderMin = remainder;
                    noteIndex = i;
                    octaveIndex = j;
                }
            }
        }
    }

    return String(noteNames[noteIndex] + String(octaveIndex));
}

void setup() {
  Serial.begin(9600);

  cli(); // disable interrupts
  ADCSRA = 0;
  ADCSRB = 0;

  ADMUX |= (1 << REFS0); // set reference voltage
  ADMUX |= (1 << ADLAR); // left align the ADC value- so we can read highest 8 bits from ADCH register only
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); // set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); // enabble auto trigger
  ADCSRA |= (1 << ADIE); // enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); // enable ADC
  ADCSRA |= (1 << ADSC); // start ADC measurements
  sei(); // enable interrupts
}

ISR(ADC_vect) { // when new ADC value ready
  PORTB &= B11101111; // set pin 12 low
  prevData = newData; // store previous value
  newData = ADCH; // get value from A0

  if (prevData < 127 && newData >= 127) { // if increasing and crossing midpoint
    newSlope = newData - prevData; // calculate slope
    if (abs(newSlope - maxSlope) < slopeTolerance) { // if slopes are equal
      // record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;

      if (index == 0) { // new max slope just reset
        PORTB |= B00010000; // set pin 12 high
        noMatch = 0;
        index++; // increment index
      } else if (
        abs(timer[0] - timer[index]) < timerTol &&
        abs(slope[0] - newSlope) < slopeTolerance
      ) { // if timer duration and slopes match
        // sum timer values
        totalTimer = 0;
        for (byte i = 0; i < index; i++){
          totalTimer += timer[i];
        }
        period = totalTimer; // set period
        // reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;
        PORTB |= B00010000; // set pin 12 high
        noMatch = 0;
      } else { // crossing midpoint but not match
        index++;
        if (index > 9){
          reset();
        }
      }
    } else if (newSlope > maxSlope){ // if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0; // reset clock
      noMatch = 0;
      index = 0; // reset index
    } else { // slope not steep enough
      noMatch++; // increment no match counter
      if (noMatch > 9) {
        reset();
      }
    }
  }

  time++; // increment timer at rate of 38.5kHz

  ampTimer++; // increment amplitude timer
  if (abs(127 - ADCH) > maxAmp){
    maxAmp = abs(127 - ADCH);
  }
  if (ampTimer == 1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }

}

void loop() {
  if (checkMaxAmp > ampThreshold) {
    frequency = INTERRUPT_FREQUENCY / float(period); // calculate frequency timer rate/period
  }

  if (period > 0 && frequency > 0) {
    Serial.println(frequency);
    Serial.println(getNote(frequency));
  }
  delay(100);
}
