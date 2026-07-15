#pragma once

#include <cstdint>

enum STEPPER_DIR : bool { CW = false, CCW = true };

class Stepper_Motor {
private:
  uint8_t m_pin_step, m_pin_dir, m_pulse_us = 5;
  uint16_t m_delay_us = 2000;

public:
  Stepper_Motor(const uint8_t pin_step, const uint8_t pin_dir);

  void step(const uint16_t steps, const bool dir);

  void test();
};
