find_program(PIO_COMMAND
    NAMES pio platformio
    REQUIRED
)

# Compile commands (for LSP)
set(PIO_COMPILE_COMMANDS "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json")
add_custom_target(export_compile_commands ALL
    COMMAND "${PIO_COMMAND}" run -t compiledb
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${PIO_COMPILE_COMMANDS}"
            "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json"
    COMMAND "${CMAKE_COMMAND}" -E remove -f "${PIO_COMPILE_COMMANDS}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Generating compile commands"
)

# Build firmware
add_custom_target(firmware
    COMMAND "${PIO_COMMAND}" run
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Building firmware"
)

add_dependencies(firmware export_compile_commands)

# Build and upload firmware)
add_custom_target(upload ALL
    COMMAND "${PIO_COMMAND}" run -t upload
    COMMAND "${PIO_COMMAND}" device monitor -b 115200
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Uploading firmware"
    DEPENDS firmware
)

add_dependencies(upload export_compile_commands)
