#include <WiFi.h>
#include <PubSubClient.h>

#define TRIG_PIN 14
#define ECHO_PIN 27

// Hysteresis: inside <= ON, outside >= OFF
const float SUCTION_ON_CM  = 10.0f;
const float SUCTION_OFF_CM = 12.0f;

// ── Wi-Fi & MQTT Config ───────────────────────────────────────────
const char* WIFI_SSID  = "WIFI_SSID";
const char* WIFI_PASS  = "WIFI_PASSWORD";

const char* MQTT_HOST  = "MACHINE_IP_ADDRESS"; // e.g., 192.168.1.50
const int   MQTT_PORT  = 1883;

const char* ROOM_NAME  = "OR-DEV";             // <room> for topic suction/<room>/state
const char* DEVICE_ID  = "esp32_dev1";

// Optional device status topic (Last Will & Testament)
const char* TOPIC_STATUS = "suction/dev1/status";

// Compose state topic: "suction/<room>/state"
char TOPIC_STATE[96];

// ── Globals ───────────────────────────────────────────────────────
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool suctionOn = false;        // current derived state
bool lastSentSuction = true;   // force first publish
unsigned long lastSampleMs = 0;

// ── Helpers ───────────────────────────────────────────────────────
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

  // LWT: if we drop, broker sets status=offline (retained)
  const char* willPayload = "{\"status\":\"offline\"}";
  bool ok = mqtt.connect(
      DEVICE_ID,
      /*willTopic*/ TOPIC_STATUS,
      /*willQos*/   0,               // PubSubClient supports QoS 0 only
      /*willRetain*/ true,
      /*willMessage*/ willPayload
  );

  if (ok) {
    Serial.println("MQTT connected.");
    mqtt.publish(TOPIC_STATUS, "{\"status\":\"online\"}", true /*retained*/);
  } else {
    Serial.print("MQTT connect FAILED, state=");
    Serial.println(mqtt.state());
  }
  return ok;
}

// Ultrasonic distance in cm (negative = invalid)
float readDistanceCm() {
  // ensure line is low
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // 10us pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // echo timeout ~30ms
  unsigned long us = pulseInLong(ECHO_PIN, HIGH, 30000UL);
  if (us == 0)    return -1.0f; // timeout/no echo
  if (us < 150UL) return -2.0f; // too close / spurious

  // speed of sound: 0.0343 cm/us, divide by 2 (round-trip)
  return (us * 0.0343f) * 0.5f;
}

// Publish suction state payload expected by your ingestor
void publishSuctionState(bool on) {
  // Exactly what your Crow app expects:
  // topic:  suction/<room>/state
  // payload: {"suction_on": true/false}
  const char* val = on ? "true" : "false";
  char buf[48];
  snprintf(buf, sizeof(buf), "{\"suction_on\":%s}", val);

  // Retained so late subscribers get the latest instantly
  bool ok = mqtt.publish(TOPIC_STATE, buf, true /*retained*/);
  Serial.print("Publish suction_on=");
  Serial.print(val);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // Build the state topic once
  snprintf(TOPIC_STATE, sizeof(TOPIC_STATE), "suction/%s/state", ROOM_NAME);

  delay(150);
  connectWiFi();
  while (!connectMQTT()) {
    delay(1000);
  }

  // Force first publish on first valid reading
  lastSentSuction = !suctionOn;
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

  // Sample every ~100 ms (adjust as desired)
  if (millis() - lastSampleMs >= 100) {
    lastSampleMs = millis();

    float cm = readDistanceCm();
    if (cm > 0) {
      // Hysteresis → suction state
      if (!suctionOn && cm <= SUCTION_ON_CM) {
        suctionOn = true;
      } else if (suctionOn && cm >= SUCTION_OFF_CM) {
        suctionOn = false;
      }

      // Publish only on change
      if (suctionOn != lastSentSuction) {
        publishSuctionState(suctionOn);
        lastSentSuction = suctionOn;
      }
    }
  }
}
