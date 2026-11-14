# AGENTS.md — Otto I2R Robot AI 코딩 안내

이 파일은 **AI 코딩 도구(Claude, ChatGPT, Gemini, Cursor 등)가 읽는 안내서**입니다.
Otto I2R 로봇의 아두이노 프로그램을 만들 때 여기 적힌 하드웨어 정보와 규칙을 그대로 따르세요.
사람을 위한 자세한 설명은 [docs/guide.md](docs/guide.md)에 있습니다.

---

## 0. 사용자와 대화 방식

- 사용자는 대부분 **프로그래밍 초보자**(학생, 일반인)입니다. **한국어**로, 쉬운 말로 설명합니다.
- 코드는 **처음부터 끝까지 전체**를 줍니다. "...생략..." 같은 부분 코드를 주지 않습니다.
- 한 번에 한 가지 동작만 만들고, 올려서 확인한 뒤 다음 기능을 덧붙입니다.
- 코드에는 초보자가 이해할 수 있는 한국어 주석을 붙입니다.
- 로봇이 이상하게 움직인다고 하면 **먼저 전원(배터리)과 보정**을 의심하고, 그다음에 코드를 고칩니다. (8장 참고)

## 1. 하드웨어

| 항목 | 값 |
|---|---|
| 보드 | **i2r-05** (ESP32-S3-WROOM-1-N16R8: 플래시 16MB, PSRAM 8MB, USB 내장) + 확장 보드 **i2r-shield-03** |
| USB | ESP32-S3 내장 USB (USB-Serial/JTAG). 별도 USB-UART 칩 없음 |
| 서보 전원 | 외부 배터리 권장. USB만 쓰면 서보가 동시에 움직일 때 전압이 떨어져 보드가 재부팅됨 |
| 회로도 | i2r-05: https://github.com/kdi6033/i2r-05 / i2r-shield-03: docs/guide.md 1.1 |

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

## 2. 개발 환경

- ESP32 보드 패키지: **esp32 by Espressif Systems 3.0 이상** (3.3.x에서 확인함)
- 보드 설정 (Arduino IDE 메뉴 / arduino-cli FQBN)

| Arduino IDE 메뉴 | 값 |
|---|---|
| 보드 | ESP32S3 Dev Module |
| USB CDC On Boot | **Enabled** (켜야 시리얼 모니터에 글자가 나옴) |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| Erase All Flash Before Sketch Upload | **Disabled** (꼭 끔. 켜면 보정값과 보안 정보가 지워짐) |

```
FQBN = esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB
```

- 필요한 라이브러리

| 라이브러리 | arduino-cli 설치 이름 | 언제 |
|---|---|---|
| OttoI2R | `OttoI2R` | 항상 (서보 동작) |
| FastLED | `FastLED` | LED 링 |
| U8g2 | `U8g2` | OLED |
| Adafruit VL53L0X | `Adafruit_VL53L0X` | 거리 센서 |
| BH1750 | `BH1750` | 조도 센서 |

- 시리얼 속도: **115200** (`ottoInit()`이 `Serial.begin(115200)`을 함께 부릅니다)

## 3. 꼭 지킬 규칙

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

## 4. OttoI2R 함수 목록 (라이브러리 1.1.0)

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

## 5. 프로그램 모양 (이 틀에서 시작)

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

## 6. 참고할 예제

새 프로그램은 가장 비슷한 예제를 먼저 읽고 그 방식대로 만듭니다.

**동작 예제** (OttoI2R 라이브러리 안)
- 설치된 위치: `문서/Arduino/libraries/OttoI2R/examples/` (Windows: `%USERPROFILE%\Documents\Arduino\libraries\OttoI2R\examples`)
- GitHub: https://github.com/kdi6033/OttoI2R/tree/main/examples
- walk_front, walk_front_speed, walk_back, walk_back_speed, walk_left, walk_right, slide_front, slide_back, slide_left, slide_right, rolling_front, rolling_front_speed, rolling_back, rolling_back_speed, rolling_left, rolling_right, hip_move, roll_one_foot, dance

**장치와 응용 예제** (이 저장소의 [examples/](examples/))

| 예제 | 내용 |
|---|---|
| servo_yl_pulse, servo_rr_pulse | PWM 펄스폭으로 서보를 직접 움직이는 원리 (교육용) |
| servo_yl_movehip, servo_rr_rollfoot | 같은 동작을 라이브러리 함수로 |
| dance_left_turn, dance_right_turn | 한 발로 서서 돌기 |
| otto_web_walk | 로봇에 웹서버를 만들어 휴대폰으로 걷기 속도 조절 (Wi-Fi) |
| oled_hello, oled_korean, oled_eyes, oled_sleepy_eyes | OLED 글자, 한글, 눈 그래픽 |
| led_basic, led_effects | LED 링 색과 효과 |
| distance_vl53l0x | 거리 센서 읽기 |
| face_sensor_led | OLED 눈 + 거리 센서 + LED를 함께 쓰기 |
| light_gy302 | 조도 센서 읽기 |

웹에서 읽을 때는 raw 주소를 씁니다. 예: `https://raw.githubusercontent.com/kdi6033/otto/main/examples/led_basic/led_basic.ino`

## 7. Claude Code / 터미널에서 직접 컴파일하고 올리기

AI 코딩 도구가 터미널을 쓸 수 있으면 아래 순서로 **직접 컴파일, 업로드, 결과 확인**까지 합니다.

**① 준비 확인 (처음 한 번)**

```bash
arduino-cli version
```

- 없으면 Arduino IDE에 들어 있는 것을 씁니다.
  Windows: `"/c/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"`
- 그것도 없으면 설치합니다. Arduino IDE는 없어도 됩니다.
  - Windows: `winget install --id ArduinoSA.CLI -e --source winget --accept-source-agreements --accept-package-agreements`
  - macOS: `brew install arduino-cli`
  - 설치 직후에는 지금 터미널의 PATH에 반영되지 않아 `arduino-cli`를 못 찾을 수 있습니다. 그때는 `%LOCALAPPDATA%\Microsoft\WinGet\Links\arduino-cli.exe` 또는 `%LOCALAPPDATA%\Microsoft\WinGet\Packages\ArduinoSA.CLI*\arduino-cli.exe`를 찾아 **전체 경로로** 실행합니다.
- 보드 패키지와 라이브러리 설치 (이미 있으면 건너뜀). esp32 보드 패키지는 내려받는 양이 많아 **5~15분** 걸릴 수 있으니 사용자에게 미리 알립니다.

```bash
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install OttoI2R FastLED U8g2 Adafruit_VL53L0X BH1750
```

**② 컴파일**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB my_sketches/my_robot
```

오류가 나면 오류 메시지를 읽고 고친 뒤 다시 컴파일합니다. 성공할 때까지 반복합니다.

**③ 포트 찾기**

```bash
arduino-cli board list
```

USB로 연결한 Otto는 보통 `ESP32 Family Device` 또는 VID `303A`로 보입니다. (Windows `COM3` 같은 이름, macOS `/dev/cu.usbmodem...`)
포트가 여러 개면 사용자에게 USB를 뺐다 꽂게 해서 새로 생긴 포트를 고릅니다.

**④ 업로드**

```bash
arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB my_sketches/my_robot
```

**⑤ 시리얼 출력 확인** (정해진 시간만 읽고 끝남)

- Windows: `powershell -ExecutionPolicy Bypass -File tools/serial-read.ps1 -Port COM5 -Seconds 10`
- macOS/Linux: `timeout 10 arduino-cli monitor -p /dev/cu.usbmodem1101 --config baudrate=115200`

**⑥ 사용자에게 확인 요청**: 로봇이 실제로 어떻게 움직였는지 물어보고, 그 말을 바탕으로 값을 고칩니다.
움직임은 AI가 볼 수 없으므로 "몇 걸음 갔는지, 어느 쪽으로 휘었는지"처럼 구체적으로 물어봅니다.

## 8. 문제 해결

| 증상 | 원인과 해결 |
|---|---|
| 걷다가 멈추고 보드가 다시 시작됨, 시리얼에 `Brownout` | **전원 부족**. 외부 배터리를 연결하고 배터리를 새것으로. 코드 문제가 아님 |
| 한쪽으로 휘며 걷거나 굴러감 | 보정 문제. 웹 [로봇 맞추기]에서 다시 맞추게 안내 (코드에 숫자로 넣지 않음) |
| 시리얼 모니터에 아무것도 안 나옴 | `USB CDC On Boot: Enabled`인지, 속도가 115200인지 확인 |
| 포트가 안 보임, 업로드 실패 | i2r-05는 버튼을 누르지 않아도 자동으로 업로드됩니다. BOOT/RESET 버튼을 누르라고 안내하지 않습니다. ① **데이터 전송이 되는** USB 케이블인지 확인 (충전 전용 케이블은 포트가 안 보임) ② USB를 뺐다 다시 꽂고 `arduino-cli board list`로 포트를 다시 찾기 ③ Arduino IDE 시리얼 모니터 등 다른 프로그램이 포트를 쓰고 있으면 닫기 ④ 다른 USB 포트나 케이블로 바꿔 보기 |
| `ESP32Servo` 관련 오류 | ESP32Servo를 쓰지 않습니다. OttoI2R 함수로 바꿈 |
| 컴파일 오류 `xxx.h: No such file` | 2장 표의 라이브러리를 설치 |
| OLED가 안 켜짐 | `Wire.begin(18, 17)`을 불렀는지, SH1106용 생성자를 썼는지 확인 |

## 9. 공식 펌웨어로 되돌리기

내 스케치를 올리면 공식 Otto 펌웨어(웹 조종, AI 대화, [로봇 맞추기])가 덮어써집니다.
다시 쓰려면 i2r.link에서 펌웨어를 다시 설치합니다. 이때 **보드의 저장값이 지워지므로** [로봇 맞추기]를 다시 하고 저장합니다.
보정은 공식 펌웨어에서만 할 수 있으므로, **내 스케치를 올리기 전에 먼저 [로봇 맞추기]를 끝내고 저장**하는 것이 좋습니다.

## 10. 문서

- 제작 가이드(사람용): [docs/guide.md](docs/guide.md)
- 통신 프로토콜(MQTT, BLE): [docs/protocol.md](docs/protocol.md)
- Claude Code로 시작하기: [docs/claude-code.md](docs/claude-code.md)
- 라이브러리 저장소: https://github.com/kdi6033/OttoI2R
