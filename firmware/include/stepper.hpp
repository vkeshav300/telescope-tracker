#pragma once

#include <cstdint>

namespace Stepper {

enum : bool { DIR_CCW = 0, DIR_CW = 1 };

class Motor {
private:
  uint8_t m_pin_step, m_pin_dir, m_pin_ms1, m_pin_ms2, m_division = 8,
                                                       m_pulse_us = 5;
  uint16_t m_delay_us = 2000;
  float m_step_angle;

public:
  Motor(const uint8_t pin_step, const uint8_t pin_dir);

  Motor(const uint8_t pin_step, const uint8_t pin_dir, const uint8_t pin_ms1,
        const uint8_t pin_ms2);

  Motor(const uint8_t pin_step, const uint8_t pin_dir, const uint8_t pin_ms1,
        const uint8_t pin_ms2, const float step_angle);

  bool configure_mstep(const uint8_t division);

  double lost(const double deg);

  void step(const uint32_t steps, const bool dir);

  double revolve(const double deg, const bool dir);
};

} // namespace Stepper
