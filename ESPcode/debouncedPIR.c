#define MOTION_PIN 27

// New state must be stable for this long before we accept/print it
const unsigned long STABLE_TIME_MS = 2000;  // 2 seconds

bool lastReportedState = LOW;          // Last state we actually printed
int  lastRawState      = LOW;          // Last raw reading from the pin
unsigned long lastRawChangeTime = 0;   // When the raw state last changed
bool initialized = false;

void setup() {
  Serial.begin(115200);
  pinMode(MOTION_PIN, INPUT_PULLDOWN);  // or INPUT if PULLDOWN not available
}

void loop() {
  unsigned long now = millis();
  int raw = digitalRead(MOTION_PIN);

  // First run initialization
  if (!initialized) {
    lastRawState = raw;
    lastReportedState = raw;
    lastRawChangeTime = now;
    initialized = true;

    // Print initial state once
    if (lastReportedState == HIGH) {
      Serial.println("Motion Detected");
    } else {
      Serial.println("No motion Detected");
    }
    delay(100);
    return;
  }

  // If the raw reading changes, remember WHEN it changed
  if (raw != lastRawState) {
    lastRawState = raw;
    lastRawChangeTime = now;
  }

  // Only consider a state change if:
  //  1) The raw state is different from what we last reported, AND
  //  2) It has stayed in that new raw state for at least STABLE_TIME_MS
  if (lastRawState != lastReportedState &&
      (now - lastRawChangeTime) >= STABLE_TIME_MS) {

    lastReportedState = lastRawState;  // accept the new state

    if (lastReportedState == HIGH) {
      Serial.println("Motion Detected");
    } else {
      Serial.println("No motion Detected");
    }
  }

  delay(100);  // sample every 100 ms
}
