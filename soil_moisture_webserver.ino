#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// ⚙️ WiFi Settings
// ==========================================
const char* ssid = "shon";       // ⚠️ Replace with your WiFi Network Name
const char* password = "njanparayilla"; // ⚠️ Replace with your WiFi Password

// Start the ESP32 Web Server on Port 80
WebServer server(80);

// ==========================================
// 🔌 Sensor & Calibration Settings
// ==========================================
#define SOIL_MOISTURE_PIN 34 

const int DRY_VAL = 4095; // Typical completely dry analog reading
const int WET_VAL = 1500; // Typical completely wet analog reading

int currentRaw = 0;
int currentPct = 0;

// ==========================================
// 🎨 The Beautiful Website Code (HTML/CSS/JS)
// ==========================================
// We use PROGMEM to save RAM, storing the giant HTML string in Flash memory
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Plant Monitor</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;500;700&display=swap" rel="stylesheet">
<style>
  :root {
    --bg: #090e17;
    --card-bg: rgba(255, 255, 255, 0.05);
    --border: rgba(255, 255, 255, 0.1);
    --primary: #4dabf7; 
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 0; height: 100vh;
    font-family: 'Outfit', sans-serif;
    background: var(--bg); color: #fff;
    display: flex; justify-content: center; align-items: center;
    background-image: radial-gradient(circle at top right, rgba(77, 171, 247, 0.15), transparent 40%),
                      radial-gradient(circle at bottom left, rgba(81, 207, 102, 0.1), transparent 40%);
  }
  .dashboard {
    width: 340px;
    background: var(--card-bg);
    backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px);
    border: 1px solid var(--border);
    border-radius: 30px;
    padding: 40px 30px;
    text-align: center;
    box-shadow: 0 40px 80px rgba(0,0,0,0.6);
  }
  h1 { font-size: 22px; font-weight: 500; margin-bottom: 30px; letter-spacing: 1px; color: #e2e8f0; text-transform: uppercase; }
  
  .liquid-container {
    position: relative; width: 220px; height: 220px; margin: 0 auto 30px;
    border-radius: 50%;
    background: rgba(0, 0, 0, 0.4);
    box-shadow: inset 0 0 20px rgba(0,0,0,0.8), 0 0 25px rgba(77, 171, 247, 0.2);
    border: 4px solid rgba(255,255,255,0.05);
    overflow: hidden;
  }
  
  /* Animated Water Fill */
  .wave {
    position: absolute; width: 450px; height: 450px;
    background: var(--primary);
    border-radius: 40%;
    bottom: -450px; left: -115px;
    animation: spin 6s linear infinite;
    transition: bottom 1.5s cubic-bezier(0.4, 0, 0.2, 1), background 1s ease-in-out;
  }
  .wave:nth-child(2) {
    background: rgba(255,255,255,0.15); animation: spin 8s linear infinite;
  }
  
  @keyframes spin { 100% { transform: rotate(360deg); } }

  .data-text {
    position: absolute; inset: 0;
    display: flex; flex-direction: column; justify-content: center; align-items: center;
    z-index: 10; text-shadow: 0 4px 10px rgba(0,0,0,0.5);
  }
  .pct { font-size: 64px; font-weight: 700; line-height: 1; }
  .pct span { font-size: 24px; opacity: 0.8; }
  
  .status {
    padding: 10px 20px; border-radius: 50px; font-size: 15px; font-weight: 500;
    letter-spacing: 1px; text-transform: uppercase;
    background: rgba(255,255,255,0.08); transition: color 0.5s;
    margin-bottom: 20px;
  }
  
  .footer { margin-top: 25px; font-size: 13px; color: #94a3b8; display: flex; justify-content: space-between;}
</style>
</head>
<body>
<div class="dashboard">
  <h1>Soil Monitor</h1>
  <div class="liquid-container">
    <div class="wave" id="wave1"></div>
    <div class="wave" id="wave2"></div>
    <div class="data-text">
      <div class="pct" id="pctval">--<span>%</span></div>
    </div>
  </div>
  <div class="status" id="status-text">Fetching data...</div>
  <div class="footer">
    <span>Sensor: ESP32</span>
    <span>Raw ADC: <span id="rawval">----</span></span>
  </div>
</div>
<script>
  function fetchSensorData() {
    fetch('/data')
      .then(response => response.json())
      .then(data => {
        let moisture = data.moisture;
        document.getElementById('pctval').innerHTML = `${moisture}<span>%</span>`;
        document.getElementById('rawval').innerText = data.raw;
        
        let bottomVal = -450 + (moisture * 2.2);
        document.getElementById('wave1').style.bottom = `${bottomVal}px`;
        document.getElementById('wave2').style.bottom = `${bottomVal + 15}px`;
        
        let root = document.documentElement;
        let statusEl = document.getElementById('status-text');
        
        if (moisture < 30) {
          root.style.setProperty('--primary', '#ff6b6b'); // Red
          statusEl.innerText = "Dry (Needs Water)";
          statusEl.style.color = "#ff6b6b";
        } else if (moisture > 75) {
          root.style.setProperty('--primary', '#3b82f6'); // Blue
          statusEl.innerText = "Drenched";
          statusEl.style.color = "#3b82f6";
        } else {
          root.style.setProperty('--primary', '#51cf66'); // Green
          statusEl.innerText = "Optimal Moisture";
          statusEl.style.color = "#51cf66";
        }
      })
      .catch(error => {
        document.getElementById('status-text').innerText = "Offline";
        document.getElementById('status-text').style.color = "#ff6b6b";
      });
  }
  // Fast background polling
  setInterval(fetchSensorData, 1000);
</script>
</body>
</html>
)rawliteral";

// ==========================================
// 📡 Web Server Endpoints
// ==========================================

// Deliver the website when someone hits the IP
void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/html", index_html);
}

// Deliver real-time data to the running website
void handleData() {
  String json = "{\"moisture\":";
  json += String(currentPct);
  json += ",\"raw\":";
  json += String(currentRaw);
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", json);
}

// ==========================================
// 🚀 Main Setup & Loop
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(100);
  
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  Serial.println("\n--- ESP32 Next-Gen Soil Webserver ---");
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("🌐 Open Browser: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Failed to Connect! Check SSID/Password.");
  }

  // Bind the server API endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  
  // Turn on the ESP32 server 
  server.begin();
}

void loop() {
  // Process any incoming requests from our website
  server.handleClient();
  
  // Calculate Soil Moisture exactly twice a second
  static unsigned long lastReading = 0;
  if(millis() - lastReading > 500) { 
    lastReading = millis();
    
    currentRaw = analogRead(SOIL_MOISTURE_PIN);
    currentPct = map(currentRaw, DRY_VAL, WET_VAL, 0, 100);
    currentPct = constrain(currentPct, 0, 100);
  }
}
