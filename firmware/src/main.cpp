#include <Arduino.h>

#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "stepper.hpp"

static Stepper::Motor stepper(23, 22, 19, 18, 0.9f);

enum : uint8_t {
  RESPONSE_RECEIVE = 0,
  RESPONSE_FINISH,
  RESPONSE_ERR,
  RESPONSE_CUSTOM
};

void respond(const uint8_t status, const char *msg) {
  switch (status) {
  case RESPONSE_RECEIVE:
    Serial.print("receive ");
    Serial.println(msg);
    break;

  case RESPONSE_FINISH:
    Serial.print("finish ");
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
    respond(RESPONSE_RECEIVE, cmd);
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

    respond(RESPONSE_FINISH, (String(cmd) + " " + String(lost, 6)).c_str());
    return;
  }

  if (std::strncmp(cmd, "step ", 5) == 0) {
    respond(RESPONSE_RECEIVE, cmd);
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

    respond(RESPONSE_FINISH, cmd);
    return;
  }

  respond(RESPONSE_ERR, "unknown command");
}

void setup() { Serial.begin(115200, SERIAL_8N1); }

void loop() {
  static char line[64];
  static std::size_t used = 0;
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
