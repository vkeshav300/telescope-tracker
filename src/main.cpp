#include <Arduino.h>

#include "stepper.hpp"

static Stepper::Motor stepper(23, 22, 19, 18, 0.9f);

namespace io {

void display_cmds() {
  Serial.print(
      "\nCommands:\n\t[1]\tCustom stepper revolve\n\t[2]\tCustom stepper "
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
  case 1: {
    Serial.print("\nEnter degrees (+ for CW, - for CCW) > ");
    wait_until_available();
    const float deg = Serial.parseFloat();
    Serial.print(String(deg) + "\nStarting...\n");

    float lost;
    if (deg > 0)
      lost = stepper.revolve(deg, Stepper::DIR_CW);
    else
      lost = stepper.revolve(-deg, Stepper::DIR_CCW);

    Serial.print("Finshied (approximately " + String(lost, 6) +
                 " degrees lost)\n");

    break;
  }

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
    Serial.print("\nEnter microstepping division > ");
    wait_until_available();
    const int division = Serial.parseInt();
    Serial.print(String(division));

    const bool err_result = stepper.configure_mstep(division);
    if (!err_result)
      Serial.print("\nSuccess\n");
    else
      Serial.print("\nInvalid division\n");

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
