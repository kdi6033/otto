const int PIN_YL = 4;    // YL 서보 신호선 (GPIO 4)

// 펄스폭(마이크로초, us). 1ms = 1000us
const int T_90  = 1500;  // 1.5ms -> 90도
const int T_135 = 2000;  // 2.0ms -> 135도
const int T_180 = 2500;  // 2.5ms -> 180도

// 펄스폭(us)을 서보에 내보낸다.
// 주기 20ms(=20000us)를 16384칸(14비트)으로 나누므로, 칸 수 = 펄스폭 x 16384 / 20000
void servoWriteUs(int pin, int us) {
  ledcWrite(pin, (uint32_t)us * 16384 / 20000);
}

void setup() {
  Serial.begin(115200);
  ledcAttach(PIN_YL, 50, 14);  // 핀, 50Hz(20ms 주기), 14비트
}

void loop() {
  Serial.println("1.5ms -> 90도");
  servoWriteUs(PIN_YL, T_90);
  delay(2000);

  Serial.println("2.0ms -> 135도");
  servoWriteUs(PIN_YL, T_135);
  delay(2000);

  Serial.println("2.5ms -> 180도");
  servoWriteUs(PIN_YL, T_180);
  delay(2000);
}
