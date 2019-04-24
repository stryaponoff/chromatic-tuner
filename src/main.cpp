#include "main.h"

const int ocrSampleRate =  F_CPU / (1 * SAMPLE_RATE) - 1;
volatile int currentData = 0;
volatile int samples[SAMPLE_COUNT];
volatile int count = 0;
volatile bool readyFlag = false;

ISR(TIMER1_COMPA_vect){
    cli();
    samples[count] = currentData;
    if(count >= SAMPLE_COUNT) {
        readyFlag = true;
        count = 0;
    }
    else {
        count++;
    }
    sei();
}

ISR(ADC_vect) {
    cli();
    currentData = ADCH;
    ADCSRA |= (1 << ADSC); //start ADC measurements
    sei();
}

void setup()
{
    Serial.begin(9600);
    Serial.println("Ready");

    cli(); 
    timer_setup();
    adc_setup();
    sei();
}

void loop()
{
    if(readyFlag) {
        cli();
        readyFlag = false;
        sei();
    }
}

void timer_setup() {
    TCCR1A = 0; 
    TCCR1B = 0; 
    TCNT1  = 0; 
    OCR1A = ocrSampleRate;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
}

void adc_setup() {
    ADCSRA = 0;
    ADCSRB = 0;
    ADMUX |= (1 << REFS0); 
    ADMUX |= (1 << ADLAR);
    ADCSRA |= (1 << ADPS2) | (1 << ADPS0); 
    ADCSRA |= (1 << ADATE); 
    ADCSRA |= (1 << ADIE);
    ADCSRA |= (1 << ADEN);
    ADCSRA |= (1 << ADSC);
}
