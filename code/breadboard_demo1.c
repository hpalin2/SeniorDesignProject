// #define TRIG_PIN 18
// #define ECHO_PIN 4      // or 34/35/32/33 (input-only) if you want super-stable inputs
// #define LED_PIN 14

// // Distance thresholds with a little hysteresis to avoid flicker
// const float LED_ON_CM  = 10.0;  // turn on when <= 10 cm
// const float LED_OFF_CM = 12.0;  // turn off when >= 12 cm

// bool ledState = false;

// void setup() {
//   Serial.begin(115200);
//   pinMode(TRIG_PIN, OUTPUT);
//   pinMode(ECHO_PIN, INPUT);      // no pullups needed
//   pinMode(LED_PIN, OUTPUT);      // <-- missing in your code
//   digitalWrite(TRIG_PIN, LOW);
//   digitalWrite(LED_PIN, LOW);
//   delay(100);
// }

// void loop() {
//   // Make sure ECHO is low before triggering (avoid catching leftovers)
//   unsigned long t0 = micros();
//   while (digitalRead(ECHO_PIN) == HIGH && (micros() - t0) < 30000UL) {}

//   // 10 us trigger pulse
//   digitalWrite(TRIG_PIN, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(TRIG_PIN, LOW);

//   // ~30 ms timeout ≈ ~5 m roundtrip
//   unsigned long us = pulseInLong(ECHO_PIN, HIGH, 30000UL);

//   if (us == 0) {
//     // timeout -> treat as no object; LED off
//     if (ledState) {
//       ledState = false;
//       digitalWrite(LED_PIN, LOW);
//     }
//     Serial.println("timeout (no echo)");
//   } else if (us < 150) {
//     // ignore sub-spec blips (<~1.3 cm)
//     Serial.printf("ignored short blip: %lu us\n", us);
//   } else {
//     float cm = (us * 0.0343f) * 0.5f;  // speed of sound 0.0343 cm/us, divide by 2
//     Serial.printf("echo=%lu us  dist=%.1f cm\n", us, cm);

//     // Hysteresis: on at <=10, off at >=12
//     if (!ledState && cm <= LED_ON_CM) {
//       ledState = true;
//       digitalWrite(LED_PIN, HIGH);
//     } else if (ledState && cm >= LED_OFF_CM) {
//       ledState = false;
//       digitalWrite(LED_PIN, LOW);
//     }
//   }

//   // Keep at least ~60 ms between pings
//   delay(80);
// }
