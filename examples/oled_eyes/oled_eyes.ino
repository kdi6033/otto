#include <Wire.h>
#include <U8g2lib.h>

// SH1106 또는 SSD1306 OLED, I2C 연결
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int pupilX = 0;
int pupilY = 0;
int directionX = 1;
int directionY = 1;

void setup() {
  Wire.begin(18, 17);   // 사용자 SDA, SCL 설정
  u8g2.begin();
}

void loop() {
  // 눈 위치
  int eye1X = 32;
  int eye2X = 96;
  int eyeY = 32;
  int radius = 16;

  // 눈동자 위치 이동
  pupilX += directionX;
  pupilY += directionY;

  if (abs(pupilX) > 5) directionX = -directionX;
  if (abs(pupilY) > 3) directionY = -directionY;

  u8g2.clearBuffer();

  // 왼쪽 눈
  u8g2.drawCircle(eye1X, eyeY, radius, U8G2_DRAW_ALL);
  u8g2.drawDisc(eye1X + pupilX, eyeY + pupilY, 5, U8G2_DRAW_ALL);

  // 오른쪽 눈
  u8g2.drawCircle(eye2X, eyeY, radius, U8G2_DRAW_ALL);
  u8g2.drawDisc(eye2X + pupilX, eyeY + pupilY, 5, U8G2_DRAW_ALL);

  u8g2.sendBuffer();
  delay(100);
}
