#include <Arduino.h>

#include "stepper_motor.hpp"

static Stepper_Motor stepper(23, 22);

namespace io {

void display_cmds() {
  Serial.print("\nCommands:\n\t[1]\tTest stepper motor\n\n > ");
}

void poll() {
  if (!Serial.available())
    return;

  int input = Serial.parseInt();
  Serial.print(String(input));
  switch (input) {
  case 1:
    Serial.print("\nTesting...\n");
    stepper.test();
    break;

  default:
    Serial.print(" (ignoring invalid input)\n");
    break;
  }

  display_cmds();
}

}; // namespace io

void setup() { Serial.begin(115200); }

void loop() {
  io::poll();
  delay(100);
}
