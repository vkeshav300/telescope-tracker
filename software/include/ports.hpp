#pragma once

#include "log.hpp"

#include <libserialport.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace serial {

struct port_info {
  std::string name, desc;
};

using port_list_t = std::vector<port_info>;

enum : uint8_t {
  STATUS_UNINITIALIZED = 1,
  STATUS_OPEN = 2,
  STATUS_NOT_CONNECTED = 4,
  STATUS_CONNECTED = 8
};

enum : uint8_t {
  RESULT_OK = 0,
  RESULT_ERR_TIMEOUT,
  RESULT_ERR_INCOMING,
  RESULT_ERR_INCOMING_OVERFLOW,
  RESULT_ERR_INFLIGHT,
  RESULT_ERR_MISC,
  RESULT_ERR_PENDING
};

port_list_t get_ports();

class port {
private:
  sp_port *m_handle = nullptr;
  uint32_t m_baud_rate = 0;
  std::string m_name;

  std::atomic<bool> m_initialized = false, m_open = false, m_connected = false;
  std::optional<std::string> m_pending_finish;
  std::string m_pending_buff;

  std::mutex m_mtx;

  log m_log = log("port uninitialized");

  void clean();

  uint8_t send_impl(const std::string &cmd);
  uint8_t wait_impl(std::string &pending, const std::string &expected,
                    const std::chrono::milliseconds &timeout_ms);

public:
  port() = default;
  port(const std::string &name, const uint32_t baud_rate);
  port(const port &) = delete;
  port &operator=(const port &) = delete;
  ~port();

  uint8_t init(const std::string &name, const uint32_t baud_rate);
  uint8_t send(const std::string &cmd,
               const std::string &expected_prefix = "finish ");
  uint8_t wait(const std::chrono::milliseconds &timeout_ms);

  bool available();
  bool connected();
};

} // namespace serial
