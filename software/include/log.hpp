#pragma once

#include <cstddef>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>

namespace Serial {

class Log {
private:
  // Synchronize the redirected stream itself, including direct cout writes.
  class Buffer : public std::streambuf {
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

  static Buffer m_buffer;
  static std::mutex m_lifecycle_mutex;
  static std::streambuf *m_original;
  static std::size_t instances;

  std::string m_identifier;

public:
  Log(const std::string &identifier);
  Log(const Log &) = delete;
  ~Log();

  Log &operator=(const Log &) = delete;

  std::string text() const;

  void clear();

  void add_entry(const std::string &entry);
};

} // namespace Serial
