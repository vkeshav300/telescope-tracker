#include "log.hpp"

#include <cstddef>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>

namespace serial {

log::buffer log::m_buffer;
std::mutex log::m_lifecycle_mutex;
std::streambuf *log::m_original = nullptr;
std::size_t log::instances = 0;

std::streamsize log::buffer::xsputn(const char *text, std::streamsize count) {
  if (count > 0) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_text.append(text, static_cast<std::size_t>(count));
  }
  return count;
}

log::buffer::int_type log::buffer::overflow(int_type ch) {
  if (!traits_type::eq_int_type(ch, traits_type::eof())) {
    const char character = traits_type::to_char_type(ch);
    xsputn(&character, 1);
  }
  return traits_type::not_eof(ch);
}

void log::buffer::append(const std::string &text) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_text += text;
}

std::string log::buffer::text() const {
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_text;
}

void log::buffer::clear() {
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_text.clear();
}

log::log(const std::string &identifier) : m_identifier(identifier) {
  const std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
  if (instances == 0)
    m_original = std::cout.rdbuf(&m_buffer);

  instances++;
}

log::~log() {
  const std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
  if (instances == 1)
    std::cout.rdbuf(m_original);

  instances--;
}

std::string log::text() const { return m_buffer.text(); }

void log::clear() { m_buffer.clear(); }

void log::add_entry(const std::string &entry) {
  // Append a whole entry atomically so messages from different threads do
  // not interleave. cout still uses the same synchronized buffer.
  m_buffer.append("[" + m_identifier + "] " + entry + "\n");
}

} // namespace serial
