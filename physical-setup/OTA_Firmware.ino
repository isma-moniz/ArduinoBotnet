#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

// ── Network config ────────────────────────────────────────────────
const char* SSID        = "Mock_Lab";
const char* PASSWORD    = "password";
const char* HOST_NAME   = "esp32-mock-node";
const char* WWW_USER    = "admin";
const char* WWW_PASS    = "password";

WebServer server(80);

// ── HTML ──────────────────────────────────────────────────────────
const char HTML_INDEX[] PROGMEM = R"(
<!DOCTYPE html><html><head>
<meta charset='utf-8'><title>Control Panel</title>
<style>body{font-family:sans-serif;max-width:600px;margin:40px auto;padding:20px}</style>
</head><body>
<h1>Control Panel</h1>
<p>System Online.</p>
<p>Uptime: <span id='up'></span></p>
<a href='/ota'>⚙ Advanced Settings (OTA)</a>
<script>
  fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('up').textContent = d.uptime + 's';
  });
</script>
</body></html>
)";

const char HTML_OTA[] PROGMEM = R"(
<!DOCTYPE html><html><head>
<meta charset='utf-8'><title>OTA Update</title>
<style>body{font-family:sans-serif;max-width:600px;margin:40px auto;padding:20px}</style>
</head><body>
<h2>Firmware Update</h2>
<form method='POST' action='/update' enctype='multipart/form-data'>
  <input type='file' name='update' accept='.bin'>
  <br><br>
  <input type='submit' value='Flash Firmware'>
</form>
<div id='msg'></div>
</body></html>
)";

// ── Handlers ──────────────────────────────────────────────────────
void handleRoot() {
  if (!server.authenticate(WWW_USER, WWW_PASS))
    return server.requestAuthentication();
  server.send_P(200, "text/html", HTML_INDEX);
}

void handleOTA() {
  if (!server.authenticate(WWW_USER, WWW_PASS))
    return server.requestAuthentication();
  server.send_P(200, "text/html", HTML_OTA);
}

void handleStatus() {
  if (!server.authenticate(WWW_USER, WWW_PASS))
    return server.requestAuthentication();

  String json = "{";
  json += "\"uptime\":"  + String(millis() / 1000) + ",";
  json += "\"heap\":"    + String(ESP.getFreeHeap()) + ",";
  json += "\"ip\":\""    + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":"    + String(WiFi.RSSI());
  json += "}";
  server.send(200, "application/json", json);
}

void handleUpdateDone() {
  bool ok = !Update.hasError();
  server.send(200, "text/plain", ok ? "Success. Restarting..." : "Update failed!");
  Serial.println(ok ? "[OTA] Flash complete. Restarting..." : "[OTA] Flash error.");
  delay(500);
  ESP.restart();
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("[OTA] Receiving: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[OTA] Final size: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

// ── WiFi with timeout ─────────────────────────────────────────────
bool connectWiFi(int timeoutSec = 20, int maxRetries = 5) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOST_NAME);
  
  // Force static IP to bypass DHCP entirely
  IPAddress local_IP(10, 42, 0, 50);
  IPAddress gateway(10, 42, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(10, 42, 0, 1);
  
  if (!WiFi.config(local_IP, gateway, subnet, dns)) {
    Serial.println("[WiFi] Static IP config failed.");
  }

  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    Serial.printf("[WiFi] Attempt %d/%d — connecting to '%s'...\n", attempt, maxRetries, SSID);
    WiFi.begin(SSID, PASSWORD);

    int elapsed = 0;
    while (WiFi.status() != WL_CONNECTED && elapsed < timeoutSec * 2) {
      delay(500);
      Serial.print(".");
      elapsed++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) return true;

    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println("[WiFi] SSID not found."); break;
      case WL_CONNECT_FAILED:
        Serial.println("[WiFi] Wrong password."); break;
      case WL_DISCONNECTED:
        Serial.println("[WiFi] Could not associate."); break;
      default:
        Serial.printf("[WiFi] Status code: %d\n", WiFi.status()); break;
    }

    Serial.println("[WiFi] Retrying in 3s...");
    WiFi.disconnect();
    delay(3000);
  }

  return false;
}

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);

  if (!connectWiFi()) {
    Serial.println("[ERROR] Could not connect to WiFi. Restarting in 5s...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("[SYSTEM] Mock device online.");
  Serial.printf("[SYSTEM] IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[SYSTEM] Hostname: http://%s.local\n", HOST_NAME);
  Serial.printf("[SYSTEM] RSSI: %d dBm\n", WiFi.RSSI());

  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/ota",    HTTP_GET,  handleOTA);
  server.on("/status", HTTP_GET,  handleStatus);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);

  server.begin();
  Serial.println("[HTTP] Server started on port 80.");
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
  server.handleClient();

  // Reconnect if WiFi drops
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost. Reconnecting...");
    connectWiFi();
  }
}