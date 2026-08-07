#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>

// ---------- User Settings ----------
#define LED_PIN     2          // GPIO2 = D4 on D1 Mini
#define NUM_LEDS    48         // Default strip length (adjustable from UI)
#define AP_SSID     "ARAsHHH"
#define AP_PASSWORD ""         // Open network

// ---------- Global Objects ----------
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);

// ---------- Effect State ----------
enum Effect { STATIC, RAINBOW, CHASE, BREATHE, PALETTE_SWEEP };
Effect currentEffect = STATIC;
uint32_t staticColor = strip.Color(255, 0, 0);  // Default RED
uint8_t brightness = 100;
uint8_t speed = 50;
unsigned long lastUpdate = 0;
uint8_t rainbowHue = 0;
int chasePos = 0;
float breathePhase = 0;
uint8_t paletteIndex = 0;
int ledCount = NUM_LEDS;

// ---------- HTML Page (unchanged beautiful UI) ----------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>ARAsHHH</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 100%);
      font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      color: #fff;
    }
    .container {
      width: 100%;
      max-width: 420px;
      padding: 20px;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }
    h1 {
      text-align: center;
      font-weight: 300;
      font-size: 2.2rem;
      letter-spacing: 2px;
      background: linear-gradient(90deg, #ff00cc, #3333ff);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      margin-bottom: 5px;
    }
    .card {
      background: rgba(255,255,255,0.06);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border-radius: 24px;
      padding: 20px;
      border: 1px solid rgba(255,255,255,0.1);
      box-shadow: 0 20px 40px rgba(0,0,0,0.3);
    }
    #stripPreview {
      width: 100%;
      height: 45px;
      background: rgba(0,0,0,0.4);
      border-radius: 12px;
      display: block;
    }
    .color-section {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
    }
    #colorPicker {
      -webkit-appearance: none;
      width: 70px;
      height: 70px;
      border-radius: 50%;
      border: 3px solid rgba(255,255,255,0.3);
      background: transparent;
      cursor: pointer;
      transition: transform 0.2s;
    }
    #colorPicker:hover { transform: scale(1.05); }
    #colorPicker::-webkit-color-swatch-wrapper { padding: 0; }
    #colorPicker::-webkit-color-swatch { border: none; border-radius: 50%; }
    .effect-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      margin-top: 5px;
    }
    button {
      background: rgba(255,255,255,0.08);
      border: 1px solid rgba(255,255,255,0.15);
      color: #ccc;
      padding: 12px 8px;
      border-radius: 14px;
      font-size: 0.9rem;
      font-weight: 500;
      cursor: pointer;
      transition: all 0.2s;
      backdrop-filter: blur(8px);
    }
    button.active {
      background: linear-gradient(135deg, #ff00cc, #3333ff);
      border-color: transparent;
      color: white;
      box-shadow: 0 4px 15px rgba(255,0,204,0.4);
    }
    .slider-group {
      display: flex;
      flex-direction: column;
      gap: 4px;
      margin: 5px 0;
    }
    .slider-header {
      display: flex;
      justify-content: space-between;
      font-size: 0.9rem;
      color: #aaa;
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      height: 8px;
      border-radius: 8px;
      background: rgba(255,255,255,0.15);
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: white;
      box-shadow: 0 0 15px #ff00cc;
      cursor: pointer;
    }
    .value-badge {
      background: rgba(255,255,255,0.1);
      padding: 2px 10px;
      border-radius: 20px;
      font-size: 0.8rem;
    }
    .rgb-sliders {
      display: flex;
      gap: 8px;
      align-items: center;
      margin-top: 8px;
    }
    .rgb-sliders input {
      flex: 1;
    }
    .rgb-label {
      width: 20px;
      font-size: 0.8rem;
      color: #aaa;
      text-align: center;
    }
    footer {
      text-align: center;
      font-size: 0.7rem;
      color: #555;
      margin-top: 5px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>AAAsHHH</h1>

    <!-- LED Count slider (above preview) -->
    <div class="card" style="padding:15px;">
      <div class="slider-group">
        <div class="slider-header">
          <span>LED Count</span>
          <span class="value-badge" id="ledCountVal">60</span>
        </div>
        <input type="range" min="1" max="300" value="60" id="ledCountSlider" oninput="setLedCountDebounced(this.value)">
      </div>
    </div>

    <!-- Strip preview -->
    <div class="card" style="padding:10px;">
      <canvas id="stripPreview" width="400" height="45"></canvas>
    </div>

    <!-- Color picker + RGB sliders -->
    <div class="card color-section">
      <label style="font-size:0.9rem; color:#aaa;">Pick a Color</label>
      <input type="color" id="colorPicker" value="#ff0000" onchange="setColorFromPicker(this.value)">
      <div class="rgb-sliders">
        <span class="rgb-label">R</span>
        <input type="range" min="0" max="255" value="255" id="rSlider" oninput="setColorFromSliders()">
        <span class="rgb-label">G</span>
        <input type="range" min="0" max="255" value="0" id="gSlider" oninput="setColorFromSliders()">
        <span class="rgb-label">B</span>
        <input type="range" min="0" max="255" value="0" id="bSlider" oninput="setColorFromSliders()">
      </div>
    </div>

    <!-- Effects -->
    <div class="card">
      <div style="font-size:0.9rem; color:#aaa; margin-bottom:8px;">Effect</div>
      <div class="effect-grid">
        <button onclick="setEffect('static')" id="btn-static">Static</button>
        <button onclick="setEffect('rainbow')" id="btn-rainbow">Rainbow</button>
        <button onclick="setEffect('chase')" id="btn-chase">Chase</button>
        <button onclick="setEffect('breathe')" id="btn-breathe">Breathe</button>
        <button onclick="setEffect('palette')" id="btn-palette">Palette</button>
      </div>
    </div>

    <!-- Brightness & Speed -->
    <div class="card">
      <div class="slider-group">
        <div class="slider-header">
          <span>Brightness</span>
          <span class="value-badge" id="brightVal">100</span>
        </div>
        <input type="range" min="0" max="255" value="100" id="brightSlider" oninput="setBrightnessDebounced(this.value)">
      </div>
      <div class="slider-group" style="margin-top:12px;">
        <div class="slider-header">
          <span>Speed</span>
          <span class="value-badge" id="speedVal">50</span>
        </div>
        <input type="range" min="0" max="100" value="50" id="speedSlider" oninput="setSpeedDebounced(this.value)">
      </div>
    </div>
    <footer>AAAsHHH WS28XX Controller</footer>
  </div>

  <script>
    // ----- State synced with ESP -----
    let ledCount = 60;
    let currentColor = '#ff0000';
    let currentEffect = 'static';
    let brightness = 100;
    let speed = 50;

    // Animation state (mirror ESP)
    let rainbowHue = 0;
    let chasePos = 0;
    let breathePhase = 0;
    let paletteIndex = 0;

    let lastTimestamp = 0;
    let accumulator = 0;

    const canvas = document.getElementById('stripPreview');
    const ctx = canvas.getContext('2d');

    function debounce(func, delay) {
      let timer;
      return function(...args) {
        clearTimeout(timer);
        timer = setTimeout(() => func.apply(this, args), delay);
      };
    }

    function mapValue(x, in_min, in_max, out_min, out_max) {
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    function hexToRgb(hex) {
      return {
        r: parseInt(hex.substr(1,2), 16),
        g: parseInt(hex.substr(3,2), 16),
        b: parseInt(hex.substr(5,2), 16)
      };
    }

    function rgbToHex(r, g, b) {
      return '#' + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    }

    function drawPreview() {
      const w = canvas.width;
      const h = canvas.height;
      ctx.clearRect(0, 0, w, h);
      const spacing = w / ledCount;
      const radius = Math.min(spacing * 0.45, 10);

      for (let i = 0; i < ledCount; i++) {
        const x = i * spacing + spacing / 2;
        const y = h / 2;
        let color;

        switch (currentEffect) {
          case 'static':
            color = currentColor;
            break;
          case 'rainbow':
            const hue = (rainbowHue + i * 3) % 256;
            color = `hsl(${Math.floor(hue * 360 / 255)}, 100%, 60%)`;
            break;
          case 'chase':
            color = (i === chasePos || i === (chasePos + 1) % ledCount || i === (chasePos + 2) % ledCount) ? currentColor : '#111';
            break;
          case 'breathe':
            {
              const phaseVal = (Math.sin(breathePhase) + 1) / 2;
              const rgb = hexToRgb(currentColor);
              const rr = Math.floor(rgb.r * phaseVal);
              const gg = Math.floor(rgb.g * phaseVal);
              const bb = Math.floor(rgb.b * phaseVal);
              color = `rgb(${rr},${gg},${bb})`;
            }
            break;
          case 'palette':
            const wheelPos = (paletteIndex + i * 5) % 256;
            color = `hsl(${Math.floor(wheelPos * 360 / 255)}, 100%, 60%)`;
            break;
        }

        ctx.beginPath();
        ctx.arc(x, y, radius, 0, 2 * Math.PI);
        ctx.fillStyle = color;
        ctx.shadowBlur = 6;
        ctx.shadowColor = color;
        ctx.fill();
        ctx.shadowBlur = 0;
      }
    }

    function advanceOneStep() {
      switch (currentEffect) {
        case 'rainbow': rainbowHue = (rainbowHue + 1) % 256; break;
        case 'chase': chasePos = (chasePos + 1) % ledCount; break;
        case 'breathe':
          breathePhase += 0.1;
          if (breathePhase > 2 * Math.PI) breathePhase = 0;
          break;
        case 'palette': paletteIndex = (paletteIndex + 1) % 256; break;
      }
    }

    function animate(timestamp) {
      if (currentEffect === 'static') {
        drawPreview();
        lastTimestamp = timestamp;
        requestAnimationFrame(animate);
        return;
      }
      if (!lastTimestamp) lastTimestamp = timestamp;
      let delta = timestamp - lastTimestamp;
      lastTimestamp = timestamp;
      if (delta > 500) delta = 500;

      const stepDuration = mapValue(speed, 0, 100, 80, 5);
      accumulator += delta;
      while (accumulator >= stepDuration) {
        advanceOneStep();
        accumulator -= stepDuration;
      }
      drawPreview();
      requestAnimationFrame(animate);
    }

    function updateColorUI(hex) {
      currentColor = hex;
      document.getElementById('colorPicker').value = hex;
      const rgb = hexToRgb(hex);
      document.getElementById('rSlider').value = rgb.r;
      document.getElementById('gSlider').value = rgb.g;
      document.getElementById('bSlider').value = rgb.b;
    }

    function setColorFromPicker(hex) {
      updateColorUI(hex);
      setColorDebounced(hex);
      if (currentEffect === 'static' || currentEffect === 'breathe') drawPreview();
    }

    function setColorFromSliders() {
      const r = parseInt(document.getElementById('rSlider').value);
      const g = parseInt(document.getElementById('gSlider').value);
      const b = parseInt(document.getElementById('bSlider').value);
      const hex = rgbToHex(r, g, b);
      updateColorUI(hex);
      setColorDebounced(hex);
      if (currentEffect === 'static' || currentEffect === 'breathe') drawPreview();
    }

    const setColorDebounced = debounce(function(hex) {
      const rgb = hexToRgb(hex);
      fetch(`/setcolor?r=${rgb.r}&g=${rgb.g}&b=${rgb.b}`);
    }, 100);

    const setBrightnessDebounced = debounce(function(val) {
      brightness = parseInt(val);
      document.getElementById('brightVal').innerText = val;
      fetch(`/setbrightness?val=${val}`);
    }, 100);

    const setSpeedDebounced = debounce(function(val) {
      speed = parseInt(val);
      document.getElementById('speedVal').innerText = val;
      fetch(`/setspeed?val=${val}`);
    }, 100);

    const setLedCountDebounced = debounce(function(val) {
      ledCount = parseInt(val);
      document.getElementById('ledCountVal').innerText = ledCount;
      fetch(`/setledcount?count=${ledCount}`);
      if (chasePos >= ledCount) chasePos = 0;
      drawPreview();
    }, 100);

    function setEffect(effect) {
      currentEffect = effect;
      fetch(`/seteffect?effect=${effect}`);
      document.querySelectorAll('button').forEach(b => b.classList.remove('active'));
      document.getElementById('btn-' + effect).classList.add('active');
      rainbowHue = chasePos = paletteIndex = 0;
      breathePhase = 0;
      accumulator = 0;
    }

    async function loadState() {
      try {
        const resp = await fetch('/status');
        const data = await resp.json();
        ledCount = data.ledcount;
        document.getElementById('ledCountSlider').value = ledCount;
        document.getElementById('ledCountVal').innerText = ledCount;
        brightness = data.brightness;
        document.getElementById('brightSlider').value = brightness;
        document.getElementById('brightVal').innerText = brightness;
        speed = data.speed;
        document.getElementById('speedSlider').value = speed;
        document.getElementById('speedVal').innerText = speed;
        currentColor = data.color;
        document.getElementById('colorPicker').value = currentColor;
        const rgb = hexToRgb(currentColor);
        document.getElementById('rSlider').value = rgb.r;
        document.getElementById('gSlider').value = rgb.g;
        document.getElementById('bSlider').value = rgb.b;
        currentEffect = data.effect;
        setEffect(data.effect);
      } catch (e) {
        console.log('State load failed, using defaults');
      }
    }

    window.onload = () => {
      loadState();
      requestAnimationFrame(animate);
    };
  </script>
</body>
</html>
)rawliteral";

// ---------- Helper: 32-bit color to hex (used in JSON) ----------
String colorToHex(uint32_t color) {
  char buf[8];
  sprintf(buf, "#%06X", color);
  return String(buf);
}

// ---------- Web Handlers ----------
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleSetColor() {
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    staticColor = strip.Color(r, g, b);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing color values");
  }
}

void handleSetEffect() {
  String effect = server.arg("effect");
  if (effect == "static")      currentEffect = STATIC;
  else if (effect == "rainbow") currentEffect = RAINBOW;
  else if (effect == "chase")   currentEffect = CHASE;
  else if (effect == "breathe") currentEffect = BREATHE;
  else if (effect == "palette") currentEffect = PALETTE_SWEEP;
  else {
    server.send(400, "text/plain", "Unknown effect");
    return;
  }
  server.send(200, "text/plain", "Effect set");
}

void handleSetBrightness() {
  if (server.hasArg("val")) {
    brightness = server.arg("val").toInt();
    strip.setBrightness(brightness);
    server.send(200, "text/plain", "OK");
  }
}

void handleSetSpeed() {
  if (server.hasArg("val")) {
    speed = server.arg("val").toInt();
    server.send(200, "text/plain", "OK");
  }
}

void handleSetLedCount() {
  if (server.hasArg("count")) {
    ledCount = server.arg("count").toInt();
    if (ledCount < 1) ledCount = 1;
    if (ledCount > 300) ledCount = 300;
    strip.updateLength(ledCount);
    strip.show();
    server.send(200, "text/plain", "OK");
  }
}

void handleStatus() {
  // Fixed-size buffer to avoid heap fragmentation
  char json[256];
  const char* effStr = "static";
  if (currentEffect == RAINBOW) effStr = "rainbow";
  else if (currentEffect == CHASE) effStr = "chase";
  else if (currentEffect == BREATHE) effStr = "breathe";
  else if (currentEffect == PALETTE_SWEEP) effStr = "palette";

  snprintf(json, sizeof(json),
           "{\"ledcount\":%d,\"brightness\":%d,\"speed\":%d,\"color\":\"#%06X\",\"effect\":\"%s\"}",
           ledCount, brightness, speed, staticColor, effStr);
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// ---------- NeoPixel Helpers ----------
void fillStrip(uint32_t color) {
  for (int i = 0; i < ledCount; i++) {
    strip.setPixelColor(i, color);
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  if (WheelPos < 170) { WheelPos -= 85; return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3); }
  WheelPos -= 170; return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// ---------- Animation Update ----------
void updateEffect() {
  switch (currentEffect) {
    case STATIC: fillStrip(staticColor); break;
    case RAINBOW:
      for (int i = 0; i < ledCount; i++)
        strip.setPixelColor(i, Wheel((rainbowHue + i * 3) & 255));
      rainbowHue++;
      break;
    case CHASE:
      for (int i = 0; i < ledCount; i++)
        strip.setPixelColor(i, (i == chasePos || i == (chasePos + 1) % ledCount || i == (chasePos + 2) % ledCount) ? staticColor : 0);
      chasePos = (chasePos + 1) % ledCount;
      break;
    case BREATHE: {
      float val = (sin(breathePhase) + 1) / 2.0;
      uint8_t r = (staticColor >> 16) & 0xFF;
      uint8_t g = (staticColor >> 8) & 0xFF;
      uint8_t b = staticColor & 0xFF;
      fillStrip(strip.Color(r * val, g * val, b * val));
      breathePhase += 0.1;
      if (breathePhase > 2 * PI) breathePhase = 0;
      break;
    }
    case PALETTE_SWEEP:
      for (int i = 0; i < ledCount; i++)
        strip.setPixelColor(i, Wheel((paletteIndex + i * 5) & 255));
      paletteIndex++;
      break;
  }
  strip.show();
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.updateLength(ledCount);
  strip.setBrightness(brightness);
  strip.show();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", handleRoot);
  server.on("/setcolor", handleSetColor);
  server.on("/seteffect", handleSetEffect);
  server.on("/setbrightness", handleSetBrightness);
  server.on("/setspeed", handleSetSpeed);
  server.on("/setledcount", handleSetLedCount);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
  server.handleClient();          // process HTTP requests
  yield();                        // feed the watchdog

  unsigned long now = millis();
  // Map speed 0-100 to delay range 80..10 ms (slow..fast)
  int delayTime = map(speed, 0, 100, 80, 10);
  if (now - lastUpdate >= delayTime) {
    lastUpdate = now;
    updateEffect();
  }
}
