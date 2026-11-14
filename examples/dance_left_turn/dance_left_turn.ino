#include <OttoI2R.h>

void setup() {
  ottoInit();
}

void loop() {
  moveHip(90, 90);
  delay(2000);
  moveHip(125, 125);
  delay(2000);

  rollLeftFoot(150);     // 땅에 디딘 왼발을 굴려 왼쪽으로 돈다
  delay(2000);
  rollLeftFoot(0);      // 왼발 멈춤
  delay(2000);
  rollLeftFoot(-150);     // 땅에 디딘 왼발을 굴려 왼쪽으로 돈다
  delay(2000);
  rollLeftFoot(0);      // 왼발 멈춤
}
