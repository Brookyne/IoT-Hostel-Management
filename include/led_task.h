#ifndef LED_TASK_H
#define LED_TASK_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// LED strip configuration
#define LED_PIN 2  // Changed to match main configuration
#define NUM_LEDS 160
#define BRIGHTNESS 255

// Function declarations
void setupLEDStrip();
void updateLEDStrip(bool state);
void ledTask(void *parameter);

// External variables
extern Adafruit_NeoPixel strip;
extern bool ledState;

#endif /* LED_TASK_H */ 