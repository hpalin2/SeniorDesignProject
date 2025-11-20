#include <WiFi.h>
#include <PubSubClient.h>

#define FLOW_PIN 12   
#define MOTION_PIN 5   

const unsigned long MOTION_STABLE_MS = 5000;  // 5 seconds

// ── Wi-Fi & MQTT Config ─────────────────────────────
const char* WIFI_SSID  = "SSID";
const char* WIFI_PASS  = "PASSWORD";

const char* MQTT_HOST  = "MACHINE_IP"; 
const int   MQTT_PORT  = 1883;

const char* ROOM_NAME  = "OR-DEV";             // <room> for topic suction/<room>/state
const char* DEVICE_ID  = "esp32_dev1";

const char* TOPIC_STATUS = "suction/dev1/status";

char TOPIC_STATE[96];

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool suctionOn       = false;
bool motionState     = false;

bool lastSentSuction = false;
bool lastSentMotion  = false;

int  lastRawMotion          = LOW;
unsigned long lastMotionChangeTime = 0;
bool motionInitialized      = false;

unsigned long lastSampleMs = 0;

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

  const char* willPayload = "{\"status\":\"offline\"}";
  bool ok = mqtt.connect(
      DEVICE_ID,
      TOPIC_STATUS,
      0,               
      true,
      willPayload
  );

  if (ok) {
    Serial.println("MQTT connected.");
    mqtt.publish(TOPIC_STATUS, "{\"status\":\"online\"}", true);
  } else {
    Serial.print("MQTT connect FAILED, state=");
    Serial.println(mqtt.state());
  }
  return ok;
}

void publishState(bool suction, bool motion) {
  // topic:   suction/<room>/state
  // payload: {"suction_on": true/false, "motion": true/false}
  const char* s_val = suction ? "true" : "false";
  const char* m_val = motion  ? "true" : "false";

  char buf[96];
  snprintf(buf, sizeof(buf),
           "{\"suction_on\":%s,\"motion\":%s}", s_val, m_val);

  bool ok = mqtt.publish(TOPIC_STATE, buf, true);
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

  pinMode(FLOW_PIN, INPUT_PULLUP);   // HIGH = no suction, LOW = suction ON

  pinMode(MOTION_PIN, INPUT_PULLDOWN);

  // Build the MQTT state topic once
  snprintf(TOPIC_STATE, sizeof(TOPIC_STATE), "suction/%s/state", ROOM_NAME);

  delay(150);
  connectWiFi();
  while (!connectMQTT()) {
    delay(1000);
  }

  unsigned long now = millis();
  int rawMotion = digitalRead(MOTION_PIN);
  lastRawMotion         = rawMotion;
  motionState           = (rawMotion == HIGH); 
  lastMotionChangeTime  = now;
  motionInitialized     = true;

  bool rawFlow    = digitalRead(FLOW_PIN);
  suctionOn       = !rawFlow;             

  lastSentSuction = !suctionOn;
  lastSentMotion  = !motionState;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    connectMQTT();
  }
  mqtt.loop();

  if (millis() - lastSampleMs >= 100) {
    lastSampleMs = millis();
    unsigned long now = millis();
    bool rawFlow = digitalRead(FLOW_PIN);
    bool newSuctionOn = !rawFlow;   

    if (newSuctionOn != suctionOn) {
      suctionOn = newSuctionOn;
    }

    int rawMotion = digitalRead(MOTION_PIN);

    if (!motionInitialized) {
      lastRawMotion         = rawMotion;
      motionState           = (rawMotion == HIGH);
      lastMotionChangeTime  = now;
      motionInitialized     = true;
    }

    if (rawMotion != lastRawMotion) {
      lastRawMotion        = rawMotion;
      lastMotionChangeTime = now;
    }

    bool candidateState = (lastRawMotion == HIGH);
    if (candidateState != motionState &&
        (now - lastMotionChangeTime) >= MOTION_STABLE_MS) {
      motionState = candidateState;
    }

    if (suctionOn != lastSentSuction || motionState != lastSentMotion) {
      publishState(suctionOn, motionState);
      lastSentSuction = suctionOn;
      lastSentMotion  = motionState;
    }
  }
}
