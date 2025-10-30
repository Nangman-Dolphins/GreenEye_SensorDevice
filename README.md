# GreenEye 원예/화훼용 스마트 센서 단말기 (ESP32-CAM)

본 레포지토리는 ESP32-CAM을 기반으로 하는, 원예 및 화훼 환경 모니터링용 센서 단말기 펌웨어다. 이 장치는 화분이나 재배지의 환경 및 토양 데이터를 정밀하게 수집하고, 식물의 생장 상태 이미지를 촬영하여 MQTT 브로커를 통해 중앙 제어 장치(CCU)로 전송하도록 설계되었다. CCU로 전송된 데이터는 시계열 분석 및 인공지능(AI) 이미지 패턴 인식을 통해 식물의 현재 상태를 정밀하게 판단하고 관리하는 데 사용된다.

## 주요 기능

* **다중 센서 데이터 수집:**
    * **공중 환경:** 주변 온도, 습도 (AHT20)
    * **광량:** 작물 생장에 유효한 조도 (BH1750/GY-302)
    * **토양 상태:** 토양 온도, 토양 수분(함수율), 토양 전기 전도도(EC) (ADS1015 ADC 경유)
    * **장치 상태:** 배터리 잔량 (ADS1015 ADC 경유)
* **식물 생장 이미지 캡처:**
    * ESP32-CAM을 사용하여 설정된 주기(전원 모드별 상이)로 식물 이미지를 촬영하고, Base64로 인코딩하여 MQTT로 전송한다.
* **MQTT 양방향 통신:**
    * 수집된 모든 데이터를 JSON 형식으로 MQTT 브로커에 발행(publish)한다.
    * CCU로부터 설정 변경(전원 모드, 야간 모드 등) 및 실시간 데이터 요청 명령을 구독(subscribe)하여 원격으로 장치를 제어할 수 있다.
* **듀얼 모드 작동 (저전력/설정):**
    * **일반 모드 (Normal Mode):** 평상시 작동 모드. 설정된 주기에 따라 센서 측정, 데이터 전송 후 즉시 Deep Sleep 모드로 진입하여 배터리 소모를 최소화한다.
    * **설정 모드 (Setup Mode):** 장치 설치 시 사용하는 모드. WiFi AP로 동작하여 스마트폰 등으로 접속 가능한 웹 대시보드를 제공한다. WiFi 연결, CCU 주소 설정, 실시간 센서 값 확인, 카메라 스트리밍 등을 지원한다.
* **지능형 전원 관리:**
    * 6가지 상세 전원 모드를 지원하며, MQTT를 통해 원격으로 모드를 변경하여 데이터 수집 주기를 조절할 수 있다.
    * NTP 서버에서 시간을 동기화하여, 야간(21:00~06:00)에는 자동으로 측정을 중단하고 Deep Sleep을 유지하는 '야간 모드(Night Mode)'로 작동한다.

## 작동 모드 상세

이 펌웨어는 두 가지 명확히 구분된 모드로 작동한다.

### 1. 일반 모드 (Normal Mode)

* **진입:** 기본 작동 모드. Deep Sleep 상태에서 타이머에 의해 깨어나거나, 설정 모드에서 재부팅될 때 진입한다.
* **동작:**
    1.  Deep Sleep에서 깨어난다 (`bootCount` 1 증가).
    2.  `NetworkManager`가 `Preferences`에 저장된 WiFi 정보로 STA 모드 연결을 시도한다. (실패 시 설정 모드로 전환하기 위해 재부팅)
    3.  WiFi 연결 성공 시, `TimeManager`가 NTP 서버(pool.ntp.org, KST UTC+9)와 시간을 동기화한다.
    4.  `TimeManager`가 `isNightTime()` (21:00~06:00)을 확인한다.
        * **야간 모드일 경우:** `MQTT.loop()`만 짧게 실행하여 CCU의 명령(예: 모드 변경)을 수신한 뒤, `getSecondsUntil6AM()`으로 계산된 시간만큼 즉시 다시 Deep Sleep에 진입한다. (센서/카메라 작동 안 함)
        * **주간 모드일 경우:** 다음 5~9단계를 순차적으로 수행한다.
    5.  `MQTTClient`가 `Preferences`에 저장된 CCU 주소로 MQTT 브로커에 연결한다. (실패 시 설정 모드로 전환하기 위해 재부팅)
    6.  `sensors.readAllSensors()`를 호출하여 모든 센서 값을 읽어온다.
    7.  수집된 센서 데이터를 JSON으로 포맷하여 `GreenEye/data/[DeviceID]` 토픽으로 MQTT 발행한다.
    8.  `powerManager.shouldSendCameraData(bootCount)`를 호출, 현재 부팅 횟수가 카메라 전송 주기에 해당하는지 확인한다.
    9.  전송 주기가 맞다면, `camera.captureFrameForAnalyze()` (고화질)를 호출, 이미지를 Base64로 인코딩하여 `plant_img` 키와 함께 MQTT로 발행한다.
    10. `sensors.endI2C()`로 I2C 버스를 비활성화하고, `powerManager.enterDeepSleep()`을 호출하여 현재 전원 모드에 설정된 `_senseInterval` 시간만큼 Deep Sleep에 진입한다.

### 2. 설정 모드 (Setup Mode)

* **진입:**
    1.  장치가 Deep Sleep 상태(또는 전원이 꺼진 상태)일 때 `SETUP_BUTTON_PIN` (IO2) 버튼을 누른 상태로 전원을 켜거나 Deep Sleep에서 깨운다.
    2.  장치가 `ESP_SLEEP_WAKEUP_EXT0` (외부 핀) 인터럽트로 즉시 깨어난다.
    3.  `handleWakeupReason()` 함수가 이 웨이크업 사유를 감지하고, 비휘발성 메모리(Preferences)에 `setup_mode` 플래그를 `true`로 저장한 뒤, 장치를 **재부팅(ESP.restart())**한다.
    4.  재부팅된 장치의 `setup()` 함수가 `setup_mode` 플래그를 읽고 `isSetupMode = true`로 설정하여 설정 모드로 진입한다.
* **동작:**
    1.  `NetworkManager`가 `WIFI_AP_STA` 듀얼 모드로 작동한다.
        * **AP 모드:** `ge-sd-xxxx` (xxxx는 MAC 주소 하위 2바이트)라는 이름의 WiFi AP를 생성한다.
        * **STA 모드:** 동시에 `Preferences`에 저장된 기존 WiFi로 연결을 시도한다.
    2.  mDNS 서비스(`http://ge-sd-xxxx.local`)를 시작하여, AP에 연결된 기기가 이 주소로 쉽게 접속할 수 있게 한다.
    3.  `ESPAsyncWebServer`를 실행하여 `Webpages.h`에 정의된 웹 대시보드(HTML/CSS/JS)를 서비스한다.
    4.  **RTOS 태스크**를 생성하여 웹 서버와 다른 기능들이 병렬로 작동하게 한다. (Deep Sleep에 진입하지 않는다)
        * `backgroundTask` (Core 0): `MQTT.loop()`를 주기적으로 실행하고(WiFi STA가 연결된 경우), IO2 버튼을 3초 이상 누르면 설정 모드를 종료(플래그 삭제 및 재부팅)하는지 감지한다.
        * `cameraStreamTask` (Core 0): 웹 대시보드의 '카메라' 탭에서 WebSocket (`/ws`) 클라이언트가 연결되면, `camera.captureFrameForStream()` (저화질)을 통해 실시간 영상 스트림을 전송한다. 클라이언트가 없으면 `vTaskSuspend`로 작업을 중지시켜 자원을 아낀다.
* **종료:**
    1.  **(수동 종료)** IO2 버튼을 3초 이상 길게 누르면 `backgroundTask`가 이를 감지하여 `setup_mode` 플래그를 `false`로 바꾸고 재부팅한다.
    2.  **(자동 종료)** 웹 대시보드에서 WiFi 정보 또는 CCU 정보를 '저장'하거나 '삭제'하면, 해당 정보가 Preferences에 저장된 후 장치가 자동으로 재부팅된다. 재부팅 시 `setup_mode` 플래그가 없으므로 일반 모드로 시작한다.

## 레포지토리 구성 (파일 상세)

### 1. Arduino 펌웨어 (`.ino`, `.h`)

* **`SensorDevice_SW.ino`**:
    * **역할:** 메인 펌웨어 파일. 모든 클래스를 초기화하고 모드(일반/설정)에 따라 전체 동작 흐름을 제어한다.
    * **상세:**
        * `setup()`: `setup_mode` 플래그를 확인하여 **일반 모드** 또는 **설정 모드** 중 하나로 진입을 결정한다.
        * `loop()`: **일반 모드**에서는 센서 읽기, MQTT 전송, Deep Sleep의 순차적 로직을 수행한다. **설정 모드**에서는 아무 작업도 하지 않으며(모든 작업이 RTOS 태스크로 처리됨), `vTaskDelay`로 CPU를 양보한다.
        * **설정 모드**의 모든 웹 서버 라우팅(`server.on(...)`)과 WebSocket 이벤트 핸들러(`onWsEvent`)를 정의한다.
        * MQTT 콜백 함수(`handleDataRequest`, `handleConfig`)를 정의하여 CCU로부터의 원격 명령을 처리한다.

* **`Camera.h`**:
    * **역할:** ESP32-CAM (AI-Thinker 모델 기준)의 핀맵을 정의하고 카메라 센서(OV2640)를 초기화한다.
    * **상세:**
        * `captureFrameForStream()`: 웹소켓을 통한 실시간 스트리밍용. 저화질/고속(240x240, JPEG Quality 15) 프레임을 캡처한다.
        * `captureFrameForAnalyze()`: MQTT를 통해 CCU로 전송할 분석용. 고화질(240x240, JPEG Quality 3) 프레임을 캡처한다. (해상도는 동일하나 압축률이 낮아 화질이 좋다)

* **`SensorIO_withExtLibs.h`**: (현재 사용 중인 버전)
    * **역할:** 센서 관리를 위한 **외부 라이브B러리 기반** 클래스다.
    * **상세:** `Adafruit_ADS1X15`, `Adafruit_AHTX0`, `BH1750` 라이브러리를 사용한다. `begin()` 함수에서 각 라이브러리의 `begin()`을 호출하여 센서를 초기화하며, `read...()` 함수들이 라이브러리가 제공하는 API(예: `aht.getEvent()`, `lightMeter.readLightLevel()`)를 사용하여 값을 읽어온다. 개발 및 유지보수가 용이하다.

* **`SensorsIO.h`**: (대체/백업 버전)
    * **역할:** `SensorIO_withExtLibs.h`와 동일한 목적을 가지나, **외부 라이브러리 없이** `Wire.h`만을 사용하여 I2C 통신을 **직접 구현한** 클래스다.
    * **상세:** `readADS1015()`, `readAmbientTempHumi()` 등의 함수가 센서 데이터시트의 I2C 프로토콜(레지스터 주소, 설정값, 응답 바이트 파싱)을 직접 `Wire.write()`와 `Wire.requestFrom()`으로 구현한다. 라이브러리 의존성을 없애거나 메모리를 최적화할 때 사용될 수 있다.

* **`PowerManager.h`**:
    * **역할:** 장치의 전원 정책과 Deep Sleep 주기를 총괄한다.
    * **상세:**
        * `PowerMode` 열거형(enum)을 통해 6가지 전원 모드를 정의한다:
            * `ULTRA_LOW_POWER` (Z): 센서 2시간, 카메라 4시간
            * `LOW_POWER` (L): 센서 1시간, 카메라 2시간
            * `NORMAL` (M): 센서 30분, 카메라 1시간 (기본값)
            * `HIGH_FREQ` (H): 센서 20분, 카메라 1시간
            * `ULTRA_HIGH_FREQ` (U): 센서 20분, 카메라 40분
            * `DEBUGGING` (D): 센서 30초, 카메라 60초 (Deep Sleep 최소화, 테스트용)
        * `setMode(char modeChar)`: 외부(MQTT 또는 웹)에서 'Z', 'L' 등의 문자로 모드를 입력받아 `Preferences`에 저장한다.
        * `shouldSendCameraData(int bootCount)`: 현재 `bootCount`와 설정된 간격(`_senseInterval`, `_camInterval`)을 나누어, 나머지가 0일 때(즉, 카메라 주기가 도래했을 때) `true`를 반환한다.
        * `enterDeepSleep(unsigned long sleep_seconds)`: Deep Sleep 진입 전 `Wire.end()`로 I2C 버스를 종료하고, 타이머(시간) 및 `GPIO_NUM_2`(버튼)를 Wakeup 소스로 설정한 뒤 `esp_deep_sleep_start()`를 호출한다.

* **`MQTT.h`**:
    * **역할:** `PubSubClient` 라이브러리를 래핑하여 MQTT 통신을 안정적으로 관리한다.
    * **상세:**
        * 장치 MAC 주소 하위 2바이트로 고유 ID (예: `A1B2`)를 생성한다.
        * 3가지 토픽을 사용한다:
            * 발행(Pub): `GreenEye/data/[DeviceID]` (센서/이미지 데이터)
            * 구독(Sub): `GreenEye/req/[DeviceID]` (CCU의 데이터 즉시 전송 요청)
            * 구독(Sub): `GreenEye/conf/[DeviceID]` (CCU의 설정 변경 명령)
        * `reconnect()`: MQTT 브로커 연결을 시도하고, 실패 시 5회 재시도한다.
        * `onDataRequest`, `onConfig`: `SensorDevice_SW.ino`에 정의된 콜백 함수를 연결하여, 구독한 토픽에 메시지가 수신되면 해당 함수를 실행시킨다.

* **`NetworkManager.h`**:
    * **역할:** WiFi 연결(STA) 및 AP 모드를 관리한다.
    * **상세:** `begin()` 함수 하나로 `WIFI_AP_STA` 듀얼 모드를 시작한다. AP(`ge-sd-xxxx`)를 활성화하는 동시에 `Preferences`에 저장된 SSID로 STA 연결을 시도한다. 또한 `MDNS.begin()`을 호출하여 `http://ge-sd-xxxx.local` 주소로 웹 대시보드에 접속할 수 있도록 mDNS 서비스를 시작한다.

* **`TimeManager.h`**:
    * **역할:** NTP(Network Time Protocol)를 이용한 시간 동기화 및 야간/주간 판단을 담당한다.
    * **상세:**
        * `NTPClient`를 KST (UTC+9)로 초기화한다.
        * `isNightTime()`: 현재 시간이 21시(오후 9시)에서 6시(오전 6시) 사이인지 확인하여 야간 절전 모드 여부를 반환한다.
        * `getSecondsUntil6AM()`: 현재 시간이 야간일 때, 정확히 오전 6시(60초 버퍼 포함)에 깨어날 수 있도록 Deep Sleep에 필요한 총 시간(초)을 계산한다.

* **`Webpages.h`**:
    * **역할:** **설정 모드**에서 사용되는 모든 사용자 인터페이스(UI)의 HTML, CSS, JavaScript 코드를 `PROGMEM`을 통해 C++ 문자열 상수로 저장한다. (플래시 메모리에 저장하여 RAM을 절약)
    * **상세:**
        * `DASHBOARD_MAIN_TEMPLATE`: 모든 페이지의 기본 레이아웃 (헤더, 네비게이션 탭).
        * `SETUP_FORM_CONTENT` (루트 `/`, WiFi 미연결 시): WiFi SSID, PW, CCU 주소를 입력받는 폼. `/save`로 POST.
        * `DEVICE_STATUS_CONTENT` (루트 `/`, WiFi 연결 시): 현재 연결된 SSID와 CCU 주소를 보여주고 'WiFi 정보 삭제' 기능을 제공. `/forget`으로 POST.
        * `DASHBOARD_CONTENT` (`/dashboard`): `/api/sensors` 엔드포인트에 주기적으로 `fetch` 요청을 보내 실시간 센서 데이터를 JSON으로 받아와 표시한다.
        * `DEBUG_PAGE_CONTENT` (`/debug`): 수동 데이터 전송(`/send_sensor`), 전원 모드 변경(`/set_power_mode`), 야간 모드 활성화/비활성화(`/set_night_mode`)를 위한 폼을 제공한다.
        * `WEBSOCKET_CAMERA_PAGE_HTML` (`/camera`): 페이지 로드 시 `/ws` 주소로 WebSocket에 연결하고, 수신되는 바이너리(Blob) 이미지 데이터를 실시간으로 `<canvas>`에 그린다.