#include <Arduino.h>

constexpr char TEST_LED_PIN = 23;

namespace io {

void display_cmds() {
  Serial.print("\nCommands:\n\t[1]\tStart\n\t[2]\tStop\n\n > ");
}

void poll() {
  if (!Serial.available())
    return;

  int input = Serial.parseInt();
  Serial.print(String(input));
  switch (input) {
  case 1:
    Serial.print("\nStarting...\n");
    digitalWrite(TEST_LED_PIN, 1);
    break;

  case 2:
    Serial.print("\nStopping ...\n");
    digitalWrite(TEST_LED_PIN, 0);
    break;

  default:
    Serial.print(" (ignoring invalid input)\n");
    break;
  }

  display_cmds();
}

}; // namespace io

void setup() {
  Serial.begin(115200);
  pinMode(TEST_LED_PIN, OUTPUT);
}

void loop() {
  io::poll();
  delay(100);
}
