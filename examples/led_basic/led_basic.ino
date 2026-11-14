#include <FastLED.h>

#define NUM_LEDS 16
#define DATA_PIN 1   // LED 링 데이터 핀 (GP01)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(1000);

  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(1000);

  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(1000);
}
