#include <Arduino.h>

#include "stepper.hpp"

static Stepper::Motor stepper(23, 22, 19, 18);

namespace io {

void display_cmds() {
  Serial.print("\nCommands:\n\t[1]\tTest stepper motor\n\t[2]\tCustom stepper "
               "test\n\t[3]\tSwitch microstepping modes\n\n > ");
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
      stepper.step(steps, Stepper::DIR_CW);
    else
      stepper.step(-steps, Stepper::DIR_CCW);

    Serial.print("Finshied\n");

    break;
  }

  case 3: {
    static uint8_t mode;
    switch (mode) {
    case Stepper::MSTEP_8:
      mode = Stepper::MSTEP_16;
      Serial.print("\nMode: MS16\n");
      break;

    case Stepper::MSTEP_16:
      mode = Stepper::MSTEP_32;
      Serial.print("\nMode: MS32\n");
      break;

    case Stepper::MSTEP_32:
      mode = Stepper::MSTEP_64;
      Serial.print("\nMode: MS64\n");
      break;

    default:
      mode = Stepper::MSTEP_8;
      Serial.print("\nMode: MS8\n");
      break;
    }

    stepper.configure_mstep(mode);
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
