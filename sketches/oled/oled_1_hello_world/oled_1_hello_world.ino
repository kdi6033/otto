// "hello world" 출력 프로그램
// 1.3" OLED(SH1106, 128x64, I2C) 예제. U8g2 라이브러리가 필요합니다. (Arduino IDE 라이브러리 관리자에서 "U8g2" 설치)
// 보드: i2r-05 (SDA=18, SCL=17)

#include <Wire.h>
#include <U8g2lib.h>

// SH1106용 I2C 설정, FULL 버퍼
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Wire.begin(18, 17);  // SDA=18, SCL=17
  u8g2.begin();
}

void loop() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr); // 폰트 설정
  u8g2.drawStr(0, 32, "Hello World");
  u8g2.sendBuffer();
  delay(1000);
}
