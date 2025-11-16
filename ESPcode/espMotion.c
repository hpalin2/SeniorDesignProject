#include <WiFi.h>
#include <PubSubClient.h>

#define MOTION_PIN 12

// ── Wi-Fi & MQTT Config ───────────────────────────────────────────
const char* WIFI_SSID  = "Iphone 110";
const char* WIFI_PASS  = "highop23";

const char* MQTT_HOST  = "172.20.10.2"; 
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
int val = 0;

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

  pinMode(MOTION_PIN, INPUT);

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
  val = digitalRead(MOTION_PIN);
  if(!suctionOn && val){ //turn suction on
    suctionOn = true;
  } else if(suctionOn && !val){ //turn suction off
    suctionOn = false;
  }

  // Publish only on change
  if (suctionOn != lastSentSuction) {
    publishSuctionState(suctionOn);
    lastSentSuction = suctionOn;
  }
}
