// Otto 웹 조종 + OLED 눈 (가이드 4.1.2 2단계)
// 1단계 otto_web_modes 에 OLED 눈 화면(움직이는 눈, 잠자는 눈, 찡그린 눈)을 더한 프로그램이다.
//
// OLED 눈 화면
//   - 처음 켜지면 접속 주소 같은 정보를 5초 동안 보여 준 뒤 눈을 그린다.
//   - Wi-Fi 연결이 안 되면 → 찡그린 눈
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
const int PUPIL_R = 12;                          // 눈동자 크기

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
const char *WIFI_SSID = "공유기이름";   // 공유기 이름 (SSID)
const char *WIFI_PASS = "비밀번호";     // 공유기 비밀번호
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
};
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
const char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ko">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Otto 조종</title>
<style>
  body { margin:0; font-family:sans-serif; background:#f4f6f9; color:#1f2937;
         display:flex; flex-direction:column; align-items:center; padding:0 16px; }
  h1 { font-size:20px; margin:20px 0 12px; }
  .modes { display:flex; gap:4px; padding:5px; background:#eef1f5; border-radius:14px; margin-bottom:12px; }
  .modes button { border:none; background:none; padding:10px 16px; border-radius:10px;
                  font-size:15px; font-weight:bold; color:#475569; cursor:pointer; }
  .modes button.on { background:#2563eb; color:#fff; box-shadow:0 2px 6px rgba(37,99,235,.35); }
  #state { font-size:16px; margin-bottom:8px; color:#475569; min-height:20px; }
  #state b { color:#2563eb; }
  svg { width:min(92vw,380px); height:auto; touch-action:none; user-select:none; }
  .sec { fill:#fafbfc; }
  .sec.on { fill:#2563eb; }
  .small { font-size:11px; fill:#64748b; pointer-events:none; }
  .dir { font-size:20px; font-weight:bold; fill:#1f2937; pointer-events:none; }
  .on-text { fill:#fff; }
  #msg { height:20px; font-size:13px; color:#94a3b8; margin-top:8px; }
  .help { font-size:13px; color:#64748b; text-align:center; margin:4px 0 20px; }
</style>
</head>
<body>
<h1>Otto 조종</h1>

<div class="modes">
  <button data-m="w" class="on">걷기 (walk)</button>
  <button data-m="r">굴리기 (roll)</button>
  <button data-m="s">미끄러짐 (slide)</button>
</div>

<div id="state">정지</div>

<svg id="pad" viewBox="0 0 320 320">
  <circle cx="160" cy="160" r="156" fill="#eef1f5"/>
  <!-- 네 구역 (전진/우회전/후진/좌회전) -->
  <path id="sec-f" class="sec" d="M160 160 L61 61 A140 140 0 0 1 259 61 Z"/>
  <path id="sec-r" class="sec" d="M160 160 L259 61 A140 140 0 0 1 259 259 Z"/>
  <path id="sec-b" class="sec" d="M160 160 L259 259 A140 140 0 0 1 61 259 Z"/>
  <path id="sec-l" class="sec" d="M160 160 L61 259 A140 140 0 0 1 61 61 Z"/>
  <circle cx="160" cy="160" r="140" fill="none" stroke="#94a3b8" stroke-width="3"/>
  <!-- X 선 -->
  <line x1="61" y1="61" x2="259" y2="259" stroke="#cbd5e1"/>
  <line x1="259" y1="61" x2="61" y2="259" stroke="#cbd5e1"/>
  <!-- 느리게/빠르게 경계 점선 (속도를 쓰는 모드에서만) -->
  <circle id="ring" cx="160" cy="160" r="80" fill="none" stroke="#cbd5e1" stroke-dasharray="4 4"/>
  <!-- 눌린 위치 -->
  <circle id="dot" cx="160" cy="160" r="9" fill="#f59e0b" opacity="0"/>
  <!-- 글자 -->
  <g id="spd-f">
    <text x="160" y="35" text-anchor="middle" class="small">빠르게 200</text>
    <text x="160" y="102" text-anchor="middle" class="small">느리게 40</text>
  </g>
  <g id="spd-b">
    <text x="160" y="226" text-anchor="middle" class="small">느리게 40</text>
    <text x="160" y="293" text-anchor="middle" class="small">빠르게 200</text>
  </g>
  <text id="txt-f" x="160" y="72"  text-anchor="middle" class="dir">전진</text>
  <text id="txt-b" x="160" y="262" text-anchor="middle" class="dir">후진</text>
  <text id="txt-l" x="62"  y="166" text-anchor="middle" class="dir">좌회전</text>
  <text id="txt-r" x="258" y="166" text-anchor="middle" class="dir">우회전</text>
  <!-- 정지 버튼 -->
  <circle cx="160" cy="160" r="42" fill="#ffe4e6" stroke="#fda4af" stroke-width="4"/>
  <text x="160" y="168" text-anchor="middle" style="font-size:22px;font-weight:bold;fill:#e11d48;pointer-events:none">정지</text>
</svg>
<div id="msg"></div>
<div class="help">구역을 누르면 정지할 때까지 계속 움직입니다. 가운데 [정지]를 누르면 멈춥니다.</div>

<script>
const R_STOP = 42, R_OUT = 140, V_MIN = 40, V_MAX = 200;
const MODE_NAME = { w: '걷기', r: '굴리기', s: '미끄러짐' };
const DIR_NAME = { f: '전진', b: '후진', l: '좌회전', r: '우회전' };
const pad = document.getElementById('pad');
const dot = document.getElementById('dot');
const state = document.getElementById('state');
const msg = document.getElementById('msg');

let mode = 'w';          // w 걷기, r 굴리기, s 미끄러짐
let dir = '';            // '' 이면 정지, f b l r
let speed = 0;           // 속도를 쓰는 동작일 때만 40~200
let sentKey = '', lastSend = 0, pressing = false, waiting = null;

const usesSpeed = (m, d) => (m === 'w' || m === 'r') && (d === 'f' || d === 'b');

// 로봇에 보낸다. 너무 자주 보내면 로봇이 바빠지므로 0.1초에 한 번만 보내고,
// 그사이 바뀐 값은 0.1초 뒤에 꼭 보낸다 (손가락을 밀면 속도가 끊김 없이 따라간다).
function send(force) {
  const a = dir ? mode + dir : 'stop';
  const key = a + ':' + speed;
  const now = Date.now();
  if (!force && key === sentKey) return;
  if (!force && now - lastSend < 100) {
    if (!waiting) waiting = setTimeout(() => { waiting = null; send(false); }, 100 - (now - lastSend));
    return;
  }
  lastSend = now; sentKey = key;
  fetch('/move?a=' + a + '&v=' + speed, { cache: 'no-store' })
    .then(() => { msg.textContent = ''; })
    .catch(() => { msg.textContent = '로봇과 연결이 끊겼어요'; });
}

// 화면을 지금 상태에 맞게 그린다
function show() {
  for (const d of ['f', 'b', 'l', 'r']) {
    const on = d === dir;
    document.getElementById('sec-' + d).classList.toggle('on', on);
    document.getElementById('txt-' + d).classList.toggle('on-text', on);
  }
  const spdMode = mode === 'w' || mode === 'r';
  document.getElementById('spd-f').style.display = spdMode && dir !== 'f' ? '' : 'none';
  document.getElementById('spd-b').style.display = spdMode && dir !== 'b' ? '' : 'none';
  document.getElementById('ring').style.display = spdMode ? '' : 'none';
  if (!dir) {
    state.textContent = '정지 (' + MODE_NAME[mode] + ')';
    dot.setAttribute('opacity', 0);
  } else {
    state.innerHTML = MODE_NAME[mode] + ' ' + DIR_NAME[dir] + (usesSpeed(mode, dir) ? ' <b>' + speed + '</b>' : '');
  }
}

function stop() { dir = ''; speed = 0; show(); send(true); }

// 모드 버튼: 바꾸면 먼저 멈춘다
document.querySelectorAll('.modes button').forEach(b => {
  b.addEventListener('click', () => {
    if (b.dataset.m === mode) return;
    mode = b.dataset.m;
    document.querySelectorAll('.modes button').forEach(x => x.classList.toggle('on', x === b));
    stop();
  });
});

// 눌린 위치로 방향과 속도를 정한다
function handle(e) {
  const p = pad.createSVGPoint();
  p.x = e.clientX; p.y = e.clientY;
  const q = p.matrixTransform(pad.getScreenCTM().inverse());
  const dx = q.x - 160, dy = q.y - 160, r = Math.hypot(dx, dy);

  if (r <= R_STOP) { stop(); return; }   // 정지 버튼
  if (r > R_OUT + 12) return;            // 원 바깥

  dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'r' : 'l') : (dy < 0 ? 'f' : 'b');
  if (usesSpeed(mode, dir)) {
    const t = Math.min(1, (r - R_STOP) / (R_OUT - R_STOP));      // 0(가운데) ~ 1(바깥)
    speed = Math.round(V_MIN + t * (V_MAX - V_MIN));              // 40~200 사이로 끊김 없이
    dot.setAttribute('cx', q.x); dot.setAttribute('cy', q.y);
    dot.setAttribute('opacity', 1);
  } else {
    speed = 0;
    dot.setAttribute('opacity', 0);
  }
  show(); send(false);
}

pad.addEventListener('pointerdown', e => { pressing = true; pad.setPointerCapture(e.pointerId); handle(e); });
pad.addEventListener('pointermove', e => { if (pressing) handle(e); });
pad.addEventListener('pointerup', () => { pressing = false; send(true); });
pad.addEventListener('pointercancel', () => { pressing = false; });

// 움직이는 동안 1초마다 "아직 연결돼 있어요" 신호를 보낸다 (끊기면 로봇이 3초 뒤 스스로 멈춤)
setInterval(() => { if (dir) send(true); }, 1000);
show();
</script>
</body>
</html>
)HTML";

// ── 웹 요청 처리 ────────────────────────────────────────────
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

// /move?a=동작코드&v=속도   (a=stop 이면 정지)
void handleMove() {
  String a = server.arg("a");
  int idx = -1;
  for (int i = 0; i < ACTION_COUNT; i++) {
    if (a == ACTIONS[i]) { idx = i; break; }
  }
  lastSignal = millis();

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

// 걷기·미끄러짐은 한 번에 약 1초 걸린다. 그동안에도 웹 요청을 받도록 동작은 따로 돌린다.
// 굴리기는 발을 계속 돌려 두고 바로 돌아오므로 짧게 쉬면서 다시 부른다.
void moveTask(void *) {
  for (;;) {
    int a = curAction;
    int v = curSpeed;
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
      if (a >= 0) {
        // 움직이는 방향을 본다 (0 전진, 1 후진, 2 좌회전, 3 우회전)
        int d = a % 4;
        tx = (d == 2) ? -10 : (d == 3) ? 10 : 0;   // 눈동자가 눈 밖으로 나가지 않게 (24 - 12 - 2 = 10)
        ty = (d == 0) ? -9 : (d == 1) ? 9 : 0;
      } else if ((long)(now - nextLook) >= 0) {
        // 가만히 있을 때는 이따금 여기저기 둘러본다
        tx = random(-10, 11);
        ty = random(-9, 10);
        nextLook = now + random(600, 2000);
      }
      float dist = sqrt(tx * tx + ty * ty);     // 대각선으로도 눈 밖으로 나가지 않게 10 이내로
      if (dist > 10) { tx = tx * 10 / dist; ty = ty * 10 / dist; }
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
  showLcd("Otto Web Eyes", "Connecting...", String("WiFi: ") + WIFI_SSID, "");

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
