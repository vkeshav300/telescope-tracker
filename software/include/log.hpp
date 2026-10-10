#pragma once

#include <cstddef>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>

namespace serial {

class log {
private:
  // Synchronize the redirected stream itself, including direct cout writes.
  class buffer : public std::streambuf {
  private:
    mutable std::mutex m_mutex;
    std::string m_text;

  protected:
    std::streamsize xsputn(const char *text, std::streamsize count) override;
    int_type overflow(int_type ch) override;

  public:
    void append(const std::string &text);
    std::string text() const;
    void clear();
  };

  static buffer m_buffer;
  static std::mutex m_lifecycle_mutex;
  static std::streambuf *m_original;
  static std::size_t instances;

  std::string m_identifier;

public:
  log(const std::string &identifier);
  log(const log &) = delete;
  ~log();

  std::string text() const;

  void clear();

  void add_entry(const std::string &entry);
};

} // namespace serial
