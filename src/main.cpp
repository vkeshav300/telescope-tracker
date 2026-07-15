#include <Arduino.h>

#include "stepper_motor.hpp"

static Stepper_Motor stepper(23, 22);

namespace io {

void display_cmds() {
  Serial.print("\nCommands:\n\t[1]\tTest stepper motor\n\t[2]\tCustom stepper "
               "test\n\n > ");
}

void wait_until_available() {
  while (!Serial.available())
    delay(50);
}

void poll() {
  wait_until_available();

  const int input = Serial.parseInt();
  Serial.print(String(input));
  switch (input) {
  case 1:
    Serial.print("\nTesting...\n");
    stepper.test();
    break;

  case 2: {
    Serial.print("\nEnter steps (+ for CW, - for CCW) > ");
    wait_until_available();
    const int steps = Serial.parseInt();
    Serial.print(String(steps) + "\nStarting...\n");

    if (steps > 0)
      stepper.step(steps, STEPPER_DIR::CW);
    else
      stepper.step(-steps, STEPPER_DIR::CCW);

    Serial.print("Finshied\n");

    break;
  }

  default:
    Serial.print(" (ignoring invalid input)\n");
    break;
  }

  display_cmds();
}

}; // namespace io

void setup() { Serial.begin(115200); }

void loop() { io::poll(); }
