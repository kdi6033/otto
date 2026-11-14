#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);

  // I2C 핀 설정 (SDA=18, SCL=17) — i2r-05 보드 전용
  Wire.begin(18, 17);

  // BH1750 초기화
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("✅ BH1750 시작 성공!");
  } else {
    Serial.println("❌ BH1750 초기화 실패. 연결 확인!");
    while (1);
  }
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("💡 조도: ");
  Serial.print(lux);
  Serial.println(" lux");

  delay(1000);  // 1초 간격
}
