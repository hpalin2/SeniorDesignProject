const int FLOW_PIN = 27;

void setup() {
  pinMode(FLOW_PIN, INPUT_PULLUP);  // internal pull-up
  Serial.begin(115200);
}

void loop() {
  bool raw = digitalRead(FLOW_PIN);     // HIGH = no flow, LOW = flow
  bool airflow_present = !raw;          // invert to make it intuitive

  if (airflow_present) {
    Serial.println("Airflow detected!");
  } else {
    Serial.println("No airflow.");
  }

  delay(1000);
}
