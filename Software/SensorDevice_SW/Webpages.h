#pragma once

static const char* BUILD_TIMESTAMP = __DATE__ " " __TIME__;

// --- HTML Content Pages ---
static const char DASHBOARD_MAIN_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>ESP32 Dashboard</title>
<style>
body{font-family:Arial,sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100%%;background-color:#f0f2f5;margin:0;padding-top:1rem;padding-bottom:1rem;width:100%%;box-sizing:border-box;}
.container{margin:0.5rem;background-color:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center;width:90%%;max-width:420px;box-sizing:border-box;}
h1{color:#333;line-height:1.2;margin-top:0;margin-bottom:0.5rem;} h3{margin-top:0;}
.form-group{margin-bottom:1.5rem;text-align:left}
label{display:block;margin-bottom:.5rem;font-weight:700;color:#555}
input[type=text],input[type=password],select{width:100%%;padding:10px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}
button{width:100%%;padding:12px;color:#fff;border:none;border-radius:4px;font-size:1rem;cursor:pointer;background-color:#128037}
button:hover{background-color:#007025}
.status-box{text-align:left;padding:1.5rem;border:1px solid #e0e0e0;border-radius:8px;}
.status-item{display:flex;justify-content:space-between;align-items:center;margin-bottom:1rem}
.status-label{font-weight:700;color:#333}
.status-value{color:#555}
.status-connected{color:#28a745;font-weight:700}
.btn-danger{margin-top:1rem;background-color:#dc3545}
.btn-danger:hover{background-color:#c82333}
.btn-secondary{background-color:#6c757d} .btn-secondary:hover{background-color:#5a6268}
.btn-group form{margin-bottom:0.5rem;}
a{margin:0.2rem;width:24%%; text-decoration:none;}
.nav{margin-top:-1rem;margin-bottom:-1rem;display:flex;align-items:center;flex-direction:row;justify-content:center;}
.nav-button{background-color:#666666;width:100%%;padding:12px;color:#fff;border:none;border-radius:4px;font-size:1rem;cursor:pointer;}
.nav-button:hover{background-color:#333;}
.battery-display{font-size:0.9rem;color:#333;margin-top:-0.5rem;margin-bottom:0.5rem;}
</style>
</head><body><div class="container"><h1>%DASHBOARD_TITLE%</h1><br>
    <div class="nav"><a href="/"><button class="nav-button">연결</button></a><a href="/dashboard"><button class="nav-button">센서</button></a><a href="/camera"><button class="nav-button">카메라</button></a><a href="/debug"><button class="nav-button">디버그</button></a></div></div>
<div class="container">%PAGE_CONTENT%</div></body></html>
)rawliteral";

static const char DEVICE_STATUS_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box">
    <div class="status-item"><span class="status-label">연결 상태:</span><span class="status-value status-connected">연결됨</span></div>
    <div class="status-item"><span class="status-label">WiFi 이름 (SSID):</span><span class="status-value">%CURRENT_SSID%</span></div>
    <form action="/forget" method="post"><button type="submit" class="btn-danger">WiFi 정보 삭제</button></form>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:2rem 0;">
    <div class="status-item"><span class="status-label">현재 CCU 주소:</span><span class="status-value">%CURRENT_CCU_ADDRESS%</span></div>
    <form action="/save_ccu" method="post">
        <div class="form-group"><hr style="border:none;border-top:0px solid #e0e0e0;margin:1rem 0;">
            <input type="text" id="ccu_address" name="ccu_address" placeholder="예: ge-sd-xxxx.local">
        </div>
        <button type="submit">새 CCU 주소 저장</button>
    </form>
</div>
)rawliteral";

static const char SETUP_FORM_CONTENT[] PROGMEM = R"rawliteral(
<form action="/save" method="post">
    <div class="form-group"><label for="ssid">WiFi 이름 (SSID)</label><input type="text" id="ssid" name="ssid" required></div>
    <div class="form-group"><label for="password">비밀번호</label><input type="password" id="password" name="password"></div>
    <div class="form-group"><label for="ccu_address">CCU 주소 (선택 사항)</label><input type="text" id="ccu_address" name="ccu_address" placeholder="예: 192.168.0.100"></div>
    <button type="submit" style="margin-top:1rem;">저장 및 재부팅</button>
</form>
)rawliteral";

static const char DASHBOARD_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box">
    <h3>단말 정보</h3>
    <div class="status-item"><span class="status-label">배터리 잔량:</span><span class="status-value"><span id="bat_level">Loading...</span> %%</span></div>
    <div class="status-item"><span class="status-label">전원 모드:</span><span class="status-value" id="pwr_mode">Loading...</span></div>
    <div class="status-item"><span class="status-label">야간 모드:</span><span class="status-value" id="night_mode">Loading...</span></div>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
    <h3>환경 정보</h3>
    <div class="status-item"><span class="status-label">온도:</span><span class="status-value"><span id="amb_temp">Loading...</span> &deg;C</span></div>
    <div class="status-item"><span class="status-label">습도:</span><span class="status-value"><span id="amb_humi">Loading...</span> %%</span></div>
    <div class="status-item"><span class="status-label">광도:</span><span class="status-value"><span id="amb_light">Loading...</span> lux</span></div>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
    <h3>토양 정보</h3>
    <div class="status-item"><span class="status-label">온도:</span><span class="status-value"><span id="soil_temp">Loading...</span> &deg;C</span></div>
    <div class="status-item"><span class="status-label">수분:</span><span class="status-value"><span id="soil_humi">Loading...</span> %%</span></div>
    <div class="status-item"><span class="status-label">전도도:</span><span class="status-value"><span id="soil_ec">Loading...</span> uS/cm</span></div>
</div>
<script>
    function fetchSensorData() {
        fetch('/api/sensors')
            .then(response => response.json())
            .then(data => {
                document.getElementById('bat_level').textContent = data.bat_level;
                document.getElementById('pwr_mode').textContent = data.pwr_mode;
                document.getElementById('night_mode').textContent = data.night_mode;
                document.getElementById('amb_temp').textContent = data.amb_temp.toFixed(1);
                document.getElementById('amb_humi').textContent = data.amb_humi.toFixed(1);
                document.getElementById('amb_light').textContent = data.amb_light.toFixed(1);
                document.getElementById('soil_temp').textContent = data.soil_temp.toFixed(1);
                document.getElementById('soil_humi').textContent = data.soil_humi.toFixed(1);
                document.getElementById('soil_ec').textContent = data.soil_ec.toFixed(1);
            })
            .catch(error => console.error('Error fetching sensor data:', error));
    }
    window.onload = fetchSensorData;
</script>
)rawliteral";

static const char DEBUG_PAGE_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box">
    <h3>수동 제어</h3>
    <div class="btn-group">
        <form action="/send_sensor" method="post"><button type="submit">센싱 데이터 전송</button></form>
        <form action="/send_all" method="post"><button type="submit" class="btn-secondary">전체 데이터(센서+카메라) 전송</button></form>
    </div>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
    <h3>전원 모드 변경</h3>
    <form action="/set_power_mode" method="post">
        <div class="form-group">
            <select id="pwr_mode" name="pwr_mode">
                <option value="D">Debugging</option>
                <option value="U">Ultra High Freq</option>
                <option value="H">High Freq</option>
                <option value="M" selected>Normal</option>
                <option value="L">Low Power</option>
                <option value="Z">Ultra Low Power</option>
            </select>
        </div>
        <button type="submit">전원 모드 적용</button>
    </form>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
    <h3>야간 모드 변경</h3>
    <form action="/set_night_mode" method="post">
        <div class="form-group">
            <select id="nht_mode" name="nht_mode">
                <option value="1">활성화 (ON)</option>
                <option value="0">비활성화 (OFF)</option>
            </select>
        </div>
        <button type="submit">야간 모드 적용</button>
    </form>
</div>
)rawliteral";

static const char SUCCESS_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="10; url=/">
<title>설정 저장됨</title>
<style>
body{font-family:Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;background-color:#f0f2f5;margin:0}
.container{background-color:#fff;padding:3rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center}
.icon{width:50px;height:50px;margin-bottom:1rem;fill:#28a745}
h1{color:#28a745;margin-bottom:1rem;}
p{color:#555;font-size:1.1rem;}
</style></head>
<body><div class="container">
<svg class="icon" viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"></path></svg>
<h1>저장 완료!</h1><p>WiFi 정보가 저장되었습니다.<br>잠시 후(<span id="countdown">10</span>초) 메인 페이지로 이동합니다...</p>
</div>
<script>
  var countdownElement = document.getElementById('countdown');
  var seconds = 10;
  var interval = setInterval(function() {
    seconds--;
    countdownElement.textContent = seconds;
    if (seconds <= 0) {
      clearInterval(interval);
    }
  }, 1000);
</script>
</body></html>
)rawliteral";

static const char WEBSOCKET_CAMERA_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>ESP32 Dashboard</title>
<style>
body{font-family:Arial,sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100%%;background-color:#f0f2f5;margin:0;padding-top:1rem;padding-bottom:1rem;width:100%%;box-sizing:border-box;}
.container{margin:0.5rem;background-color:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center;width:90%%;max-width:420px;box-sizing:border-box;}
h1{color:#333;line-height:1.2;margin-top:0;margin-bottom:0.5rem;} h3{margin-top:0;}
.nav{margin-top:-1rem;margin-bottom:-1rem;display:flex;align-items:center;flex-direction:row;justify-content:center;}
a{margin:0.2rem;width:24%%; text-decoration:none;}
.nav-button{background-color:#666666;width:100%%;padding:12px;color:#fff;border:none;border-radius:4px;font-size:1rem;cursor:pointer;}
.nav-button:hover{background-color:#333;}
canvas { background-color: #000; border-radius: 8px; }
</style>
</head><body><div class="container"><h1>%DASHBOARD_TITLE%</h1><br>
<div class="nav"><a href="/"><button class="nav-button">연결</button></a><a href="/dashboard"><button class="nav-button">센서</button></a><a href="/camera"><button class="nav-button">카메라</button></a><a href="/debug"><button class="nav-button">디버그</button></a></div></div>
<div class="container">
<canvas id="stream-canvas" width="240" height="240"></canvas>
</div>
<script>
    const canvas = document.getElementById('stream-canvas');
    const ctx = canvas.getContext('2d');
    let ws;
    function connectWebSocket() {
        ws = new WebSocket(`ws://${window.location.hostname}/ws`);
        ws.binaryType = 'blob';
        ws.onopen = () => console.log('WebSocket open');
        ws.onclose = () => { console.log('WebSocket close, reconnecting...'); setTimeout(connectWebSocket, 1000); };
        ws.onmessage = (event) => {
            const image = new Image();
            image.src = URL.createObjectURL(event.data);
            image.onload = () => {
                ctx.drawImage(image, 0, 0, canvas.width, canvas.height);
                URL.revokeObjectURL(image.src);
            };
        };
    }
    window.onload = connectWebSocket;
</script>
</body></html>
)rawliteral";