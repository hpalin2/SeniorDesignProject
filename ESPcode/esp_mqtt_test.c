#include <WiFi.h>
#include <PubSubClient.h>

// ── Wi-Fi & MQTT Config ─────────────────────────────────────────────
const char* WIFI_SSID  = "";
const char* WIFI_PASS  = "";

const char* MQTT_HOST  = "";   // Broker's LAN IP (Mac or Pi)
const int   MQTT_PORT  = 1883;
const char* DEVICE_ID  = "esp32_dev1";

// ── Globals ─────────────────────────────────────────────────────────
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastSend = 0;

// ── Helpers ────────────────────────────────────────────────────────
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi OK. IP: ");
  Serial.println(WiFi.localIP());
}

bool connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  // Last Will: if device drops unexpectedly, broker sets "offline"
  const char* willTopic = "suction/dev1/status";
  const char* willMsg   = "{\"status\":\"offline\"}";

  Serial.print("Connecting MQTT to ");
  Serial.println(MQTT_HOST);

  // No username/password (anonymous)
  bool ok = mqtt.connect(DEVICE_ID, nullptr, nullptr, willTopic, 1, true, willMsg);

  if (ok) {
    Serial.println("MQTT connected.");
    // Announce we're online (retained)
    mqtt.publish("suction/dev1/status", "{\"status\":\"online\"}", true);
  } else {
    Serial.print("MQTT connect FAILED, state=");
    Serial.println(mqtt.state());
  }
  return ok;
}

// ── Arduino Lifecycle ──────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);

  connectWiFi();

  // Retry MQTT until connected
  while (!connectMQTT()) {
    delay(1000);
  }
}

void loop() {
  // Reconnect Wi-Fi if needed
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Reconnect MQTT if needed
  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    connectMQTT();
  }

  mqtt.loop();

  // Publish once per second
  if (millis() - lastSend >= 1000) {
    lastSend = millis();

    float fake_flow   = (millis() % 5000) / 1000.0;     // 0..5 LPM
    bool  fake_motion = (millis() / 1000) % 2;          // toggles each second

    String payload = String("{\"room_id\":\"OR-DEV\",\"flow_lpm\":")
                   + String(fake_flow, 2)
                   + ",\"motion\":" + (fake_motion ? "true" : "false") + "}";

    bool ok = mqtt.publish("suction/dev1/telemetry", payload.c_str());
    Serial.print("Publish ");
    Serial.println(ok ? "OK" : "FAIL");
  }
}
