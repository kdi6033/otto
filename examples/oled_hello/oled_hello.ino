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
