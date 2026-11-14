---
name: otto-robot
description: Otto I2R 로봇(ESP32-S3 i2r-05 보드, 서보 YL/YR/RL/RR, OttoI2R 아두이노 라이브러리)의 아두이노 프로그램을 만들거나 고칠 때 사용한다. 사용자가 Otto, 오토 로봇, OttoI2R, walkFront, i2r-05, 걷는 로봇 코드를 말하면 이 스킬을 쓴다.
---

# Otto I2R Robot 프로그래밍

1. 가능하면 먼저 최신 안내서를 읽는다: https://raw.githubusercontent.com/kdi6033/otto/main/AGENTS.md
   (읽을 수 없으면 아래 요약을 따른다)
2. 사용자는 초보자다. 한국어로 쉽게 설명하고, 코드는 처음부터 끝까지 전체를 준다. 한 번에 한 기능씩.
3. 로봇이 걷다가 멈추고 재부팅되면 코드보다 배터리(전원)를 먼저 의심한다. 한쪽으로 휘면 웹 [로봇 맞추기] 보정을 안내한다.

## 하드웨어
**서보 4개** (좌/우는 *로봇이 바라보는 방향* 기준)

| 이름 | 위치 | 핀 | 종류 | 하는 일 |
|---|---|---|---|---|
| YL | 왼쪽 엉덩이 | GPIO 4 | 180도 각도 서보 | 다리를 기울여 무게중심 이동 |
| YR | 오른쪽 엉덩이 | GPIO 5 | 180도 각도 서보 | 다리를 기울여 무게중심 이동 |
| RL | 왼발 | GPIO 6 | 360도 연속회전 서보 | 발을 굴려 앞뒤로 이동 |
| RR | 오른발 | GPIO 7 | 360도 연속회전 서보 | 발을 굴려 앞뒤로 이동 |

**그 밖의 장치**

| 장치 | 연결 | 라이브러리 / 설정 |
|---|---|---|
| LED 링 (WS2812B 16개) | **GPIO 1** | FastLED, `WS2812B`, `GRB`, `NUM_LEDS 16` |
| OLED 1.3" 128x64 (SH1106) | I2C **SDA 18, SCL 17** | U8g2, `U8G2_SH1106_128X64_NONAME_F_HW_I2C` |
| 거리 센서 VL53L0X | I2C (주소 0x29) | Adafruit_VL53L0X |
| 조도 센서 GY-302 (BH1750) | I2C | BH1750 |
| 스피커 앰프 MAX98357A (I2S 출력) | **BCLK 12, LRCLK 11, DIN 13, SD_MODE 14** | GPIO 14를 HIGH로 하면 앰프가 켜지고 LOW면 꺼짐. 라이브러리 ESP32-audioI2S 또는 `driver/i2s_std.h` |
| 마이크 INMP441 (I2S 입력) | **SCK 8, WS 10, SD 9** | `driver/i2s_std.h`, 16000Hz 권장. 녹음 버퍼가 크면 PSRAM(OPI) 사용 |
| 배터리 전압 | **GPIO 15** (ADC) | VIN → 47kΩ → GPIO15 → 10kΩ → GND. 배터리 전압 ≈ `analogReadMilliVolts(15) * 5.7 / 1000` V (ADC2 핀이라 Wi-Fi 사용 중에는 읽기가 실패할 수 있음) |
| 스위치 SW3 | **GPIO 16** (누르면 LOW, 외부 풀업) | 공식 펌웨어에서는 Wi-Fi/공장 초기화 버튼. 내 스케치에서는 자유롭게 사용 |
| 확장 포트 J9 (3핀: 신호/5V/GND) | GPIO 2 | 서보나 LED를 하나 더 연결할 때 |
| 확장 포트 J14 (4핀: GPIO47, GPIO38, 5V, GND) | GPIO 47, 38 | 추가 센서용 |
| 업로드 | USB만 연결하면 **자동으로 업로드 모드**로 들어감 | BOOT/RESET 버튼을 누를 필요 없음 (보드 회로로 자동 처리) |

- I2C 장치는 모두 같은 버스를 씁니다. `Wire.begin(18, 17);`을 **한 번만** 부르세요.
- LED 밝기는 `FastLED.setBrightness(50);` 정도로 낮게 시작하세요. 밝으면 전류가 많아 서보와 함께 쓸 때 재부팅될 수 있습니다.

## 보드 설정
| Arduino IDE 메뉴 | 값 |
|---|---|
| 보드 | ESP32S3 Dev Module |
| USB CDC On Boot | **Enabled** (켜야 시리얼 모니터에 글자가 나옴) |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| Erase All Flash Before Sketch Upload | **Disabled** (꼭 끔. 켜면 보정값과 보안 정보가 지워짐) |

## 꼭 지킬 규칙
1. 서보를 쓰는 스케치는 `#include <OttoI2R.h>`를 넣고, **`setup()`의 첫 줄에서 `ottoInit();`** 을 부릅니다.
   - `ottoInit()`이 보드에 저장된 보정값(웹 [로봇 맞추기]에서 저장한 값)을 불러옵니다.
   - `ottoInit()` 없이 `walkFront()` 등을 바로 부르면 저장값이 아니라 **기본값**으로 움직여 로봇이 잘 못 걷습니다.
2. **보정값을 코드에 숫자로 적지 않습니다.** `ottoSetCalibration`, `ottoSaveCalibration`, `ottoClearCalibration`, `setDanceTilts`, `clearDanceTilts`는 사용자가 분명히 원할 때만 씁니다. (보드에 저장된 값을 덮어쓰거나 지웁니다)
3. 서보 핀(4~7)에 `ledcAttach`, `Servo.attach`를 직접 쓰지 않습니다. 동작은 OttoI2R 함수로 만듭니다.
   (`examples/servo_*_pulse`는 PWM 원리를 배우는 교육용 예제라서 예외입니다)
4. **ESP32Servo 라이브러리를 쓰지 않습니다.** OttoI2R이 ESP32 기본 PWM(LEDC)으로 직접 구동하므로 함께 쓰면 충돌합니다.
5. 엉덩이 각도의 안전 범위는 **YR 0~135도, YL 55~180도**이고 90도가 가운데입니다. 범위를 벗어나면 라이브러리가 자동으로 맞춥니다.
6. 발 속도는 **-400 ~ 400**입니다. 양수는 앞, 음수는 뒤, 0은 멈춤입니다. 걷기·굴리기 속도(velocity)는 **20~200**이고 100이 기본입니다.
7. `rollingFront/Back/Right/Left`는 **발을 멈추지 않고 계속 돌립니다.** 멈추려면 `ottoRollFeet(0, 0);`을 부릅니다.
8. **NVS(보드 저장 공간)를 지우거나 다른 프로그램의 값을 건드리지 않습니다.**
   - `nvs_flash_erase()`, `Preferences::clear()`를 아래 이름에 쓰지 않습니다.
   - 공식 펌웨어가 쓰는 이름: `cfg`, `netcfg`, `certs`(보안 인증서), `ottosched` / 라이브러리가 쓰는 이름: `ottocal`, `ottodance`
   - 내 값을 저장하려면 `my`로 시작하는 새 이름을 씁니다. 예: `prefs.begin("myrobot", false);`
9. **Wi-Fi 비밀번호와 API 키는 `secrets.h`에 따로 두고** 스케치에서 `#include "secrets.h"`로 불러옵니다. `secrets.h`는 `.gitignore`에 들어 있어 GitHub에 올라가지 않습니다. 비밀 값을 코드에 직접 쓰거나 시리얼로 출력하지 않습니다.
10. 라이브러리 파일(OttoI2R.cpp 등)은 고치지 않습니다. 새 프로그램은 `my_sketches/스케치이름/스케치이름.ino`로 만듭니다. (폴더 이름과 .ino 파일 이름이 같아야 합니다)

## OttoI2R 함수 목록
**시작과 보정**

| 함수 | 설명 |
|---|---|
| `ottoInit()` | 저장된 보정값을 불러와 서보를 준비하고 `Serial.begin(115200)`도 합니다. `setup()` 첫 줄에 씁니다 |
| `ottoGetCalibration(OttoCalibration &c)` | 지금 쓰는 보정값을 읽습니다 (`ylCenter, yrCenter, tiltR, tiltL, rlSpeed, rrSpeed`) |
| `ottoHasSavedCalibration()` | 보드에 저장된 보정값이 있으면 `true` |

**걷기** (한 번 부르면 한 걸음 주기(오른발+왼발, 속도 100에서 약 1초)가 끝날 때까지 기다렸다가 돌아옵니다. `loop()`에서 반복해서 부릅니다)

| 함수 | 설명 |
|---|---|
| `walkFront()` / `walkFront(v)` | 앞으로 걷기. v = 20~200, 100이 기본 |
| `walkBack()` / `walkBack(v)` | 뒤로 걷기 |
| `walkLeft()` | 걸으면서 왼쪽으로 돌기 |
| `walkRight()` | 걸으면서 오른쪽으로 돌기 |

**미끄러지기** (한 주기씩, `loop()`에서 반복)

| 함수 | 설명 |
|---|---|
| `slideFront()` / `slideBack()` | 스케이트처럼 좌우 교대로 앞 / 뒤로 |
| `slideRight()` / `slideLeft()` | 스케이트처럼 오른쪽 / 왼쪽으로 돌기 |

**굴리기** (엉덩이를 굴리기 자세(YR 0도, YL 180도)로 바꾼 뒤 발을 계속 돌리고 **바로 돌아옵니다**)

| 함수 | 설명 |
|---|---|
| `rollingFront()` / `rollingFront(v)` | 앞으로 계속 굴러가기. v = 20~200 |
| `rollingBack()` / `rollingBack(v)` | 뒤로 계속 굴러가기 |
| `rollingRight()` / `rollingLeft()` | 제자리에서 오른쪽 / 왼쪽으로 계속 돌기 |

**관절 직접 움직이기**

| 함수 | 설명 |
|---|---|
| `moveHip(yr, yl)` | 엉덩이 각도. 90이 가운데, YR 0~135, YL 55~180 |
| `ottoRollFeet(rr, rl)` | 두 발 속도를 함께 정합니다. -400~400, 0은 멈춤 |
| `rollRightFoot(speed)` / `rollLeftFoot(speed)` | 한쪽 발만 굴립니다. -400~400 |
| `writeYL(angle)` / `writeYR(angle)` | 엉덩이 서보 하나를 보정된 각도로 움직입니다 |

**댄스와 시험**

| 함수 | 설명 |
|---|---|
| `dance()` | 한 발로 서서 반대쪽 발을 굴려 좌회전·우회전 (한 번에 약 8초). 넘어지기 쉬우니 손으로 받치고 시험 |
| `ottoCalibrationWalk()` / `ottoCalibrationRolling()` | 보정 확인용 걷기 / 굴리기 시험 동작 |

**보정값 바꾸기** (사용자가 원할 때만. 3장 규칙 2)

`ottoSetCalibration(yl, yr, tiltR, tiltL, rlSpeed, rrSpeed)`, `ottoSetHipCenter(yl, yr)`, `ottoSetTilt(tiltR, tiltL)`, `ottoSetSpeed(rl, rr)`, `ottoSaveCalibration()`, `ottoLoadCalibration()`, `ottoClearCalibration()`, `setDanceTilts(rYr, rYl, lYr, lYl)`, `applyDanceTilts(...)`, `getDanceTilts(...)`, `saveDanceTilts()`, `clearDanceTilts()`

## 프로그램 모양

```cpp
#include <OttoI2R.h>

void setup() {
  ottoInit();                 // 꼭 첫 줄: 저장된 보정값 불러오기 + Serial.begin(115200)
  Serial.println("시작!");
}

void loop() {
  walkFront(100);             // 한 걸음 주기(약 1초) 걷기
}
```

**센서를 보면서 움직이기**: 걷기 함수는 한 걸음 동안 기다리므로, 걸음과 걸음 사이에 센서를 읽습니다.

```cpp
void loop() {
  int mm = readDistanceMm();  // 거리 센서 읽기 (직접 만드는 함수)
  if (mm > 0 && mm < 150) {
    walkBack(100);            // 가까우면 뒤로 한 걸음
  } else {
    walkFront(100);           // 아니면 앞으로 한 걸음
  }
}
```

**굴리다가 멈추기**

```cpp
rollingFront(80);             // 발이 계속 돈다
delay(2000);                  // 2초 동안 굴러간다
ottoRollFeet(0, 0);           // 멈춘다
moveHip(90, 90);              // 서 있는 자세로 돌아온다
```

## 예제

- 동작: https://github.com/kdi6033/OttoI2R/tree/main/examples (walk_front, rolling_front, hip_move, dance 등)
- 장치(OLED, LED, 센서, 웹서버): https://github.com/kdi6033/otto/tree/main/examples
- raw 주소 예: https://raw.githubusercontent.com/kdi6033/otto/main/examples/led_basic/led_basic.ino
