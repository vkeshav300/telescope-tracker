#include <Arduino.h>

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "stepper.hpp"

static Stepper::Motor stepper(23, 22, 19, 18, 0.9f);

// namespace io {
//
// void display_cmds() {
// Serial.print(
// "\nCommands:\n\t[1]\tCustom stepper revolve\n\t[2]\tCustom stepper "
// "test\n\t[3]\tSwitch microstepping modes\n\n > ");
// }
//
// void wait_until_available() {
// while (!Serial.available())
// delay(50);
// }
//
// void poll() {
// wait_until_available();
//
// const int input = Serial.parseInt();
// Serial.print(String(input));
// switch (input) {
// case 1: {
// Serial.print("\nEnter deg (+ for CW, - for CCW) > ");
// wait_until_available();
// const float deg = Serial.parseFloat();
// Serial.print(String(deg) + "\nStarting...\n");
//
// float lost;
// if (deg > 0)
// lost = stepper.revolve(deg, Stepper::DIR_CW);
// else
// lost = stepper.revolve(-deg, Stepper::DIR_CCW);
//
// Serial.print("Finshied (approximately " + String(lost, 6) +
// " deg lost)\n");
//
// break;
// }
//
// case 2: {
// Serial.print("\nEnter steps (+ for CW, - for CCW) > ");
// wait_until_available();
// const int steps = Serial.parseInt();
// Serial.print(String(steps) + "\nStarting...\n");
//
// if (steps > 0)
// stepper.step(steps, Stepper::DIR_CW);
// else
// stepper.step(-steps, Stepper::DIR_CCW);
//
// Serial.print("Finshied\n");
//
// break;
// }
//
// case 3: {
// Serial.print("\nEnter microstepping division > ");
// wait_until_available();
// const int division = Serial.parseInt();
// Serial.print(String(division));
//
// const bool err_result = stepper.configure_mstep(division);
// if (!err_result)
// Serial.print("\nSuccess\n");
// else
// Serial.print("\nInvalid division\n");
//
// break;
// }
//
// default:
// Serial.print(" (ignoring invalid input)\n");
// break;
// }
//
// display_cmds();
// }
//
// }; // namespace io

enum : uint8_t { RESPONSE_OK = 0, RESPONSE_ERR, RESPONSE_CUSTOM };

void respond(const uint8_t status, const char *msg) {
  switch (status) {
  case RESPONSE_OK:
    Serial.print("ok ");
    Serial.println(msg);
    break;

  case RESPONSE_ERR:
    Serial.print("err ");
    Serial.println(msg);
    break;

  case RESPONSE_CUSTOM:
    Serial.println(msg);
    break;

  default:
    Serial.print("err unknown response status: ");
    Serial.println(msg);
    break;
  }
}

void process_cmd(const char *cmd) {
  if (std::strcmp(cmd, "ping") == 0) {
    respond(RESPONSE_CUSTOM, "pong");
    return;
  }

  if (std::strncmp(cmd, "revolve ", 8) == 0) {
    const char *arg = cmd + 8;
    char *end = nullptr;

    errno = 0;
    const double deg = std::strtod(arg, &end);
    constexpr double MAX_DEG = 360.0;

    if (end == arg || *end != '\0' || errno == ERANGE || !std::isfinite(deg) ||
        std::abs(deg) > MAX_DEG) {
      respond(RESPONSE_ERR, "invalid rotation angle");
      return;
    }

    const double lost = stepper.revolve(
        std::abs(deg), deg < 0 ? Stepper::DIR_CCW : Stepper::DIR_CW);
    const String msg = "lost " + String(lost, 6);
    respond(RESPONSE_OK, msg.c_str());
    return;
  }

  if (std::strncmp(cmd, "step ", 5) == 0) {
    const char *arg = cmd + 5;
    char *end = nullptr;

    errno = 0;
    const long long steps = std::strtoll(arg, &end, 10);
    constexpr long long MAX_STEPS = std::numeric_limits<uint32_t>::max();

    if (end == arg || *end != '\0' || errno == ERANGE || steps < -MAX_STEPS ||
        steps > MAX_STEPS) {
      respond(RESPONSE_ERR, "invalid step count");
      return;
    }

    stepper.step(static_cast<uint32_t>(steps < 0 ? -steps : steps),
                 steps < 0 ? Stepper::DIR_CCW : Stepper::DIR_CW);

    respond(RESPONSE_OK, "");
    return;
  }

  respond(RESPONSE_ERR, "unknown command");
}

void setup() { Serial.begin(115200, SERIAL_8N1); }

void loop() {
  static char line[64];
  static size_t used = 0;
  static bool overflow = false;

  while (Serial.available() > 0) {
    const int value = Serial.read();
    if (value < 0)
      break;

    const char c = static_cast<char>(value);
    if (c == '\r')
      continue;

    if (c == '\n') {
      if (overflow)
        respond(RESPONSE_ERR, "line too long");
      else if (used > 0) {
        line[used] = '\0';
        process_cmd(line);
      }

      used = 0;
      overflow = false;
    } else if (!overflow) {
      if (used < sizeof(line) - 1)
        line[used++] = c;
      else
        overflow = true;
    }
  }

  delay(1);
}
