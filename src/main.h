#include <Arduino.h>
#include "arduinoFFT.h"

#define F_CPU 16000000L
#define SAMPLE_RATE 5000
#define SAMPLE_COUNT 128

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

void setup();
void loop();
void timer_setup();
void adc_setup();
void print_vector(double *vData, uint16_t bufferSize, uint8_t scaleType);
