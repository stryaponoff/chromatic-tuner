#include "Arduino.h"
#include <PCD8544.h>

#define SAMPLE_RATE 38462
#define MEASURE_COUNT 10 // count of frequency measures used to compute average freqency value
#define INPUT_THRESHOLD 5 // threshold of analog input
#define DISPLAY_DELAY 3000 // a number of milliseconds of displaying last note
#define IDLE_DELAY 1000 // delay between measures when controller in idle mode
#define WAKE_DELAY 500  // and when it hears something

byte newData = 0;
byte prevData = 0;

// Storages for events timing and slopes
int timer[10];
int slope[10];

unsigned int time = 0;   // keeps time and sends vales to store in timer[] occasionally
unsigned int totalTimer; // used to calculate period
unsigned int period;     // storage for period of wave

byte index = 0;   // current storage index
int maxSlope = 0; // used to calculate max slope as trigger point
int newSlope;     // storage for incoming slope data
float frequency;  // storage for frequency calculations

// variables for deciding whether you have a match
byte noMatch = 0;        // counts how many non-matches you've received to reset variables if it's been too long
byte slopeTolerance = 3; // slope tolerance- adjust this if you need
int timerTol = 10;       // timer tolerance- adjust this if you need

// variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30; // raise if you have a very noisy signal

// Note being played and reference frequency for it
int currentNote = 0;
int currentOctave = 0;
volatile float currentReference = 0;

const float frequencyChart[] = {
    16.3515978313, // C0
    17.3239144361, // C#0
    18.3540479948, // D0
    19.4454364826, // D#0
    20.6017223071, // E0
    21.8267644646, // F0
    23.1246514195, // F#0
    24.4997147489, // G0
    25.9565435987, // G#0
    27.5000000000, // A0
    29.1352350949, // A#0
    30.8677063285, // B0
};

const String noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// LCD
static const byte glyph[] = { B00010000, B00110100, B00110000, B00110100, B00010000 };
static PCD8544 lcd;

// Cleaning out calculation buffer. Called if there are no matches for so long
void reset() {
    index = 0;
    noMatch = 0;
    maxSlope = 0;
    currentReference = 0;
}

void getNote(float frequency) {
    float remainder = 0;
    float remainderMin;
    float freqReference = 0;
    unsigned int noteIndex = 0;
    unsigned int octaveIndex = 0;

    currentNote = 0;
    currentOctave = 0;

    for (byte j = 0; j < 10; j++) {     // chromatic sequence
        for (byte i = 0; i < 12; i++) { // notes in octave
            freqReference = frequencyChart[i] * pow(2, j);
            remainder = fmod(frequency, freqReference);

            if (i == 0 && j == 0) {
                remainderMin = remainder;
                noteIndex = i;
                octaveIndex = j;
                currentReference = freqReference;
            } else if (remainderMin < remainder) {
                remainderMin = remainder;
                noteIndex = i;
                octaveIndex = j;
                currentReference = freqReference;
            }
        }
    }
    currentNote = noteIndex; // current note
    currentOctave = octaveIndex; // current octave
}

String getNoteName(int note, int octave) {
    return String(noteNames[note] + String(octave));
}

float getFreqOffset(float measured, float reference) {
    return 1200 * log(measured / reference) / log(2);
}

void display(float freqAverage, int cents) {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print(String(getNoteName(currentNote, currentOctave)));
    lcd.setCursor(1, 1);
    lcd.print(String(cents) + " cents");

    lcd.setCursor(42 , 3);
    lcd.drawColumn(2, 32);
    lcd.setCursor(43 , 3);
    lcd.drawColumn(2, 32);

    if (cents > 43) {
        cents = 43;
    } else if (cents < -43) {
        cents = -43;
    }

    if (cents < 0) {
        for (byte i = 43 + cents; i <= 43; i++) {
            lcd.setCursor(i , 3);
            lcd.drawColumn(0, 8);
        }
    } else {
        for (byte i = 43; i <= cents + 43; i++) {
            lcd.setCursor(i , 3);
            lcd.drawColumn(0, 8);
        }
    }
    lcd.setCursor(1, 3);
    Serial.println(String("Note: " + getNoteName(currentNote, currentOctave) + ". Offset from reference is: " + cents + " cents."));
}

void setup() {
    Serial.begin(9600);

    lcd.begin(84, 48);
    lcd.setCursor(0, 0);
    lcd.print("Init");

    cli(); // disable interrupts
    ADCSRA = 0;
    ADCSRB = 0;

    ADMUX |= (1 << REFS0);                 // set reference voltage
    ADMUX |= (1 << ADLAR);                 // left align the ADC value- so we can read highest 8 bits from ADCH register only
    ADCSRA |= (1 << ADPS2) | (1 << ADPS0); // set ADC clock with 32 prescaler- 16mHz/32=500kHz
    ADCSRA |= (1 << ADATE);                // enabble auto trigger
    ADCSRA |= (1 << ADIE);                 // enable interrupts when measurement complete
    ADCSRA |= (1 << ADEN);                 // enable ADC
    ADCSRA |= (1 << ADSC);                 // start ADC measurements
    sei();                                 // enable interrupts
}

ISR(ADC_vect) { // when new ADC value ready
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
                noMatch = 0;
                index++; // increment index
            } else if (
                abs(timer[0] - timer[index]) < timerTol &&
                abs(slope[0] - newSlope) < slopeTolerance)
            { // if timer duration and slopes match
                // sum timer values
                totalTimer = 0;
                for (byte i = 0; i < index; i++)
                {
                    totalTimer += timer[i];
                }
                period = totalTimer; // set period
                // reset new zero index values to compare with
                timer[0] = timer[index];
                slope[0] = slope[index];
                index = 1;
                noMatch = 0;
            } else { // crossing midpoint but not match
                index++;
                if (index > 9) {
                    reset();
                }
            }
        } else if (newSlope > maxSlope) { // if new slope is much larger than max slope
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
    if (abs(127 - ADCH) > maxAmp) {
        maxAmp = abs(127 - ADCH);
    }
    if (ampTimer == 1000) {
        ampTimer = 0;
        checkMaxAmp = maxAmp;
        maxAmp = 0;
    }
}

void loop() {
    float freqAverage = 0;
    float frequencySum = 0;
    int cents;

    if (period > 0) {
        for (byte i = 0; i < MEASURE_COUNT; i++) {
            if (checkMaxAmp > ampThreshold) {
                frequencySum += SAMPLE_RATE / float(period); // calculate frequency timer rate/period
            }
            delay(WAKE_DELAY / MEASURE_COUNT);
        }
    }
    freqAverage = frequencySum / MEASURE_COUNT;
    cents = round(getFreqOffset(freqAverage, currentReference));
    Serial.println(String(freqAverage));

    // There's a signal on input and frequency is above 10 Hz
    if (freqAverage != INFINITY && freqAverage > 10) {
        getNote(freqAverage);
        display(freqAverage, cents);
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No sound");
    }
}
