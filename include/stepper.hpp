#pragma once

#include <cstdint>

namespace Stepper {

enum : bool { DIR_CCW = 0, DIR_CW = 1 };

enum : uint8_t { MSTEP_8, MSTEP_16, MSTEP_32, MSTEP_64 };

class Motor {
private:
  uint8_t m_pin_step, m_pin_dir, m_pin_ms1, m_pin_ms2, m_pulse_us = 5;
  uint16_t m_delay_us = 2000;

public:
  Motor(const uint8_t pin_step, const uint8_t pin_dir);
  Motor(const uint8_t pin_step, const uint8_t pin_dir, const uint8_t pin_ms1,
        const uint8_t pin_ms2);

  void configure_mstep(const uint8_t division);

  void step(const uint16_t steps, const bool dir);

  void test();
};

} // namespace Stepper
