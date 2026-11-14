#include <OttoI2R.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("앞으로 최고 속도 (400)");
  rollRightFoot(400);
  delay(2000);

  Serial.println("앞으로 느리게 (200)");
  rollRightFoot(200);
  delay(2000);

  Serial.println("정지 (0)");
  rollRightFoot(0);
  delay(2000);

  Serial.println("뒤로 느리게 (-200)");
  rollRightFoot(-200);
  delay(2000);

  Serial.println("뒤로 최고 속도 (-400)");
  rollRightFoot(-400);
  delay(2000);

  Serial.println("정지 (0)");
  rollRightFoot(0);
  delay(2000);
}
