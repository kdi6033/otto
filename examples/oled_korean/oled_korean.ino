#include <Wire.h>
#include <U8g2lib.h>

// SH1106 I2C 디스플레이 설정 (SDA=18, SCL=17)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void setup() {
  Wire.begin(18, 17);  // 사용자 설정 SDA, SCL
  u8g2.begin();
}

void loop() {
  u8g2.clearBuffer();

  // 유니폰트 한글 폰트 설정
  u8g2.setFont(u8g2_font_unifont_t_korean1);

  // 한글 유니코드로 출력
  u8g2.drawUTF8(0, 30, "안녕하세요!");

  u8g2.sendBuffer();
  delay(2000);
}
