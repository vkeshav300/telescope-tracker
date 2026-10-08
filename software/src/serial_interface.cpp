#include "serial_interface.hpp"

#include <libserialport.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Serial {

int check(sp_return result) {
  if (result < 0)
    throw std::runtime_error("libserialport error: " +
                             std::to_string(static_cast<int>(result)));

  return static_cast<int>(result);
}

ports_list_t get_ports() {
  sp_port **ports_sp = nullptr;
  const sp_return result = sp_list_ports(&ports_sp);
  if (result < 0)
    throw std::runtime_error("failed to list ports");

  try {
    std::vector<Port_Info> ports;
    for (size_t i = 0; ports_sp[i] != nullptr; i++) {
      const char *name = sp_get_port_name(ports_sp[i]);
      const char *desc = sp_get_port_description(ports_sp[i]);
      ports.emplace_back(Port_Info{name, desc});
    }

    sp_free_port_list(ports_sp);
    return ports;
  } catch (...) {
    sp_free_port_list(ports_sp);
    throw std::runtime_error("failed to create ports vector");
  }

  return {};
}

Port::Port(const std::string &name, const uint32_t baud_rate)
    : m_baud_rate(baud_rate), m_name(name) {
  try {
    log("opening port");
    check(sp_get_port_by_name(name.c_str(), &m_handle));
    check(sp_open(m_handle, SP_MODE_READ_WRITE));
    m_open = true;

    log("configuring port");
    check(sp_set_baudrate(m_handle, baud_rate));

    check(sp_set_bits(m_handle,
                      8)); // Corresponds to SERIAL_8N1 in Serial.begin()
    check(sp_set_parity(m_handle, SP_PARITY_NONE));
    check(sp_set_stopbits(m_handle, 1));

    check(sp_set_flowcontrol(m_handle, SP_FLOWCONTROL_NONE));

    check(sp_set_dtr(m_handle, SP_DTR_OFF));
    check(sp_set_rts(m_handle, SP_RTS_OFF));

    /* Handshake */
    std::string pending;
    constexpr char MAX_ATTEMPTS = 10;
    for (char attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
      log("connecting to device (attempt " + std::to_string(attempt + 1) +
          "/10)");
      send("ping");
      wait(pending, "pong", 500);
      if (m_status == STATUS_OK || m_status == STATUS_ERR_COMMUNICATION)
        break;
    }

    if (m_status == STATUS_ERR_TIMEOUT) {
      change_status(STATUS_ERR_CONNECTION, "failed to connect");
    }
  } catch (...) {
    clean();
    throw;
  }
}

Port::~Port() { clean(); }

void Port::clean() {
  if (m_open)
    sp_close(m_handle);

  if (m_handle)
    sp_free_port(m_handle);
}

void Port::log(const std::string &msg) const {
  std::cout << "[port " << m_name << "] " << msg << "\n";
}

void Port::change_status(const std::pair<uint8_t, std::string> &result) {
  change_status(result.first, result.second);
}

void Port::change_status(const uint8_t status, const std::string &msg) {
  m_status = status;
  log("status changed: " + std::to_string(status) + " " + msg);
}

void Port::send(const std::string &cmd) {
  log("TX: " + cmd);
  const std::string msg = cmd + '\n';
  const int written =
      check(sp_blocking_write(m_handle, msg.data(), msg.size(), 1000));
  if (static_cast<std::size_t>(written) != msg.size()) {
    change_status(STATUS_ERR_INCOMPLETE_WRITE, "");
    throw std::runtime_error("Serial write timed out");
  }
}

void Port::wait(std::string &pending, const std::string &expected,
                const uint16_t timeout_ms) {
  using clock = std::chrono::steady_clock;
  const auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);

  for (;;) {
    for (;;) {
      const auto newline = pending.find('\n');
      if (newline == std::string::npos)
        break;

      std::string line = pending.substr(0, newline);
      pending.erase(0, newline + 1);

      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      log("RX: " + line);

      if (line == expected) {
        change_status(STATUS_OK, "");
        return;
      }

      if (line.starts_with("err ")) {
        change_status(STATUS_ERR_COMMUNICATION, line);
        return;
      }
    }

    const auto now = clock::now();
    if (now >= deadline) {
      change_status(STATUS_ERR_TIMEOUT,
                    "timed out waiting for \'" + expected + "\'");
      return;
    }

    const auto remaining =
        std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
    const auto read_timeout = static_cast<unsigned int>(
        std::min(remaining, std::chrono::milliseconds(100)).count());
    char buffer[128];
    const int received = check(
        sp_blocking_read_next(m_handle, buffer, sizeof(buffer), read_timeout));
    pending.append(buffer, static_cast<std::size_t>(received));
    if (pending.size() > 4096) {
      change_status(STATUS_ERR_COMMUNICATION,
                    "incoming Serial line is too long");
      return;
    }
  }
}

uint32_t Port::get_baud_rate() const { return m_baud_rate; }

std::string Port::get_name() const { return m_name; }

uint8_t Port::get_status() const { return m_status; }

bool Port::is_open() const { return m_open; }

bool Port::is_ok() const { return m_status == STATUS_OK; }

} // namespace Serial
