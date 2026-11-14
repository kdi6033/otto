const int PIN_RR = 7;    // RR 서보 신호선 (GPIO 7)

// 펄스폭(마이크로초, us). 1ms = 1000us
const int T_FAST_A = 500;   // 0.5ms -> 한쪽 방향 최고 속도
const int T_SLOW_A = 1000;  // 1.0ms -> 한쪽 방향 느리게
const int T_STOP = 1500;    // 1.5ms -> 정지
const int T_SLOW_B = 2000;  // 2.0ms -> 반대 방향 느리게
const int T_FAST_B = 2500;  // 2.5ms -> 반대 방향 최고 속도

// 펄스폭(us)을 서보에 내보낸다. (1.3.4의 servoWriteUs 와 같다)
void servoWriteUs(int pin, int us) {
  ledcWrite(pin, (uint32_t)us * 16384 / 20000);
}

void setup() {
  Serial.begin(115200);
  ledcAttach(PIN_RR, 50, 14);   // 핀, 50Hz(20ms 주기), 14비트
  servoWriteUs(PIN_RR, T_STOP); // 시작할 때는 정지
}

void loop() {
  Serial.println("0.5ms -> 한쪽 방향 최고 속도");
  servoWriteUs(PIN_RR, T_FAST_A);
  delay(2000);

  Serial.println("1.0ms -> 한쪽 방향 느리게");
  servoWriteUs(PIN_RR, T_SLOW_A);
  delay(2000);

  Serial.println("1.5ms -> 정지");
  servoWriteUs(PIN_RR, T_STOP);
  delay(2000);

  Serial.println("2.0ms -> 반대 방향 느리게");
  servoWriteUs(PIN_RR, T_SLOW_B);
  delay(2000);

  Serial.println("2.5ms -> 반대 방향 최고 속도");
  servoWriteUs(PIN_RR, T_FAST_B);
  delay(2000);

  Serial.println("1.5ms -> 정지");
  servoWriteUs(PIN_RR, T_STOP);
  delay(2000);
}
