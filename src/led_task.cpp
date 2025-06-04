#include "led_task.h"
#include "mqtt_task.h"
#include "wifi_task.h"
#include "ap_mode_task.h"
#include "sinric_task.h"

// LED strip instance
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Rainbow effect variables
uint16_t rainbowHue = 0;

void setupLEDStrip() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
}

// Function to generate rainbow colors
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbowEffect() {
  for(uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((i + rainbowHue) & 255));
  }
  strip.show();
  rainbowHue++;
  if(rainbowHue >= 256) rainbowHue = 0;
}

void updateLEDStrip(bool state) {
  if (state) {
    // Create rainbow effect when LED is on
    rainbowEffect();
  } else {
    // Turn off all LEDs
    strip.clear();
    strip.show();
  }
}

void ledTask(void *parameter) {
  while (1) {
    // Update LED strip based on ledState
    updateLEDStrip(ledState);
    vTaskDelay(50 / portTICK_PERIOD_MS); // Reduced delay for smoother rainbow effect
  }
} 