cmake_minimum_required(VERSION 3.16)

project(telescope_tracker
    LANGUAGES CXX
)

# CMake exports the desktop commands; PlatformIO exports the ESP32 commands.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_GENERATOR MATCHES "Makefiles|Ninja")
    message(FATAL_ERROR "Compile command export requires a Makefiles or Ninja generator.")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build: cmake -S . -B build")
endif()

# Keep firmware upload and serial monitoring explicit in the root build.
add_subdirectory(firmware EXCLUDE_FROM_ALL)
add_subdirectory(software)

# Do not overwrite CMake's desktop-only database in the build directory.
# Publish the combined database at the project root for LSP auto-discovery.
set(MERGED_COMPILE_COMMANDS "${PROJECT_BINARY_DIR}/lsp/compile_commands.json")
add_custom_target(export_compile_commands ALL
    COMMAND "${CMAKE_COMMAND}"
            "-DSOFTWARE_DATABASE=${PROJECT_BINARY_DIR}/compile_commands.json"
            "-DFIRMWARE_DATABASE=${PROJECT_BINARY_DIR}/firmware/compile_commands.json"
            "-DOUTPUT_DATABASE=${MERGED_COMPILE_COMMANDS}"
            -P "${PROJECT_SOURCE_DIR}/cmake/merge_compile_commands.cmake"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${MERGED_COMPILE_COMMANDS}"
            "${PROJECT_SOURCE_DIR}/compile_commands.json"
    BYPRODUCTS
        "${MERGED_COMPILE_COMMANDS}"
        "${PROJECT_SOURCE_DIR}/compile_commands.json"
    VERBATIM
    COMMENT "Exporting firmware and software compile commands for the LSP"
)
add_dependencies(export_compile_commands export_firmware_compile_commands)

# Keep individual builds independent: the combined export invokes PlatformIO.
# Only the combined build needs both targets and their merged LSP database.
add_custom_target(full DEPENDS firmware software export_compile_commands)
