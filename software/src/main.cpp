#include <exception>
#include <iostream>
#include <sstream>
#include <string>

#include "serial_interface.hpp"

int main() {
  Serial::ports_list_t ports;
  try {
    ports = Serial::get_ports();
  } catch (const std::exception &err) {
    std::cerr << "ports fetch request failed: " << err.what() << "\n";
    return 1;
  }

  if (ports.empty()) {
    std::cerr << "no ports listed\n";
    return 1;
  }

  std::cout << "select port:\n";
  for (size_t i = 0; i < ports.size(); i++)
    std::cout << "\t[" << i << "] " << ports[i].name << " (" << ports[i].desc
              << ")\n";

  size_t selected_port = ports.size();
  for (;;) {
    std::cout << "\nenter port number: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
      std::cerr << "port selection canceled or input unavailable\n";
      return 1;
    }

    std::istringstream selection(input);
    long long port_number = 0;
    if (selection >> port_number && (selection >> std::ws).eof() &&
        port_number >= 0 &&
        static_cast<unsigned long long>(port_number) < ports.size()) {
      selected_port = static_cast<size_t>(port_number);
      break;
    }

    std::cerr << "invalid port number\n";
  }

  try {
    Serial::Port port(ports[selected_port].name, 115200);
    if (!port.is_ok()) {
      std::cerr << "port is not OK after initialization\n";
      return 1;
    }
  } catch (const std::exception &err) {
    std::cerr << "port initialization failed: " << err.what() << "\n";
    return 1;
  }

  return 0;
}
