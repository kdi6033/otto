// 웹 화면 (HTML). otto_web_calib.ino 가 #include "page.h" 로 불러 쓴다.
// Arduino 는 .ino 안의 긴 HTML 글자 속 JavaScript(function)를 C++ 로 잘못 읽을 수 있어서 따로 둔다.
#pragma once
#include <Arduino.h>

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
  .help { font-size:13px; color:#64748b; text-align:center; margin:4px 0 12px; }
  [hidden] { display:none !important; }
  #ctl { display:flex; flex-direction:column; align-items:center; }
  #openCal { border:1px solid #cbd5e1; background:#fff; color:#334155; font-size:15px; font-weight:bold;
             padding:10px 20px; border-radius:12px; margin-bottom:24px; cursor:pointer; }

  /* ── 로봇 맞추기 화면 ── */
  #cal { width:min(100%,980px); box-sizing:border-box; background:#fff; border:1px solid #e5e7eb;
         border-radius:16px; padding:16px 20px; margin:16px 0 24px; }
  .calhead { display:flex; align-items:center; gap:10px; }
  .calhead h2 { font-size:16px; margin:0; }
  .yt { display:inline-flex; align-items:center; gap:6px; border:1px solid #e5e7eb; border-radius:8px;
        padding:5px 10px; font-size:12px; color:#1f2937; text-decoration:none; font-weight:bold; }
  .yt i { display:inline-block; width:16px; height:11px; background:#ff0000; border-radius:3px; position:relative; }
  .yt i:after { content:''; position:absolute; left:6px; top:3px; border-left:5px solid #fff;
                border-top:3px solid transparent; border-bottom:3px solid transparent; }
  #closeCal { margin-left:auto; border:1px solid #cbd5e1; background:#f1f5f9; border-radius:8px;
              padding:6px 12px; font-size:13px; font-weight:bold; cursor:pointer; }
  #cal h3 { font-size:14px; margin:16px 0 4px; }
  #cal h4 { font-size:13px; margin:12px 0 4px; }
  .desc { font-size:12px; color:#64748b; margin:2px 0 8px; line-height:1.5; }
  .btns { display:flex; flex-wrap:wrap; gap:8px; margin:6px 0 8px; }
  .b { border:none; border-radius:8px; padding:10px 16px; color:#fff; font-size:13px; font-weight:bold; cursor:pointer; }
  .teal { background:#0e95a8; } .green { background:#16a34a; } .orange { background:#f59e0b; }
  .blue { background:#3b82f6; } .gray { background:#64748b; } .red { background:#ef4444; }
  .row { display:flex; align-items:center; gap:6px; margin:6px 0; }
  .row > span { width:150px; font-size:13px; flex-shrink:0; }
  .row input[type=range] { flex:1; min-width:60px; accent-color:#2563eb; }
  .row b { width:64px; text-align:right; font-size:13px; flex-shrink:0; }
  .st { border:1px solid #c7d2fe; background:#eef2ff; color:#1e3a8a; border-radius:6px;
        padding:4px 7px; font-size:12px; cursor:pointer; flex-shrink:0; }
  #calState, #calNow { font-size:12px; color:#475569; }
  #calNote { font-size:11px; color:#94a3b8; margin-top:8px; }
  @media (max-width:560px) {
    #cal { padding:14px 12px; }
    .row { flex-wrap:wrap; }
    .row > span { width:100%; }
  }
</style>
</head>
<body>
<h1>Otto 조종</h1>
<div id="ctl">

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
<button id="openCal">⚙ 로봇 맞추기</button>
</div>

<!-- ── 로봇 맞추기 화면 ── -->
<div id="cal" hidden>
  <div class="calhead">
    <h2>로봇 맞추기</h2>
    <a class="yt" href="https://youtu.be/_-mYb3v9v7k" target="_blank"><i></i>유튜브 보기</a>
    <button id="closeCal">닫기</button>
  </div>
  <div id="calState">보드의 현재 값을 읽는 중... (응답이 없으면 보드가 켜져 있고 연결됐는지 확인하세요)</div>

  <h3>1. 조립 자세 (다리 0도 / 180도)</h3>
  <div class="desc">이 자세로 다리를 끼우고 나사를 조이세요. (오른쪽 다리 0도, 왼쪽 다리 180도)</div>
  <div class="btns"><button class="b teal" data-act="p0">모터를 0도 / 180도로 움직이기</button></div>

  <h3>2. 다리를 수직(90도)으로 맞추기</h3>
  <div class="desc">[모터를 90도로 움직이기]를 누른 뒤, 두 다리가 정확히 수직이 되도록 숫자를 조금씩 바꾸세요. 바꾸는 즉시 다리가 움직입니다.</div>
  <div class="btns"><button class="b teal" data-act="p90">모터를 90도로 움직이기</button></div>
  <div class="row" data-k="yl" data-min="1000" data-max="1900" data-big="10" data-unit="us"><span>왼쪽 다리 (YL)</span></div>
  <div class="row" data-k="yr" data-min="1000" data-max="1900" data-big="10" data-unit="us"><span>오른쪽 다리 (YR)</span></div>

  <h3>3. 굴리기 시험 (두 발 속도)</h3>
  <div class="desc">앞으로 굴리면서 로봇이 똑바로 가도록 두 발의 속도를 맞추세요.</div>
  <div class="btns"><button class="b green" data-act="cr">앞으로 굴리기</button><button class="b orange" data-act="stop">정지</button></div>
  <div class="row" data-k="rl" data-min="0" data-max="600" data-big="10" data-unit="us"><span>왼발 속도 (RL)</span></div>
  <div class="row" data-k="rr" data-min="0" data-max="600" data-big="10" data-unit="us"><span>오른발 속도 (RR)</span></div>

  <h3>4. 걷기 시험 (발을 들 때 기울기)</h3>
  <div class="desc">[걷기 시작]으로 앞으로 걷게 하고, 걸을 때 발이 수평으로 들리도록 기울기를 조금씩 바꾸세요. 로봇이 안 들리면 올리고, 한쪽으로 돌면 그쪽을 더 올립니다. (먼저 2번에서 다리 수직을 맞추세요)</div>
  <div class="btns"><button class="b green" data-act="cw">걷기 시작</button><button class="b orange" data-act="stop">정지</button></div>
  <div class="row" data-k="tr" data-min="0" data-max="45" data-big="5" data-unit="도"><span>오른발을 들 때</span></div>
  <div class="row" data-k="tl" data-min="0" data-max="45" data-big="5" data-unit="도"><span>왼발을 들 때</span></div>

  <h3>5. 댄스 시험 (한 발로 설 때 엉덩이 각도)</h3>
  <div class="desc">[왼발 댄스] / [오른발 댄스]로 한쪽씩 따로 시험하고, 한 발로 설 때 반대쪽 발이 잘 들리면서 넘어지지 않도록 각도를 조금씩 바꾸세요. 발이 덜 들리면 90에서 더 멀게 하고, 넘어지면 줄입니다. 왼발 서기는 90보다 크게(90~150), 오른발 서기는 90보다 작게(30~90) 맞춥니다. 서보 보호를 위해 오른쪽 엉덩이(YR)는 135 이하, 왼쪽 엉덩이(YL)는 55 이상으로 자동 제한됩니다. (넘어지기 쉬우니 손으로 받치거나 푹신한 바닥에서 시험하세요.)</div>
  <h4>왼발 댄스 (왼발로 서서 좌회전 ↔ 우회전)</h4>
  <div class="btns"><button class="b green" data-act="dl">왼발 댄스</button><button class="b orange" data-act="stop">정지</button></div>
  <div class="row" data-k="ryr" data-min="0" data-max="135" data-big="5" data-unit="도"><span>오른쪽 엉덩이 (YR)</span></div>
  <div class="row" data-k="ryl" data-min="55" data-max="180" data-big="5" data-unit="도"><span>왼쪽 엉덩이 (YL)</span></div>
  <h4>오른발 댄스 (오른발로 서서 좌회전 ↔ 우회전)</h4>
  <div class="btns"><button class="b green" data-act="dr">오른발 댄스</button><button class="b orange" data-act="stop">정지</button></div>
  <div class="row" data-k="lyr" data-min="0" data-max="135" data-big="5" data-unit="도"><span>오른쪽 엉덩이 (YR)</span></div>
  <div class="row" data-k="lyl" data-min="55" data-max="180" data-big="5" data-unit="도"><span>왼쪽 엉덩이 (YL)</span></div>

  <div class="btns" style="margin-top:16px">
    <button class="b blue" id="calSave">저장 (보드 NVS)</button>
    <button class="b gray" id="calCopy">값 복사</button>
    <button class="b red" id="calReset">기본값으로</button>
  </div>
  <div id="calNow"></div>
  <div id="calNote">참고: 웹에서 펌웨어를 다시 설치하면 보드의 저장값이 지워지니 [값 복사]로 기록해 두세요.</div>
</div>

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

// ── 로봇 맞추기 ─────────────────────────────────────────────
const ctl = document.getElementById('ctl'), cal = document.getElementById('cal');
const calState = document.getElementById('calState'), calNow = document.getElementById('calNow');
const LOOP_ACTS = ['cr', 'cw', 'dl', 'dr'];   // 정지할 때까지 계속하는 시험 동작
const ACT_NAME = { p0: '0도 / 180도 자세', p90: '90도 자세', cr: '앞으로 굴리는 중', cw: '걷는 중',
                   dl: '왼발 댄스 중', dr: '오른발 댄스 중', stop: '정지' };
let cv = {};          // 지금 값 (yl yr rl rr tr tl ryr ryl lyr lyl)
let calAct = '';      // 지금 하고 있는 시험 동작 ('' 이면 없음)
let setTimer = null;

// 슬라이더 줄 만들기: [-큰값] [-1] ──●── [+1] [+큰값]  값
document.querySelectorAll('.row').forEach(r => {
  const k = r.dataset.k, big = +r.dataset.big;
  const step = (t, d) => {
    const b = document.createElement('button');
    b.className = 'st'; b.textContent = t;
    b.onclick = () => setVal(k, cv[k] + d);
    return b;
  };
  const rng = document.createElement('input');
  rng.type = 'range'; rng.min = r.dataset.min; rng.max = r.dataset.max; rng.step = 1; rng.id = 'r-' + k;
  rng.oninput = () => setVal(k, +rng.value);
  const out = document.createElement('b');
  out.id = 'v-' + k; out.dataset.unit = r.dataset.unit;
  r.append(step('-' + big, -big), step('-1', -1), rng, step('+1', 1), step('+' + big, big), out);
});

function valText() {
  return `YL ${cv.yl} / YR ${cv.yr} / 기울기 ${cv.tr}, ${cv.tl} / 속도 ${cv.rl}, ${cv.rr} / ` +
         `댄스 ${cv.ryr}, ${cv.ryl}, ${cv.lyr}, ${cv.lyl}`;
}

function showVals() {
  for (const k in cv) {
    const r = document.getElementById('r-' + k);
    if (!r) continue;
    r.value = cv[k];
    const o = document.getElementById('v-' + k);
    o.textContent = cv[k] + ' ' + o.dataset.unit;
  }
  calNow.textContent = '현재 값: ' + valText();
}

// 값 하나를 바꾸고, 0.15초 뒤에 모든 값을 로봇에 보낸다 (슬라이더를 끄는 동안 너무 자주 보내지 않게)
function setVal(k, x) {
  if (cv[k] === undefined) return;
  const r = document.getElementById('r-' + k);
  cv[k] = Math.max(+r.min, Math.min(+r.max, Math.round(x)));
  showVals();
  clearTimeout(setTimer);
  setTimer = setTimeout(() => {
    const q = Object.keys(cv).map(k => k + '=' + cv[k]).join('&');
    fetch('/cal/set?' + q, { cache: 'no-store' })
      .catch(() => { calState.textContent = '로봇과 연결이 끊겼어요'; });
  }, 150);
}

// 로봇에서 받은 값으로 화면을 채운다
function takeVals(j) {
  cv = { yl: j.yl, yr: j.yr, rl: j.rl, rr: j.rr, tr: j.tr, tl: j.tl,
         ryr: j.ryr, ryl: j.ryl, lyr: j.lyr, lyl: j.lyl };
  showVals();
}

function loadVals() {
  calState.textContent = '보드의 현재 값을 읽는 중... (응답이 없으면 보드가 켜져 있고 연결됐는지 확인하세요)';
  fetch('/cal/get', { cache: 'no-store' }).then(r => r.json()).then(j => {
    takeVals(j);
    calState.textContent = j.saved ? '보드에 저장된 값을 불러왔어요.' : '저장된 값이 없어 기본값을 보여 줍니다. 맞춘 뒤 [저장]을 누르세요.';
  }).catch(() => { calState.textContent = '값을 읽지 못했어요. 보드가 켜져 있고 연결됐는지 확인하세요.'; });
}

// 시험 동작 버튼 (p0 p90 cr cw dl dr stop)
function calDo(a) {
  calAct = LOOP_ACTS.includes(a) ? a : '';
  fetch('/move?a=' + a, { cache: 'no-store' })
    .then(() => { calState.textContent = ACT_NAME[a] || ''; })
    .catch(() => { calState.textContent = '로봇과 연결이 끊겼어요'; });
}
document.querySelectorAll('[data-act]').forEach(b => b.onclick = () => calDo(b.dataset.act));
// 시험 동작 중에는 1초마다 "아직 연결돼 있어요" 신호 (끊기면 로봇이 3초 뒤 스스로 멈춤)
setInterval(() => { if (calAct) fetch('/move?a=' + calAct, { cache: 'no-store' }).catch(() => {}); }, 1000);

document.getElementById('openCal').onclick = () => {
  stop();
  ctl.hidden = true; cal.hidden = false;
  document.querySelector('h1').textContent = 'Otto 로봇 맞추기';
  loadVals();
};
document.getElementById('closeCal').onclick = () => {
  calDo('stop');
  cal.hidden = true; ctl.hidden = false;
  document.querySelector('h1').textContent = 'Otto 조종';
};

document.getElementById('calSave').onclick = () => {
  fetch('/cal/save', { cache: 'no-store' }).then(r => r.json()).then(j => {
    calState.textContent = j.ok ? '✅ 보드에 저장했어요. 다른 프로그램도 ottoInit() 으로 이 값을 씁니다.' : '저장에 실패했어요. 다시 눌러 보세요.';
  }).catch(() => { calState.textContent = '로봇과 연결이 끊겼어요'; });
};

// 값 복사: http 주소에서는 navigator.clipboard 가 막힐 수 있어 예전 방법도 함께 쓴다
document.getElementById('calCopy').onclick = () => {
  const t = valText();
  const old = () => {
    const ta = document.createElement('textarea');
    ta.value = t; document.body.appendChild(ta); ta.select();
    let ok = false; try { ok = document.execCommand('copy'); } catch (e) {}
    ta.remove();
    calState.textContent = ok ? '값을 복사했어요: ' + t : '복사가 안 돼요. 아래 값을 직접 적어 두세요.';
  };
  if (navigator.clipboard && window.isSecureContext) {
    navigator.clipboard.writeText(t).then(() => { calState.textContent = '값을 복사했어요: ' + t; }).catch(old);
  } else old();
};

document.getElementById('calReset').onclick = () => {
  if (!confirm('보드에 저장된 값을 지우고 기본값으로 되돌릴까요?')) return;
  fetch('/cal/reset', { cache: 'no-store' }).then(r => r.json()).then(j => {
    takeVals(j);
    calState.textContent = '기본값으로 되돌렸어요. (저장된 값은 지웠습니다)';
  }).catch(() => { calState.textContent = '로봇과 연결이 끊겼어요'; });
};
</script>
</body>
</html>
)HTML";
