#include <WiFi.h>
#include <PubSubClient.h>


#define TRIG_PIN 14
#define ECHO_PIN 27          

const float MOTION_ON_CM  = 10.0f;  // motion = true when distance <= 10 cm
const float MOTION_OFF_CM = 12.0f;  // motion = false when distance >= 12 cm

// ── Wi-Fi & MQTT Config ────────────────────────────────────────────
const char* WIFI_SSID  = "WIFI_SSID";
const char* WIFI_PASS  = "WIFI_PASSWORD";

const char* MQTT_HOST  = "MACHINE_IP_ADDRESS";
const int   MQTT_PORT  = 1883;
const char* DEVICE_ID  = "esp32_dev1";

// Topics 
const char* TOPIC_STATUS   = "suction/dev1/status";     
const char* TOPIC_TELEM    = "suction/dev1/telemetry";  
const char* ROOM_ID        = "OR-DEV";

// ── Globals ────────────────────────────────────────────────────────
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool motionState = false;        
bool lastSentMotion = false;     
unsigned long lastPingMs = 0;    
unsigned long lastHeartbeatMs = 0;

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

  const char* willTopic = TOPIC_STATUS;
  const char* willMsg   = "{\"status\":\"offline\"}";

  Serial.print("Connecting MQTT to ");
  Serial.println(MQTT_HOST);

  bool ok = mqtt.connect(DEVICE_ID, nullptr, nullptr, willTopic, 1, true, willMsg);

  if (ok) {
    Serial.println("MQTT connected.");
    // Announce we're online (retained)
    mqtt.publish(TOPIC_STATUS, "{\"status\":\"online\"}", true);
  } else {
    Serial.print("MQTT connect FAILED, state=");
    Serial.println(mqtt.state());
  }
  return ok;
}

float readDistanceCm() {
  unsigned long t0 = micros();
  while (digitalRead(ECHO_PIN) == HIGH && (micros() - t0) < 30000UL) {}

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long us = pulseInLong(ECHO_PIN, HIGH, 30000UL);
  if (us == 0) return -1.0f;         
  if (us < 150UL) return -2.0f;      

  return (us * 0.0343f) * 0.5f;
}

// Publish a motion event JSON, optionally including distance if valid
void publishMotion(bool motion, float distanceCm) {
  // Build a small JSON payload. Keep it simple to avoid heap churn.
  // Example: {"room_id":"OR-DEV","motion":true,"distance_cm":9.8}
  char buf[160];
  if (distanceCm > 0) {
    snprintf(buf, sizeof(buf),
             "{\"room_id\":\"%s\",\"motion\":%s,\"distance_cm\":%.1f}",
             ROOM_ID, motion ? "true" : "false", distanceCm);
  } else {
    snprintf(buf, sizeof(buf),
             "{\"room_id\":\"%s\",\"motion\":%s}",
             ROOM_ID, motion ? "true" : "false");
  }

  bool ok = mqtt.publish(TOPIC_TELEM, buf);
  Serial.print("Publish motion ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);  
  digitalWrite(TRIG_PIN, LOW);

  delay(150);
  connectWiFi();
  while (!connectMQTT()) {
    delay(1000);
  }

  lastSentMotion = !motionState; // force first publish
}

void loop() {
  // Maintain connectivity
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    connectMQTT();
  }
  mqtt.loop();

  if (millis() - lastPingMs >= 90) {
    lastPingMs = millis();

    float cm = readDistanceCm();
    if (cm > 0) {
      if (!motionState && cm <= MOTION_ON_CM) {
        motionState = true;
      } else if (motionState && cm >= MOTION_OFF_CM) {
        motionState = false;
      }

      if (motionState != lastSentMotion) {
        publishMotion(motionState, cm);
        lastSentMotion = motionState;
      }

    } 
  }
}
