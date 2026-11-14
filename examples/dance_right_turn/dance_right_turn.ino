#include <OttoI2R.h>

// 보정값은 웹의 [로봇 맞추기]에서 맞춰 보드에 저장한 값을 쓴다.
// 아직 저장하지 않은 보드는 기본값으로 움직인다. 로봇에 따라 안 맞을 수 있으니 먼저 [로봇 맞추기]를 하세요.

void setup() {
  ottoInit();
}

void loop() {
  moveHip(90, 90);
  delay(2000);
  moveHip(55, 55);
  delay(2000);

  rollRightFoot(150);     // 땅에 디딘 오른발을 굴려 오른쪽으로 돈다
  delay(2000);
  rollRightFoot(0);      // 오른발 멈춤
  delay(2000);
  rollRightFoot(-150);     // 땅에 디딘 오른발을 굴려 오른쪽으로 돈다
  delay(2000);
  rollRightFoot(0);      // 오른발 멈춤

}
