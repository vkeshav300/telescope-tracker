#pragma once

#include "log.hpp"

#include <libserialport.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Serial {

int check(sp_return result);

enum : uint8_t {
  STATUS_OK = 0,
  STATUS_ERR_CONNECTION,
  STATUS_ERR_TIMEOUT,
  STATUS_ERR_COMMUNICATION,
  STATUS_ERR_INCOMPLETE_WRITE,
  STATUS_ERR_NOT_INITIALIZED
};

struct Port_Info {
  std::string name, desc;
};

using port_list_t = std::vector<Port_Info>;

port_list_t get_ports();

class Port {
private:
  sp_port *m_handle = nullptr;

  uint8_t m_status = STATUS_ERR_NOT_INITIALIZED;
  uint32_t m_baud_rate = 0;
  bool m_open = false;

  std::string m_name;
  std::optional<Log> m_log;

  void clean();

  void change_status(const std::pair<uint8_t, std::string> &result);
  void change_status(const uint8_t status, const std::string &msg);

public:
  Port() = default;
  Port(const std::string &name, const uint32_t baud_rate);
  Port(const Port &) = delete;
  ~Port();

  Port &operator=(const Port &) = delete;

  // Open, configure, and handshake; close any previously opened port first.
  void initialize(const std::string &name, const uint32_t baud_rate);

  void send(const std::string &cmd);
  void wait(std::string &pending, const std::string &expected,
            const uint16_t timeout_ms);

  uint32_t get_baud_rate() const;
  std::string get_name() const;
  uint8_t get_status() const;

  bool is_open() const;
  bool is_ok() const;
};

} // namespace Serial
