#include "main.h"

arduinoFFT FFT = arduinoFFT();

volatile const int ocrSampleRate =  F_CPU/SAMPLE_RATE - 1;

double vReal[SAMPLE_COUNT];
double vImag[SAMPLE_COUNT];

volatile int count = 0;
volatile bool readyFlag = false;
volatile int currentData = 0;
volatile const int magicNumber = 4964;

ISR(TIMER1_COMPA_vect){
    cli();
    //PORTB ^= (1<<PB0);
    vReal[count] = currentData;
    vImag[count] = 0.0;
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
    ADCSRA |= (1 << ADSC);
    sei();
}

void setup()
{
    Serial.begin(9600);
    cli(); 
    //DDRB |= (1<<PB0);
    timer_setup();
    adc_setup();
    sei();
}

void loop()
{
    if(readyFlag) {
        cli();
        FFT.Windowing(vReal, SAMPLE_COUNT, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.Compute(vReal, vImag, SAMPLE_COUNT, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, SAMPLE_COUNT);
        double x = FFT.MajorPeak(vReal, SAMPLE_COUNT, magicNumber);
        Serial.println(x, 6);
        delay(500);
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

void print_vector(double *vData, uint16_t bufferSize, uint8_t scaleType)
{
    for (uint16_t i = 0; i < bufferSize; i++)
    {
        double abscissa;
        switch (scaleType)
        {
            case SCL_INDEX:
                abscissa = (i * 1.0);
                break;
            case SCL_TIME:
                abscissa = ((i * 1.0) / SAMPLE_RATE);
                break;
            case SCL_FREQUENCY:
                abscissa = ((i * 1.0 * SAMPLE_RATE) / SAMPLE_COUNT);
                break;
        }
        Serial.print(abscissa, 6);
        if(scaleType==SCL_FREQUENCY)
            Serial.print("Hz");
        Serial.print(" ");
        Serial.println(vData[i], 4);
    }
    Serial.println();
}

