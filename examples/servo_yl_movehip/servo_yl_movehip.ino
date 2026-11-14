#include <OttoI2R.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("YL 90도");
  moveHip(90, 90);
  delay(2000);

  Serial.println("YL 135도");
  moveHip(90, 135);
  delay(2000);

  Serial.println("YL 180도");
  moveHip(90, 180);
  delay(2000);
}
