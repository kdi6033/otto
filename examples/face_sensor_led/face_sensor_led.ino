#include <FastLED.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <math.h>
#include "Adafruit_VL53L0X.h"

// ------------------- OLED ---------------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
int pupilX = 0, pupilY = 0;
int directionX = 1, directionY = 1;

// ------------------- FastLED ---------------------
#define NUM_LEDS 16
#define DATA_PIN 1   // LED 링 데이터 핀 (GP01)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
uint8_t gHue = 0;
int mode = 1;  // 1:rainbow, 2:blink, 3:flow, 4:breath

// 타이머
unsigned long previousBlinkMillis = 0;
bool blinkState = false;
unsigned long previousFlowMillis = 0;
int flowIndex = 0;
unsigned long previousFrameMillis = 0;
const unsigned long frameInterval = 50;

// ------------------- Breathing ---------------------
int breathBrightness = 0;
bool breathUp = true;
unsigned long previousBreathMillis = 0;

// ------------------- VL53L0X ---------------------
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
unsigned long previousDistanceMillis = 0;
const unsigned long distanceInterval = 500;

// ------------------- SETUP ---------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(18, 17);  // I2C 핀 설정

  // VL53L0X 초기화
  if (!lox.begin()) {
    Serial.println(F("VL53L0X 초기화 실패!"));
    while (1);
  }

  u8g2.begin();

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  Serial.println(F("시작 완료: OLED + LED + VL53L0X"));
}

// ------------------- LOOP ---------------------
void loop() {
  unsigned long currentMillis = millis();

  // 거리 측정 (0.5초 간격)
  if (currentMillis - previousDistanceMillis >= distanceInterval) {
    previousDistanceMillis = currentMillis;
    printDistance();
  }

  // 프레임 타이머
  if (currentMillis - previousFrameMillis >= frameInterval) {
    previousFrameMillis = currentMillis;

    updateEyeAnimation();

    switch (mode) {
      case 1: updateRainbowEffect(); break;
      case 2: updateBlinkEffect(); break;
      case 3: updateFlowingEffect(); break;
      case 4: updateBreathingEffect(); break;
    }
  }
}

// ------------------- VL53L0X ---------------------
void printDistance() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {
    Serial.print("거리: ");
    Serial.print(measure.RangeMilliMeter);
    Serial.println(" mm");
  } else {
    Serial.println("측정 실패");
  }
}

// ------------------- Eye ---------------------
void updateEyeAnimation() {
  int eye1X = 32, eye2X = 96, eyeY = 32, radius = 16;

  pupilX += directionX;
  pupilY += directionY;
  if (abs(pupilX) > 5) directionX = -directionX;
  if (abs(pupilY) > 3) directionY = -directionY;

  u8g2.clearBuffer();
  u8g2.drawCircle(eye1X, eyeY, radius, U8G2_DRAW_ALL);
  u8g2.drawDisc(eye1X + pupilX, eyeY + pupilY, 5, U8G2_DRAW_ALL);
  u8g2.drawCircle(eye2X, eyeY, radius, U8G2_DRAW_ALL);
  u8g2.drawDisc(eye2X + pupilX, eyeY + pupilY, 5, U8G2_DRAW_ALL);
  u8g2.sendBuffer();
}

// ------------------- FastLED Effects ---------------------
void updateRainbowEffect() {
  fill_rainbow(leds, NUM_LEDS, gHue++);
  FastLED.show();
}

void updateBlinkEffect() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousBlinkMillis >= 300) {
    previousBlinkMillis = currentMillis;
    blinkState = !blinkState;
    fill_solid(leds, NUM_LEDS, blinkState ? CRGB::White : CRGB::Black);
    FastLED.show();
  }
}

void updateFlowingEffect() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousFlowMillis >= 100) {
    previousFlowMillis = currentMillis;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[flowIndex] = CRGB::Blue;
    flowIndex = (flowIndex + 1) % NUM_LEDS;

    FastLED.show();
  }
}

void updateBreathingEffect() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousBreathMillis >= 20) {
    previousBreathMillis = currentMillis;

    if (breathUp) {
      breathBrightness += 5;
      if (breathBrightness >= 255) {
        breathBrightness = 255;
        breathUp = false;
      }
    } else {
      breathBrightness -= 5;
      if (breathBrightness <= 0) {
        breathBrightness = 0;
        breathUp = true;
      }
    }

    fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, breathBrightness));
    FastLED.show();
  }
}
