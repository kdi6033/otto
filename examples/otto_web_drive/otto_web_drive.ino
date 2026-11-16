// Otto 웹 조종 네 방향 (가이드 4.2): 휴대폰/PC 브라우저로 전진, 후진, 좌회전, 우회전을 한다.
// 4.1 otto_web_walk 에 후진, 좌회전, 우회전을 더한 프로그램이다.
//
// 사용법
//  1. 아래 WIFI_SSID, WIFI_PASS 에 공유기(ipTIME 등) 이름과 비밀번호를 넣고 업로드한다.
//  2. OLED 화면이나 시리얼 모니터(115200)에 나온 주소(예: http://192.168.0.37)를 같은 공유기에 접속한
//     휴대폰/PC 브라우저에서 연다. http://otto-xxxx.local 처럼 이름으로도 열 수 있다(안드로이드는 안 될 수 있음).
//  * 공유기 접속에 실패하면 Otto 가 직접 Wi-Fi "Otto-XXXX"(비밀번호 AP_PASS)를 만든다.
//    그 Wi-Fi 에 접속하면 화면이 자동으로 뜨고, 안 뜨면 http://192.168.4.1 을 연다.
//  3. 원의 전진/후진/좌회전/우회전 구역을 누르면 정지할 때까지 계속 걷는다. 가운데 [정지]를 누르면 멈춘다.
//     전진/후진은 가운데에 가까우면 느리게(40), 바깥쪽이면 빠르게(200).
//
// 걷기 함수
//   walkFront(속도)  walkBack(속도)  walkLeft()  walkRight()
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

// OLED 에 네 줄을 쓴다: 제목(작게), 주소(크게), 아래 두 줄(작게)
void showLcd(const String &title, const String &big, const String &line3, const String &line4) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, title.c_str());
  u8g2.setFont(u8g2_font_7x13B_tf);   // 주소는 굵고 크게 (IP 15자까지 한 줄)
  u8g2.drawStr(0, 30, big.c_str());
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 48, line3.c_str());
  u8g2.drawStr(0, 62, line4.c_str());
  u8g2.sendBuffer();
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

// 동작 코드 (i2r.link 웹 앱과 같은 이름). 순서가 곧 번호다.
//   w=걷기,  f=전진 b=후진 l=좌회전 r=우회전
const char *ACTIONS[] = { "wf", "wb", "wl", "wr" };
const int ACTION_COUNT = sizeof(ACTIONS) / sizeof(ACTIONS[0]);

// 속도를 쓰는 동작: 전진/후진
bool usesSpeed(int a) {
  return a == 0 || a == 1;
}

WebServer server(80);
DNSServer dns;

volatile int curAction = -1;         // 지금 동작 번호 (-1 이면 정지)
volatile int curSpeed = 0;           // 전진/후진 속도 (40~200)
volatile unsigned long lastSignal = 0;  // 마지막으로 화면 신호를 받은 시각
TaskHandle_t moveHandle = nullptr;   // 동작 작업 (정지할 때 지운다)

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
  <!-- 느리게/빠르게 경계 점선 -->
  <circle cx="160" cy="160" r="80" fill="none" stroke="#cbd5e1" stroke-dasharray="4 4"/>
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
const DIR_NAME = { f: '전진', b: '후진', l: '좌회전', r: '우회전' };
const pad = document.getElementById('pad');
const dot = document.getElementById('dot');
const state = document.getElementById('state');
const msg = document.getElementById('msg');

const mode = 'w';        // 걷기만 (4.3 에서 굴리기, 미끄러짐을 더한다)
let dir = '';            // '' 이면 정지, f b l r
let speed = 0;           // 전진/후진일 때만 40~200
let sentKey = '', lastSend = 0, pressing = false;

const usesSpeed = d => d === 'f' || d === 'b';

// 로봇에 보낸다 (같은 값은 1초에 한 번, 다른 값도 0.15초에 한 번만)
function send(force) {
  const a = dir ? mode + dir : 'stop';
  const key = a + ':' + speed;
  const now = Date.now();
  if (!force && key === sentKey && now - lastSend < 1000) return;
  if (!force && now - lastSend < 150) return;
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
  document.getElementById('spd-f').style.display = dir !== 'f' ? '' : 'none';
  document.getElementById('spd-b').style.display = dir !== 'b' ? '' : 'none';
  if (!dir) {
    state.textContent = '정지';
    dot.setAttribute('opacity', 0);
  } else {
    state.innerHTML = DIR_NAME[dir] + (usesSpeed(dir) ? ' <b>' + speed + '</b>' : '');
  }
}

function stop() { dir = ''; speed = 0; show(); send(true); }

// 눌린 위치로 방향과 속도를 정한다
function handle(e) {
  const p = pad.createSVGPoint();
  p.x = e.clientX; p.y = e.clientY;
  const q = p.matrixTransform(pad.getScreenCTM().inverse());
  const dx = q.x - 160, dy = q.y - 160, r = Math.hypot(dx, dy);

  if (r <= R_STOP) { stop(); return; }   // 정지 버튼
  if (r > R_OUT + 12) return;            // 원 바깥

  dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'r' : 'l') : (dy < 0 ? 'f' : 'b');
  if (usesSpeed(dir)) {
    const t = Math.min(1, (r - R_STOP) / (R_OUT - R_STOP));      // 0(가운데) ~ 1(바깥)
    speed = Math.round((V_MIN + t * (V_MAX - V_MIN)) / 10) * 10;  // 10 단위
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
      // 다른 방향으로 바뀌면 먼저 멈추고 다리를 가운데로 둔 뒤 시작한다
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

// 걷기 함수는 한 번에 약 1초 걸린다. 그동안에도 웹 요청을 받도록 동작은 따로 돌린다.
void moveTask(void *) {
  for (;;) {
    int a = curAction;
    int v = curSpeed;
    if (a >= 0 && millis() - lastSignal > SIGNAL_TIMEOUT_MS) {
      Serial.println("화면 신호가 끊겨서 멈춤");
      curAction = -1;
      restPose();
      continue;
    }
    switch (a) {
      case 0:  walkFront(v); break;
      case 1:  walkBack(v);  break;
      case 2:  walkLeft();   break;
      case 3:  walkRight();  break;
      default: delay(20);    break;
    }
  }
}

void startMoveTask() {
  xTaskCreatePinnedToCore(moveTask, "move", 4096, nullptr, 1, &moveHandle, 1);
}

// [정지]: 하던 동작을 그 자리에서 끊고, 두 발을 멈추고 다리를 가운데로 둔다.
// 걷기 함수는 한 걸음(약 1초)이 끝나야 돌아오므로, 동작 작업을 지우고 새로 만든다.
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

void setup() {
  Serial.begin(115200);
  ottoInit();  // 저장된 보정값을 읽고 다리를 가운데로

  Wire.begin(18, 17);  // OLED I2C: SDA=18, SCL=17
  u8g2.begin();
  showLcd("Otto Web Drive", "Connecting...", String("WiFi: ") + WIFI_SSID, "");

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
}

void loop() {
  if (apMode) dns.processNextRequest();
  server.handleClient();
  delay(2);
}
