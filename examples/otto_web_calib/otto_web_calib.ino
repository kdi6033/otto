// Otto 웹 조종 + OLED 눈 + [로봇 맞추기]
// otto_web_eyes 에 [로봇 맞추기] 버튼과 보정 화면을 더한 프로그램이다.
//
// [로봇 맞추기] 화면 (조종 화면 아래 [로봇 맞추기] 버튼을 누르면 열린다)
//   1. 조립 자세   : 다리를 0도 / 180도로 움직인다 (이 자세로 다리를 끼우고 나사를 조인다)
//   2. 다리 수직   : 다리를 90도로 움직인 뒤 YL, YR 중심값(us)을 바꿔 다리를 수직으로 맞춘다
//   3. 굴리기 시험 : 앞으로 굴리면서 왼발/오른발 속도(us)를 맞춰 똑바로 가게 한다
//   4. 걷기 시험   : 걸으면서 발을 들 때 기울기(도)를 맞춘다
//   5. 댄스 시험   : 한 발로 설 때 엉덩이 각도를 맞춘다 (왼발 댄스 / 오른발 댄스)
//   [저장 (보드 NVS)] 을 누르면 보드에 저장되고, 다른 스케치도 ottoInit() 으로 이 값을 쓴다.
//   [값 복사] 는 값을 글자로 복사한다. [기본값으로] 는 저장된 값을 지우고 기본값으로 되돌린다.
//
// OLED 눈 화면
//   - 처음 켜지면 접속 주소 같은 정보를 5초 동안 보여 준 뒤 눈을 그린다.
//   - Wi-Fi 연결이 안 되면 → 찡그린 눈c:\Users\kdi\Downloads\i2r-04.step
//   - 모두 연결되면       → 눈동자가 움직인다 (로봇이 움직이면 그 방향을 본다)
//   - 5초 동안 안 움직이면  → 잠자는 눈 (Z z)
//   - 스위치 SW3(GPIO 16)을 누르면 정보 화면을 다시 5초 동안 보여 준다.
//
// 사용법
//  1. 아래 WIFI_SSID, WIFI_PASS 에 공유기(ipTIME 등) 이름과 비밀번호를 넣고 업로드한다.
//  2. OLED 화면이나 시리얼 모니터(115200)에 나온 주소(예: http://192.168.0.37)를 같은 공유기에 접속한
//     휴대폰/PC 브라우저에서 연다. http://otto-xxxx.local 처럼 이름으로도 열 수 있다(안드로이드는 안 될 수 있음).
//  * 공유기 접속에 실패하면 Otto 가 직접 Wi-Fi "Otto-XXXX"(비밀번호 AP_PASS)를 만든다.
//    그 Wi-Fi 에 접속하면 화면이 자동으로 뜨고, 안 뜨면 http://192.168.4.1 을 연다.
//  3. 위의 [걷기] [굴리기] [미끄러짐] 중 하나를 고른다.
//  4. 원의 전진/후진/좌회전/우회전 구역을 누르면 정지할 때까지 계속 움직인다. 가운데 [정지]를 누르면 멈춘다.
//     걷기·굴리기의 전진/후진은 가운데에 가까울수록 느리게(40), 바깥으로 갈수록 빠르게(200).
//     누른 채로 밀면 속도가 끊김 없이 바뀐다. (걷기는 다음 걸음부터, 굴리기는 바로)
//
// 동작 함수 (OttoI2R 라이브러리)
//   걷기     : walkFront(속도)   walkBack(속도)   walkLeft()   walkRight()
//   굴리기   : rollingFront(속도) rollingBack(속도) rollingLeft() rollingRight()
//   미끄러짐 : slideFront()  slideBack()  slideLeft()  slideRight()
//
// 안전: 화면이 3초 넘게 신호를 보내지 않으면(휴대폰 화면 꺼짐, Wi-Fi 끊김) 스스로 멈춘다.
// 보정값은 웹 [로봇 맞추기]에서 보드에 저장한 값을 ottoInit() 이 읽어서 쓴다.

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <OttoI2R.h>

// 1.3" OLED (SH1106, I2C: SDA=18, SCL=17) 에 접속 주소를 보여 준다 (가이드 3.17)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// 정보 화면에 쓸 네 줄 (나중에 SW3 을 누르면 다시 보여 주려고 기억해 둔다)
String infoTitle, infoBig, info3, info4;

// 정보 화면을 그린다: 제목(작게), 주소(크게), 아래 두 줄(작게)
void drawInfo() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, infoTitle.c_str());
  u8g2.setFont(u8g2_font_7x13B_tf);   // 주소는 굵고 크게 (IP 15자까지 한 줄)
  u8g2.drawStr(0, 30, infoBig.c_str());
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 48, info3.c_str());
  u8g2.drawStr(0, 62, info4.c_str());
  u8g2.sendBuffer();
}

// 정보 네 줄을 기억하고 바로 화면에 보여 준다
void showLcd(const String &title, const String &big, const String &line3, const String &line4) {
  infoTitle = title; infoBig = big; info3 = line3; info4 = line4;
  drawInfo();
}

// ── 눈 그리기 ───────────────────────────────────────────────
const int EYE_LX = 32, EYE_RX = 96, EYE_Y = 32;  // 왼쪽/오른쪽 눈 가운데
const int EYE_R = 24;                            // 눈 크기 (화면 높이 64 안에 들어가게 24까지)
const int PUPIL_R = 15;                          // 눈동자 크기
const int PUPIL_MAX_OFFSET = EYE_R - PUPIL_R - 2;  // 눈동자가 테두리 밖으로 나가지 않게 이동 제한

// 뜬 눈: 동그라미 + 눈동자(검은 점) + 반짝이는 점
void drawOpenEye(int cx, int cy, int px, int py) {
  u8g2.drawCircle(cx, cy, EYE_R);
  u8g2.drawDisc(cx + px, cy + py, PUPIL_R);
  u8g2.setDrawColor(0);
  u8g2.drawDisc(cx + px + 4, cy + py - 4, 3);    // 반짝임
  u8g2.setDrawColor(1);
}

// 깜빡이는 순간: 가로줄
void drawBlinkEye(int cx, int cy) {
  u8g2.drawBox(cx - EYE_R, cy - 1, EYE_R * 2 + 1, 3);
}

// 잠자는 눈: 아래로 휜 눈꺼풀 + 속눈썹 (examples/oled_sleepy_eyes 와 같은 방법)
void drawSleepingEye(int cx, int cy) {
  const int radius = 16;
  for (int angle = 30; angle <= 150; angle++) {
    float rad = angle * PI / 180.0;
    u8g2.drawPixel(cx + radius * cos(rad), cy + radius * sin(rad));
  }
  for (int angle = 30; angle <= 150; angle += 15) {
    float rad = angle * PI / 180.0;
    int x1 = cx + radius * cos(rad);
    int y1 = cy + radius * sin(rad);
    u8g2.drawLine(x1, y1, x1 + 5 * cos(rad), y1 + 5 * sin(rad));
  }
}

// 찡그린 눈: 눈 위쪽을 비스듬히 가리고(눈꺼풀) 눈동자는 아래로
//   inner = 코 쪽 방향 (+1 은 오른쪽, -1 은 왼쪽). 코 쪽이 더 내려와서 찡그린 모양이 된다.
void drawFrownEye(int cx, int cy, int inner, int shake) {
  u8g2.drawCircle(cx, cy, EYE_R);
  u8g2.drawDisc(cx + shake, cy + 8, PUPIL_R - 1);

  int ox = cx - inner * (EYE_R + 2), oy = cy - EYE_R + 4;   // 바깥쪽 눈꺼풀 끝
  int ix = cx + inner * (EYE_R + 2), iy = cy - 2;           // 코 쪽 눈꺼풀 끝
  int top = cy - EYE_R - 3;
  u8g2.setDrawColor(0);                                     // 눈꺼풀 위쪽을 지운다
  u8g2.drawTriangle(ox, top, ix, top, ix, iy);
  u8g2.drawTriangle(ox, top, ix, iy, ox, oy);
  u8g2.setDrawColor(1);
  u8g2.drawLine(ox, oy, ix, iy);                            // 눈꺼풀 선 (두 줄로 굵게)
  u8g2.drawLine(ox, oy + 1, ix, iy + 1);
}

// ▼ 공유기 정보를 여기에 넣는다
const char *WIFI_SSID = "i2r";   // 공유기 이름 (SSID)
const char *WIFI_PASS = "00000000";     // 공유기 비밀번호
const unsigned long WIFI_WAIT_MS = 15000;  // 공유기 접속을 기다리는 시간

// 공유기 접속에 실패했을 때 Otto 가 만드는 Wi-Fi 의 비밀번호 (8자 이상). "" 이면 비밀번호 없음
const char *AP_PASS = "12345678";
bool apMode = false;  // true 면 Otto 가 직접 Wi-Fi 를 만든 상태

const int SPEED_MIN = 40;
const int SPEED_MAX = 200;
const unsigned long SIGNAL_TIMEOUT_MS = 3000;  // 이 시간 동안 화면 신호가 없으면 멈춤

// 동작 코드 (웹 앱의 프로토콜과 같은 이름). 순서가 곧 번호다.
//   w=걷기 r=굴리기 s=미끄러짐,  f=전진 b=후진 l=좌회전 r=우회전
const char *ACTIONS[] = {
  "wf", "wb", "wl", "wr",   // 0~3  걷기
  "rf", "rb", "rl", "rr",   // 4~7  굴리기
  "sf", "sb", "sl", "sr",   // 8~11 미끄러짐
  "cr", "cw", "dl", "dr",   // 12~15 로봇 맞추기: 굴리기 시험, 걷기 시험, 왼발 댄스, 오른발 댄스
};

// [기본값으로] 를 누르면 쓰는 값 (OttoI2R 라이브러리의 기본값과 같다)
const int DEF_YL = 1250, DEF_YR = 1450, DEF_TILT_R = 20, DEF_TILT_L = 23, DEF_RL = 450, DEF_RR = 550;
const int DANCE_SPEED = 300;  // 댄스 시험에서 발을 굴리는 속도 (라이브러리 dance() 와 같다)
const int ACTION_COUNT = sizeof(ACTIONS) / sizeof(ACTIONS[0]);

// 속도를 쓰는 동작: 걷기·굴리기의 전진/후진
bool usesSpeed(int a) {
  return a == 0 || a == 1 || a == 4 || a == 5;
}

WebServer server(80);
DNSServer dns;

volatile int curAction = -1;         // 지금 동작 번호 (-1 이면 정지)
volatile int curSpeed = 0;           // 속도를 쓰는 동작의 속도 (40~200)
volatile unsigned long lastSignal = 0;  // 마지막으로 화면 신호를 받은 시각
TaskHandle_t moveHandle = nullptr;   // 동작 작업 (정지할 때 지운다)

const int SW3_PIN = 16;                     // 스위치 SW3 (누르면 LOW)
const unsigned long INFO_SHOW_MS = 5000;    // 정보 화면을 보여 주는 시간
const unsigned long SLEEP_AFTER_MS = 5000;  // 이 시간 동안 안 움직이면 잠든다
volatile unsigned long infoUntil = 0;       // 이 시각까지 정보 화면을 보여 준다
volatile unsigned long lastActive = 0;      // 마지막으로 움직인(깨어 있던) 시각

void stopNow();

// ── 화면 (HTML) ─────────────────────────────────────────────
#include "page.h"  // 화면 (HTML) 은 page.h 에 있다

// ── 웹 요청 처리 ────────────────────────────────────────────
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

// /move?a=동작코드&v=속도   (a=stop 이면 정지)
void handleMove() {
  String a = server.arg("a");
  lastSignal = millis();

  // 로봇 맞추기: 자세 버튼 (멈춘 뒤 그 자세로 가만히 있는다)
  if (a == "p0" || a == "p90") {
    stopNow();
    if (a == "p0") {
      writeYR(0);     // 조립 자세: 오른쪽 다리 0도
      writeYL(180);   //            왼쪽 다리 180도
    } else {
      writeYR(90);    // 다리 수직 (보정된 90도)
      writeYL(90);
    }
    Serial.println(a == "p0" ? "조립 자세 0도 / 180도" : "다리 90도");
    server.send(200, "text/plain", "ok");
    return;
  }

  int idx = -1;
  for (int i = 0; i < ACTION_COUNT; i++) {
    if (a == ACTIONS[i]) { idx = i; break; }
  }

  if (idx < 0) {
    stopNow();  // stop 이나 모르는 코드는 정지
  } else {
    int v = usesSpeed(idx) ? constrain(server.arg("v").toInt(), SPEED_MIN, SPEED_MAX) : 0;
    if (idx != curAction) {
      // 다른 동작으로 바뀌면 먼저 멈추고 다리를 가운데로 둔 뒤 시작한다
      // (굴리기 자세 그대로 걷기를 시작하지 않게)
      stopNow();
      curSpeed = v;
      curAction = idx;
      Serial.printf("동작: %s  속도: %d\n", ACTIONS[idx], v);
    } else if (v != curSpeed) {
      curSpeed = v;
      Serial.printf("속도: %d\n", v);
    }
  }
  server.send(200, "text/plain", "ok");
}

// ── 로봇 맞추기 요청 ────────────────────────────────────────
// 지금 보정값을 JSON 글자로 만든다
String calJson() {
  OttoCalibration c;
  ottoGetCalibration(c);
  int ryr, ryl, lyr, lyl;
  getDanceTilts(ryr, ryl, lyr, lyl);
  char buf[220];
  snprintf(buf, sizeof(buf),
           "{\"yl\":%d,\"yr\":%d,\"tr\":%d,\"tl\":%d,\"rl\":%d,\"rr\":%d,"
           "\"ryr\":%d,\"ryl\":%d,\"lyr\":%d,\"lyl\":%d,\"saved\":%s}",
           c.ylCenter, c.yrCenter, c.tiltR, c.tiltL, c.rlSpeed, c.rrSpeed,
           ryr, ryl, lyr, lyl, ottoHasSavedCalibration() ? "true" : "false");
  return String(buf);
}

// /cal/get : 지금 값을 알려 준다
void handleCalGet() {
  server.send(200, "application/json", calJson());
}

// 주소에 그 이름의 값이 있으면 그 값, 없으면 원래 값
int argOr(const char *name, int now) {
  return server.hasArg(name) ? server.arg(name).toInt() : now;
}

// /cal/set?yl=..&yr=..&tr=..&tl=..&rl=..&rr=..&ryr=..&ryl=..&lyr=..&lyl=..
// 값을 바로 적용한다 (보드에 저장은 하지 않는다. 저장은 /cal/save)
void handleCalSet() {
  OttoCalibration c;
  ottoGetCalibration(c);
  int ryr, ryl, lyr, lyl;
  getDanceTilts(ryr, ryl, lyr, lyl);

  int yl = argOr("yl", c.ylCenter), yr = argOr("yr", c.yrCenter);
  bool hipChanged = (yl != c.ylCenter || yr != c.yrCenter);

  ottoSetHipCenter(yl, yr);                                  // 다리 중심값 (1000~1900)
  ottoSetTilt(argOr("tr", c.tiltR), argOr("tl", c.tiltL));   // 걷기 기울기 (0~45)
  ottoSetSpeed(argOr("rl", c.rlSpeed), argOr("rr", c.rrSpeed));  // 굴리기 속도 (0~600)
  applyDanceTilts(argOr("ryr", ryr), argOr("ryl", ryl), argOr("lyr", lyr), argOr("lyl", lyl));

  // 가만히 있을 때 다리 중심값을 바꾸면 바로 90도로 움직여서 수직인지 볼 수 있게 한다
  if (hipChanged && curAction < 0) {
    writeYL(90);
    writeYR(90);
  }
  lastSignal = millis();
  server.send(200, "application/json", calJson());
}

// /cal/save : 지금 값을 보드(NVS)에 저장한다
void handleCalSave() {
  bool ok = ottoSaveCalibration();
  ok = saveDanceTilts() && ok;
  Serial.println(ok ? "보정값 저장 완료" : "보정값 저장 실패");
  server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

// /cal/reset : 저장된 값을 지우고 기본값으로 되돌린다
void handleCalReset() {
  ottoClearCalibration();
  clearDanceTilts();
  ottoSetCalibration(DEF_YL, DEF_YR, DEF_TILT_R, DEF_TILT_L, DEF_RL, DEF_RR);
  if (curAction < 0) {
    writeYL(90);
    writeYR(90);
  }
  Serial.println("보정값을 기본값으로 되돌림");
  server.send(200, "application/json", calJson());
}

// 모르는 주소 처리
//  - Otto 가 만든 Wi-Fi 일 때: 휴대폰이 인터넷 확인용으로 여는 주소를 첫 화면으로 보낸다 (화면 자동으로 뜨기)
//  - 공유기에 접속했을 때: 그냥 "없음"
void handleOther() {
  if (apMode) {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "not found");
  }
}

// ── 동작 (따로 도는 작업) ───────────────────────────────────
// 두 발 정지 신호(1.5ms), 다리 가운데(90도)
void restPose() {
  ottoRollFeet(0, 0);
  writeYL(90);
  writeYR(90);
}

// 댄스 시험 한 번: 한 발로 서서 그 발을 굴려 좌회전 → 우회전 (라이브러리 dance() 의 반쪽)
//   leftStand = true  : 왼발로 서기 (오른발을 든다)
//   leftStand = false : 오른발로 서기 (왼발을 든다)
// 엉덩이 각도는 매번 새로 읽으므로, 화면에서 바꾼 값이 다음 번부터 바로 쓰인다.
void danceStep(bool leftStand, bool &first) {
  int rYr, rYl, lYr, lYl;
  getDanceTilts(rYr, rYl, lYr, lYl);
  if (first) {                      // 처음 한 번은 가운데에서 시작
    ottoRollFeet(0, 0);
    moveHip(90, 90);
    delay(700);
    first = false;
  }
  if (leftStand) {
    moveHip(rYr, rYl);               delay(700);    // 왼발로 서기
    rollLeftFoot(-DANCE_SPEED);      delay(1000);   // 좌회전
    rollLeftFoot(0);                 delay(300);
    rollLeftFoot(DANCE_SPEED);       delay(1000);   // 우회전
    rollLeftFoot(0);                 delay(300);
  } else {
    moveHip(lYr, lYl);               delay(700);    // 오른발로 서기
    rollRightFoot(DANCE_SPEED);      delay(1000);   // 좌회전
    rollRightFoot(0);                delay(300);
    rollRightFoot(-DANCE_SPEED);     delay(1000);   // 우회전
    rollRightFoot(0);                delay(300);
  }
}

// 걷기·미끄러짐은 한 번에 약 1초 걸린다. 그동안에도 웹 요청을 받도록 동작은 따로 돌린다.
// 굴리기는 발을 계속 돌려 두고 바로 돌아오므로 짧게 쉬면서 다시 부른다.
void moveTask(void *) {
  int prevA = -1;      // 바로 전 동작 (댄스를 새로 시작하는지 알려고)
  bool first = true;   // 댄스를 새로 시작하면 true
  for (;;) {
    int a = curAction;
    int v = curSpeed;
    if (a != prevA) { prevA = a; first = true; }
    if (a >= 0 && millis() - lastSignal > SIGNAL_TIMEOUT_MS) {
      Serial.println("화면 신호가 끊겨서 멈춤");
      curAction = -1;
      restPose();  // 굴리던 발도 멈춘다
      continue;
    }
    switch (a) {
      case 0:  walkFront(v);    break;
      case 1:  walkBack(v);     break;
      case 2:  walkLeft();      break;
      case 3:  walkRight();     break;
      case 4:  rollingFront(v); delay(50); break;
      case 5:  rollingBack(v);  delay(50); break;
      case 6:  rollingLeft();   delay(50); break;
      case 7:  rollingRight();  delay(50); break;
      case 8:  slideFront();    break;
      case 9:  slideBack();     break;
      case 10: slideLeft();     break;
      case 11: slideRight();    break;
      // 로봇 맞추기 시험 동작 (보정값을 바꾸면 바로 반영된다)
      case 12: rollingFront(100); delay(50); break;   // 굴리기 시험: 왼발/오른발 속도
      case 13: ottoCalibrationWalk(); break;          // 걷기 시험: 발을 들 때 기울기
      case 14: danceStep(true, first);  break;        // 왼발 댄스
      case 15: danceStep(false, first); break;        // 오른발 댄스
      default: delay(20);       break;
    }
  }
}

void startMoveTask() {
  xTaskCreatePinnedToCore(moveTask, "move", 4096, nullptr, 1, &moveHandle, 1);
}

// [정지]: 하던 동작을 그 자리에서 끊고, 두 발을 멈추고 다리를 가운데로 둔다.
// 걷기·미끄러짐은 한 번(약 1초)이 끝나야 돌아오므로, 동작 작업을 지우고 새로 만든다.
void stopNow() {
  curAction = -1;
  if (moveHandle) {
    vTaskDelete(moveHandle);
    moveHandle = nullptr;
  }
  restPose();
  Serial.println("정지");
  startMoveTask();  // 다음 명령을 받을 수 있게 동작 작업을 다시 만든다
}

// ── OLED 눈 (따로 도는 작업) ────────────────────────────────
// Wi-Fi 가 연결돼 있는지: 공유기 모드면 공유기 접속, Otto Wi-Fi 모드면 휴대폰이 1대 이상 접속
bool wifiOk() {
  if (apMode) return WiFi.softAPgetStationNum() > 0;
  return WiFi.status() == WL_CONNECTED;
}

// 0.05초마다 눈을 다시 그린다. 웹 요청 처리가 느려지지 않게 다른 코어(0번)에서 돈다.
void eyeTask(void *) {
  float px = 0, py = 0;              // 지금 눈동자 위치 (눈 가운데 기준)
  float tx = 0, ty = 0;              // 눈동자가 가려는 위치
  unsigned long nextLook = 0;        // 다음에 다른 곳을 볼 시각
  unsigned long nextBlink = millis() + 3000;  // 다음 깜빡임 시각
  int frame = 0;

  for (;;) {
    unsigned long now = millis();
    frame++;

    // SW3 을 누르면 정보 화면을 다시 보여 주고 잠에서 깬다
    if (digitalRead(SW3_PIN) == LOW) {
      infoUntil = now + INFO_SHOW_MS;
      lastActive = now + INFO_SHOW_MS;
    }

    // 정보 화면을 보여 줄 시간이면 그것만 그린다
    if ((long)(infoUntil - now) > 0) {
      drawInfo();
      delay(100);
      continue;
    }

    int a = curAction;
    if (a >= 0) lastActive = now;    // 움직이는 동안은 깨어 있다

    u8g2.clearBuffer();

    if (!wifiOk()) {
      // 1) Wi-Fi 연결 안 됨 → 찡그린 눈 (눈동자가 살짝 떨린다)
      int shake = (frame / 3) % 2 ? 1 : -1;
      drawFrownEye(EYE_LX, EYE_Y, +1, shake);
      drawFrownEye(EYE_RX, EYE_Y, -1, shake);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(46, 63, "no WiFi");

    } else if (now - lastActive > SLEEP_AFTER_MS && (long)(lastActive - now) <= 0) {
      // 2) 5초 동안 안 움직임 → 잠자는 눈 + Z z
      drawSleepingEye(EYE_LX, EYE_Y - 8);
      drawSleepingEye(EYE_RX, EYE_Y - 8);
      int step = (frame / 10) % 3;   // 0.5초마다 Z 가 하나씩 늘어난다
      u8g2.setFont(u8g2_font_7x13B_tf);
      u8g2.drawStr(56, 60, "z");
      if (step >= 1) u8g2.drawStr(64, 52, "Z");
      if (step >= 2) u8g2.drawStr(73, 44, "Z");
      px = py = 0;                   // 깨어나면 가운데를 보고 시작

    } else {
      // 3) 모두 연결됨 → 눈동자가 움직인다
      if (a >= 0 && a < 12) {
        // 움직이는 방향을 본다 (0 전진, 1 후진, 2 좌회전, 3 우회전)
        int d = a % 4;
        tx = (d == 2) ? -PUPIL_MAX_OFFSET : (d == 3) ? PUPIL_MAX_OFFSET : 0;
        ty = (d == 0) ? -PUPIL_MAX_OFFSET : (d == 1) ? PUPIL_MAX_OFFSET : 0;
      } else if ((long)(now - nextLook) >= 0) {
        // 가만히 있을 때는 이따금 여기저기 둘러본다
        tx = random(-PUPIL_MAX_OFFSET, PUPIL_MAX_OFFSET + 1);
        ty = random(-PUPIL_MAX_OFFSET, PUPIL_MAX_OFFSET + 1);
        nextLook = now + random(600, 2000);
      }
      float dist = sqrt(tx * tx + ty * ty);     // 대각선으로도 눈 밖으로 나가지 않게 한다
      if (dist > PUPIL_MAX_OFFSET) {
        tx = tx * PUPIL_MAX_OFFSET / dist;
        ty = ty * PUPIL_MAX_OFFSET / dist;
      }
      px += (tx - px) * 0.3;         // 부드럽게 따라간다
      py += (ty - py) * 0.3;

      bool blinking = (long)(now - nextBlink) >= 0;
      if (blinking && now - nextBlink > 150) {   // 0.15초 감았다가 다시 뜬다
        nextBlink = now + random(2000, 5000);
        blinking = false;
      }
      if (blinking) {
        drawBlinkEye(EYE_LX, EYE_Y);
        drawBlinkEye(EYE_RX, EYE_Y);
      } else {
        drawOpenEye(EYE_LX, EYE_Y, (int)px, (int)py);
        drawOpenEye(EYE_RX, EYE_Y, (int)px, (int)py);
      }
    }

    u8g2.sendBuffer();
    delay(50);
  }
}

void setup() {
  Serial.begin(115200);
  ottoInit();  // 저장된 보정값을 읽고 다리를 가운데로

  pinMode(SW3_PIN, INPUT);  // SW3 은 보드에 풀업 저항이 있다

  Wire.begin(18, 17);       // OLED I2C: SDA=18, SCL=17
  Wire.setClock(400000);    // 화면을 빨리 그리도록 I2C 를 빠르게
  u8g2.setBusClock(400000);
  u8g2.begin();
  showLcd("Otto Web Calib", "Connecting...", String("WiFi: ") + WIFI_SSID, "");

  // Wi-Fi 를 먼저 켜야 MAC 주소를 제대로 읽는다 (안 켜면 00:00:... 이 나온다)
  WiFi.mode(WIFI_STA);

  // 이름 끝 4글자는 보드마다 다르다 (MAC 주소). Wi-Fi 이름과 .local 이름에 쓴다
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String tail = mac.substring(8);
  tail.toLowerCase();

  // 1) 공유기에 접속한다
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("공유기 '%s' 에 접속 중", WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_WAIT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  IPAddress ip;
  if (WiFi.status() == WL_CONNECTED) {
    ip = WiFi.localIP();
    String host = "otto-" + tail;
    MDNS.begin(host.c_str());  // http://otto-xxxx.local 로도 열 수 있게
    MDNS.addService("http", "tcp", 80);
    Serial.println("공유기 접속 성공");
    Serial.println("주소: http://" + ip.toString());
    Serial.println("이름 주소: http://" + host + ".local");
    showLcd("Open in browser:", ip.toString(), String("WiFi: ") + WIFI_SSID, host + ".local");
  } else {
    // 2) 실패하면 Otto 가 직접 Wi-Fi 를 만든다
    apMode = true;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    String ssid = "Otto-" + mac.substring(8);  // 예: Otto-A1B2
    WiFi.softAP(ssid.c_str(), strlen(AP_PASS) >= 8 ? AP_PASS : nullptr);
    delay(100);
    ip = WiFi.softAPIP();
    dns.start(53, "*", ip);  // 모든 주소를 Otto 로 → 접속하면 화면이 자동으로 뜬다
    Serial.println("공유기 접속 실패: Otto 가 직접 Wi-Fi 를 만듭니다");
    Serial.println("Wi-Fi 이름: " + ssid);
    Serial.printf("비밀번호: %s\n", strlen(AP_PASS) >= 8 ? AP_PASS : "(없음)");
    Serial.println("주소: http://" + ip.toString());
    showLcd("Open in browser:", ip.toString(), "WiFi: " + ssid,
            String("PW: ") + (strlen(AP_PASS) >= 8 ? AP_PASS : "(none)"));
  }

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/cal/get", handleCalGet);
  server.on("/cal/set", handleCalSet);
  server.on("/cal/save", handleCalSave);
  server.on("/cal/reset", handleCalReset);
  server.onNotFound(handleOther);
  server.begin();

  startMoveTask();

  // 정보 화면을 5초 동안 보여 준 뒤 눈을 그린다 (그 뒤 5초 동안은 깨어 있는 눈)
  infoUntil = millis() + INFO_SHOW_MS;
  lastActive = infoUntil;
  xTaskCreatePinnedToCore(eyeTask, "eyes", 4096, nullptr, 1, nullptr, 0);
}

void loop() {
  if (apMode) dns.processNextRequest();
  server.handleClient();
  delay(2);
}
