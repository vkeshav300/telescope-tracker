#include <Arduino.h>

#include <cstdint>

#include "stepper.hpp"

void setup_out_pin(const uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

namespace Stepper {

Motor::Motor(const uint8_t pin_step, const uint8_t pin_dir)
    : m_pin_step(pin_step), m_pin_dir(pin_dir) {
  setup_out_pin(m_pin_step);
  setup_out_pin(m_pin_dir);
}

Motor::Motor(const uint8_t pin_step, const uint8_t pin_dir,
             const uint8_t pin_ms1, const uint8_t pin_ms2)
    : m_pin_step(pin_step), m_pin_dir(pin_dir), m_pin_ms1(pin_ms1),
      m_pin_ms2(pin_ms2) {
  setup_out_pin(m_pin_step);
  setup_out_pin(m_pin_dir);
  setup_out_pin(m_pin_ms1);
  setup_out_pin(m_pin_ms2);
}

void Motor::configure_mstep(const uint8_t division) {
  switch (division) {
  case MSTEP_8:
    digitalWrite(m_pin_ms1, LOW);
    digitalWrite(m_pin_ms2, LOW);
    break;

  case MSTEP_16:
    digitalWrite(m_pin_ms1, HIGH);
    digitalWrite(m_pin_ms2, HIGH);
    break;

  case MSTEP_32:
    digitalWrite(m_pin_ms1, HIGH);
    digitalWrite(m_pin_ms2, LOW);
    break;

  case MSTEP_64:
    digitalWrite(m_pin_ms1, LOW);
    digitalWrite(m_pin_ms2, HIGH);
    break;

  default:
    break;
  }
}

void Motor::step(const uint16_t steps, const bool dir) {
  digitalWrite(m_pin_dir, dir);
  delay(250);

  for (uint16_t step = 0; step < steps; step++) {
    digitalWrite(m_pin_step, HIGH);
    delayMicroseconds(m_pulse_us);
    digitalWrite(m_pin_step, LOW);
    delayMicroseconds(m_delay_us);
  }
}

void Motor::test() {
  constexpr uint16_t STEPS_PER_DIR = 400;
  step(STEPS_PER_DIR, DIR_CW);
  delay(1000);
  step(STEPS_PER_DIR, DIR_CCW);
}

} // namespace Stepper
