// secrets.example.h  ->  이 파일을 복사해서 스케치 폴더 안에 secrets.h 로 만들고 값을 넣으세요.
// secrets.h 는 .gitignore 에 들어 있어서 GitHub 에 올라가지 않습니다.
//
// 스케치에서 쓰는 법:
//   #include "secrets.h"
//   WiFi.begin(WIFI_SSID, WIFI_PASS);

#pragma once

#define WIFI_SSID "공유기이름"
#define WIFI_PASS "공유기비밀번호"
