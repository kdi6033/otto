#include <Wire.h>
#include "Adafruit_VL53L0X.h"

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ESP32용 I2C 핀 설정 (SDA = 18, SCL = 17)
  Wire.begin(18, 17);

  // VL53L0X 초기화
  if (!lox.begin()) {
    Serial.println(F("VL53L0X 초기화 실패! 연결을 확인하세요."));
    while (1);
  }

  Serial.println(F("VL53L0X 거리 측정 시작"));
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;

  // 거리 측정 실행
  lox.rangingTest(&measure, false); // false: 디버그 출력 없음

  if (measure.RangeStatus != 4) {  // 4는 측정 실패
    Serial.print("거리: ");
    Serial.print(measure.RangeMilliMeter);
    Serial.println(" mm");
  } else {
    Serial.println("측정 실패");
  }

  delay(500);  // 0.5초 간격
}
