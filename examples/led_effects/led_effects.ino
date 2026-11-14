#include <FastLED.h>

#define NUM_LEDS 16       // LED 개수
#define DATA_PIN 1        // 데이터 핀 (GP01)
#define LED_TYPE WS2812B  // LED 종류
#define COLOR_ORDER GRB   // 색 순서

CRGB leds[NUM_LEDS];
uint8_t gHue = 0;         // 무지개 색상 변화용

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  rainbowEffect();      // 무지개 효과
  blinkEffect();        // 점멸 효과
  flowingEffect();      // 흐름 효과
}

// 🌈 무지개 효과
void rainbowEffect() {
  for (int i = 0; i < 100; i++) {
    fill_rainbow(leds, NUM_LEDS, gHue++);
    FastLED.show();
    delay(50);
  }
}

// ✨ 점멸 효과
void blinkEffect() {
  for (int i = 0; i < 5; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::White); // 켜짐
    FastLED.show();
    delay(300);

    fill_solid(leds, NUM_LEDS, CRGB::Black); // 꺼짐
    FastLED.show();
    delay(300);
  }
}

// 💨 흐름 효과 (앞에서 뒤로 번지는 느낌)
void flowingEffect() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
    if (i > 0) leds[i - 1] = CRGB::Black; // 이전 LED 끄기
    FastLED.show();
    delay(100);
  }
  leds[NUM_LEDS - 1] = CRGB::Black; // 마지막도 끄기
  FastLED.show();
}
