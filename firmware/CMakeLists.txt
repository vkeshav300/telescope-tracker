find_program(PIO_COMMAND
    NAMES pio platformio
    REQUIRED
)

# Compile commands (for LSP)
set(PIO_COMPILE_COMMANDS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
add_custom_target(export_firmware_compile_commands
    COMMAND "${CMAKE_COMMAND}" -E env
            "TELESCOPE_COMPILE_COMMANDS_PATH=${PIO_COMPILE_COMMANDS}"
            "${PIO_COMMAND}" run -t compiledb
    BYPRODUCTS "${PIO_COMPILE_COMMANDS}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    VERBATIM
    COMMENT "Generating compile commands"
)

# Build firmware
add_custom_target(firmware
    COMMAND "${PIO_COMMAND}" run
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Building firmware"
)

add_dependencies(firmware export_firmware_compile_commands)

# Build and upload firmware)
add_custom_target(upload ALL
    COMMAND "${PIO_COMMAND}" run -t upload
    COMMAND "${PIO_COMMAND}" device monitor -b 115200
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Uploading firmware"
    DEPENDS firmware
)

add_dependencies(upload export_firmware_compile_commands)
