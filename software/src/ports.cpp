#include "ports.hpp"

#include <libserialport.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

inline bool matches(const std::string &line, const std::string &expected) {
  return line == expected ||
         (line.size() > expected.size() && line.starts_with(expected) &&
          line[expected.size()] == ' ');
}

std::size_t check(sp_return result) {
  if (result < 0)
    throw std::runtime_error("err (libserialport): " +
                             std::to_string(static_cast<int>(result)));

  return static_cast<std::size_t>(result);
}

namespace serial {

port_list_t get_ports() {
  sp_port **ports_sp = nullptr;
  const sp_return result = sp_list_ports(&ports_sp);
  if (result < 0)
    throw std::runtime_error("err: failed to list ports");

  try {
    std::vector<port_info> ports;
    for (std::size_t i = 0; ports_sp[i] != nullptr; i++) {
      const char *name = sp_get_port_name(ports_sp[i]);
      const char *desc = sp_get_port_description(ports_sp[i]);
      ports.emplace_back(port_info{name, desc});
    }

    sp_free_port_list(ports_sp);
    return ports;
  } catch (...) {
    sp_free_port_list(ports_sp);
    throw std::runtime_error("err: failed to create ports list");
  }

  return {};
}

void port::clean() {
  if (m_handle) {
    if (m_open.load()) {
      if (m_connected.load()) {
        send("disconnect");
        wait(std::chrono::milliseconds(100));
        m_connected.exchange(false);
      }

      sp_close(m_handle);
      m_open.exchange(false);
    }

    sp_free_port(m_handle);
    m_handle = nullptr;
  }

  m_initialized.exchange(false);
}

uint8_t port::send_impl(const std::string &cmd) {
  m_log.add_entry("TX: " + cmd);
  const std::string msg = cmd + '\n';

  std::size_t written;
  try {
    written = check(sp_blocking_write(m_handle, msg.data(), msg.size(), 1000));
  } catch (const std::exception &err) {
    m_log.add_entry(std::string("err (serial write): ") + err.what());
    return RESULT_ERR_MISC;
  }

  if (written != msg.size()) {
    m_log.add_entry("err: incorrect serial write");
    return RESULT_ERR_MISC;
  }

  return RESULT_OK;
}

uint8_t port::wait_impl(std::string &pending, const std::string &expected,
                        const std::chrono::milliseconds &timeout_ms) {
  const auto deadline = std::chrono::steady_clock::now() + timeout_ms;

  for (;;) {
    for (;;) {
      const auto newline = pending.find('\n');
      if (newline == std::string::npos)
        break;

      std::string line = pending.substr(0, newline);
      pending.erase(0, newline + 1);

      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      m_log.add_entry("RX: " + line);

      if (m_pending_finish &&
          (matches(line, *m_pending_finish) || line.starts_with("err ")))
        m_pending_finish.reset();

      if (matches(line, expected))
        return RESULT_OK;

      if (line.starts_with("err"))
        return RESULT_ERR_INCOMING;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
      m_log.add_entry("err: timed out waiting for \'" + expected + "\'");
      return RESULT_ERR_TIMEOUT;
    }

    const auto remaining =
        std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
    const unsigned int read_timeout = static_cast<unsigned int>(
        std::min(remaining, std::chrono::milliseconds(100)).count());
    char buffer[1280];

    std::size_t received;
    try {
      received = check(sp_blocking_read_next(m_handle, buffer, sizeof(buffer),
                                             read_timeout));
    } catch (const std::exception &err) {
      m_log.add_entry(std::string("err (serial read): ") + err.what());
      return RESULT_ERR_MISC;
    }

    pending.append(buffer, received);
    if (pending.size() > 4096) {
      m_log.add_entry("err: incoming serial line is too long");
      return RESULT_ERR_INCOMING_OVERFLOW;
    }
  }
}

port::port(const std::string &name, const uint32_t baud_rate) {
  init(name, baud_rate);
}

port::~port() { clean(); }

uint8_t port::init(const std::string &name, const uint32_t baud_rate) {
  clean();

  std::unique_lock<std::mutex> lock(m_mtx, std::try_to_lock);
  if (!lock.owns_lock()) {
    m_log.add_entry("err: port is in-use");
    return RESULT_ERR_INFLIGHT;
  }

  if (name != m_name) {
    m_pending_finish.reset();
    m_pending_buff.clear();
  }

  m_name = name;
  m_baud_rate = baud_rate;
  m_log = log("port " + m_name);

  try {
    m_log.add_entry("opening port");
    check(sp_get_port_by_name(name.c_str(), &m_handle));
    check(sp_open(m_handle, SP_MODE_READ_WRITE));
    m_open.exchange(true);

    m_log.add_entry("configuring port");
    check(sp_set_baudrate(m_handle, m_baud_rate));

    check(sp_set_bits(m_handle,
                      8)); // Next 3 lines correspond to Serial_8N1
    check(sp_set_parity(m_handle, SP_PARITY_NONE));
    check(sp_set_stopbits(m_handle, 1));

    check(sp_set_flowcontrol(m_handle, SP_FLOWCONTROL_NONE));

    check(sp_set_dtr(m_handle, SP_DTR_OFF));
    check(sp_set_rts(m_handle, SP_RTS_OFF));

    constexpr uint8_t MAX_ATTEMPTS = 10;
    uint8_t attempt = 0;
    for (; attempt < MAX_ATTEMPTS; attempt++) {
      m_log.add_entry("connecting (attempt " + std::to_string(attempt) + "/" +
                      std::to_string(MAX_ATTEMPTS) + ")");
      uint8_t result = send_impl("ping");
      result |=
          wait_impl(m_pending_buff, "pong", std::chrono::milliseconds(500));
      if (result == RESULT_OK)
        break;
    }

    if (attempt == MAX_ATTEMPTS)
      throw std::runtime_error("err: failed to connect");

    m_connected.exchange(true);
  } catch (const std::exception &err) {
    m_log.add_entry(err.what());
    clean();
    return RESULT_ERR_MISC;
  }

  m_initialized.exchange(true);
  return 0;
}

uint8_t port::send(const std::string &cmd, const std::string &expected_prefix) {
  wait(std::chrono::milliseconds(100));

  std::unique_lock<std::mutex> lock(m_mtx, std::try_to_lock);
  if (!lock.owns_lock()) {
    m_log.add_entry("err: port is in-use");
    return RESULT_ERR_INFLIGHT;
  }

  if (!m_connected.load()) {
    m_log.add_entry("err: port not connected");
    return RESULT_ERR_MISC;
  }

  if (m_pending_finish) {
    m_log.add_entry("err: previous command completion is still pending");
    return RESULT_ERR_PENDING;
  }

  std::optional<std::string> finish = expected_prefix + cmd;
  const uint8_t result = send_impl(cmd);
  if (result == RESULT_OK && finish)
    m_pending_finish = std::move(finish);

  return result;
}

uint8_t port::wait(const std::chrono::milliseconds &timeout_ms) {
  std::unique_lock<std::mutex> lock(m_mtx, std::try_to_lock);
  if (!lock.owns_lock()) {
    m_log.add_entry("err: port is in-use");
    return RESULT_ERR_INFLIGHT;
  }

  if (!m_connected.load()) {
    m_log.add_entry("err: port not connected");
    return RESULT_ERR_MISC;
  }

  if (!m_pending_finish)
    return 0;

  const std::string expected = *m_pending_finish;
  return wait_impl(m_pending_buff, expected, timeout_ms);
}

bool port::available() {
  std::lock_guard<std::mutex> lock(m_mtx);
  return m_initialized && m_open && m_connected && !m_pending_finish;
}

bool port::connected() {
  std::lock_guard<std::mutex> lock(m_mtx);
  return m_initialized && m_open && m_connected;
}

} // namespace serial
