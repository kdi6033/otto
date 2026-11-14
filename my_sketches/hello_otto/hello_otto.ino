// hello_otto: 처음 올려 보는 프로그램
// 인사하듯 엉덩이를 좌우로 흔들고, 앞으로 세 걸음 걷고, 멈춘다.

#include <OttoI2R.h>

void setup() {
  ottoInit();                    // 꼭 첫 줄: 저장된 보정값 불러오기 + Serial.begin(115200)
  delay(1000);
  Serial.println("안녕하세요, Otto 입니다!");

  // 인사: 엉덩이를 좌우로 두 번 흔들기
  for (int i = 0; i < 2; i++) {
    moveHip(110, 110);
    delay(400);
    moveHip(70, 70);
    delay(400);
  }
  moveHip(90, 90);               // 똑바로 서기
  delay(500);

  // 앞으로 세 걸음
  for (int i = 0; i < 3; i++) {
    Serial.print("걸음 ");
    Serial.println(i + 1);
    walkFront(100);
  }
  Serial.println("끝!");
}

void loop() {
  // 한 번만 하고 쉰다
}
