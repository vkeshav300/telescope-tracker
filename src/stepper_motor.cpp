#include <Arduino.h>

#include <cstdint>

#include "stepper_motor.hpp"

Stepper_Motor::Stepper_Motor(const uint8_t pin_step, const uint8_t pin_dir)
    : m_pin_step(pin_step), m_pin_dir(pin_dir) {
  pinMode(m_pin_step, OUTPUT);
  pinMode(m_pin_dir, OUTPUT);
  digitalWrite(m_pin_step, LOW);
  digitalWrite(m_pin_dir, LOW);
}

void Stepper_Motor::step(const uint16_t steps, const bool dir) {
  digitalWrite(m_pin_dir, dir);
  delay(250);

  for (uint16_t step = 0; step < steps; ++step) {
    digitalWrite(m_pin_step, HIGH);
    delayMicroseconds(m_pulse_us);
    digitalWrite(m_pin_step, LOW);
    delayMicroseconds(m_delay_us);
  }
}

void Stepper_Motor::test() {
  constexpr uint16_t STEPS_PER_DIR = 400;
  step(STEPS_PER_DIR, STEPPER_DIR::CW);
  delay(1000);
  step(STEPS_PER_DIR, STEPPER_DIR::CCW);
}
