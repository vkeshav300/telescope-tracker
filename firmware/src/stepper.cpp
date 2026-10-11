#include <Arduino.h>

#include <cmath>
#include <cstdint>

#include "stepper.hpp"

void setup_out_pin(const uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

namespace stepper {

motor::motor(const uint8_t pin_step, const uint8_t pin_dir)
    : m_pin_step(pin_step), m_pin_dir(pin_dir) {
  setup_out_pin(m_pin_step);
  setup_out_pin(m_pin_dir);
}

motor::motor(const uint8_t pin_step, const uint8_t pin_dir,
             const uint8_t pin_ms1, const uint8_t pin_ms2)
    : motor(pin_step, pin_dir) {
  m_pin_ms1 = pin_ms1;
  m_pin_ms2 = pin_ms2;
  setup_out_pin(m_pin_ms1);
  setup_out_pin(m_pin_ms2);
}

motor::motor(const uint8_t pin_step, const uint8_t pin_dir,
             const uint8_t pin_ms1, const uint8_t pin_ms2,
             const float step_angle)
    : motor(pin_step, pin_dir, pin_ms1, pin_ms2) {
  m_step_angle = step_angle;
}

bool motor::configure_mstep(const uint8_t division) {
  switch (division) { // TMC2209
  case 8:
    digitalWrite(m_pin_ms1, LOW);
    digitalWrite(m_pin_ms2, LOW);
    break;

  case 16:
    digitalWrite(m_pin_ms1, HIGH);
    digitalWrite(m_pin_ms2, HIGH);
    break;

  case 32:
    digitalWrite(m_pin_ms1, HIGH);
    digitalWrite(m_pin_ms2, LOW);
    break;

  case 64:
    digitalWrite(m_pin_ms1, LOW);
    digitalWrite(m_pin_ms2, HIGH);
    break;

  default:
    return 1;
  }

  m_division = division;
  return 0;
}

double motor::lost(const double deg) {
  const double resolution = static_cast<double>(m_step_angle) / m_division;
  const uint32_t steps = static_cast<uint32_t>(std::floor(deg / resolution));
  return deg - steps * resolution;
}

void motor::step(const uint32_t steps, const bool dir) {
  digitalWrite(m_pin_dir, dir);
  delay(250);

  for (uint32_t step = 0; step < steps; step++) {
    digitalWrite(m_pin_step, HIGH);
    delayMicroseconds(m_pulse_us);
    digitalWrite(m_pin_step, LOW);
    delayMicroseconds(m_delay_us);
  }
}

double motor::revolve(const double deg, const bool dir) {
  step(static_cast<uint32_t>(deg * m_division / m_step_angle), dir);
  return lost(deg);
}

} // namespace stepper
