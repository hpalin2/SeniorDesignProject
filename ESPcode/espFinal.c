#include <WiFi.h>
#include <PubSubClient.h>

// ── Pins ─────────────────────────────────────────────
#define FLOW_PIN 12    // suction / airflow sensor: GND = suction ON
#define MOTION_PIN 5   // PIR OUT pin (change to match your wiring)

// ── Motion debounce config ──────────────────────────
// New state must be stable for this long before we accept/publish it
const unsigned long MOTION_STABLE_MS = 5000;  // 5 seconds

// ── Wi-Fi & MQTT Config ─────────────────────────────
const char* WIFI_SSID  = "SSID";
const char* WIFI_PASS  = "PASSWORD";

const char* MQTT_HOST  = "MACHINE_IP"; 
const int   MQTT_PORT  = 1883;

const char* ROOM_NAME  = "OR-DEV";             // <room> for topic suction/<room>/state
const char* DEVICE_ID  = "esp32_dev1";

// Optional device status topic (Last Will & Testament)
const char* TOPIC_STATUS = "suction/dev1/status";

// Compose state topic: "suction/<room>/state"
char TOPIC_STATE[96];

// ── Globals ─────────────────────────────────────────
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Current logical states
bool suctionOn       = false;
bool motionState     = false;

// Last values we actually published
bool lastSentSuction = false;
bool lastSentMotion  = false;

// Motion debounce internals
int  lastRawMotion          = LOW;        // last raw reading from PIR pin
unsigned long lastMotionChangeTime = 0;   // when the raw state last changed
bool motionInitialized      = false;

unsigned long lastSampleMs = 0;

// ── Wi-Fi & MQTT helpers ────────────────────────────
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
      /*willTopic*/  TOPIC_STATUS,
      /*willQos*/    0,               // PubSubClient supports QoS 0 only
      /*willRetain*/ true,
      /*willMessage*/willPayload
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

// Publish combined state payload expected by your ingestor
void publishState(bool suction, bool motion) {
  // topic:   suction/<room>/state
  // payload: {"suction_on": true/false, "motion": true/false}
  const char* s_val = suction ? "true" : "false";
  const char* m_val = motion  ? "true" : "false";

  char buf[96];
  snprintf(buf, sizeof(buf),
           "{\"suction_on\":%s,\"motion\":%s}", s_val, m_val);

  bool ok = mqtt.publish(TOPIC_STATE, buf, true /*retained*/);
  Serial.print("Publish state: suction_on=");
  Serial.print(s_val);
  Serial.print(", motion=");
  Serial.print(m_val);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAIL");
}

// ── Setup / Loop ────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Suction input: manual grounding to simulate suction
  pinMode(FLOW_PIN, INPUT_PULLUP);   // HIGH = no suction, LOW = suction ON

  // PIR input: Adafruit PIR usually drives the line HIGH/LOW itself.
  // INPUT_PULLDOWN also worked in your test sketch; keep if you like:
  pinMode(MOTION_PIN, INPUT_PULLDOWN);  // or INPUT

  // Build the MQTT state topic once
  snprintf(TOPIC_STATE, sizeof(TOPIC_STATE), "suction/%s/state", ROOM_NAME);

  delay(150);
  connectWiFi();
  while (!connectMQTT()) {
    delay(1000);
  }

  // Initialize motion debounce state
  unsigned long now = millis();
  int rawMotion = digitalRead(MOTION_PIN);
  lastRawMotion         = rawMotion;
  motionState           = (rawMotion == HIGH);  // accept initial state immediately
  lastMotionChangeTime  = now;
  motionInitialized     = true;

  // Initialize suction state based on current pin
  bool rawFlow    = digitalRead(FLOW_PIN); // HIGH = no flow, LOW = flow
  suctionOn       = !rawFlow;              // invert to make it intuitive

  // Force first publish
  lastSentSuction = !suctionOn;
  lastSentMotion  = !motionState;
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

  // Sample every ~100 ms
  if (millis() - lastSampleMs >= 100) {
    lastSampleMs = millis();
    unsigned long now = millis();

    // ── Suction: digital input with pull-up ─────────
    // HIGH = no flow, LOW = flow (suction)
    bool rawFlow = digitalRead(FLOW_PIN);
    bool newSuctionOn = !rawFlow;   // invert so true = suction present

    if (newSuctionOn != suctionOn) {
      suctionOn = newSuctionOn;
    }

    // ── Motion: reuse your stability logic ──────────
    int rawMotion = digitalRead(MOTION_PIN);

    if (!motionInitialized) {
      lastRawMotion         = rawMotion;
      motionState           = (rawMotion == HIGH);
      lastMotionChangeTime  = now;
      motionInitialized     = true;
    }

    // If the raw reading changes, remember WHEN it changed
    if (rawMotion != lastRawMotion) {
      lastRawMotion        = rawMotion;
      lastMotionChangeTime = now;
    }

    // Only accept a motion state change if:
    //  1) The raw state differs from current motionState, AND
    //  2) It has stayed in that new state for at least MOTION_STABLE_MS
    bool candidateState = (lastRawMotion == HIGH);
    if (candidateState != motionState &&
        (now - lastMotionChangeTime) >= MOTION_STABLE_MS) {
      motionState = candidateState;
    }

    // ── Publish if either state changed since last publish ─
    if (suctionOn != lastSentSuction || motionState != lastSentMotion) {
      publishState(suctionOn, motionState);
      lastSentSuction = suctionOn;
      lastSentMotion  = motionState;
    }
  }
}
