# Both generators produce JSON arrays. Join their contents without changing
# compiler flags or requiring a JSON parser newer than CMake 3.16.
foreach(variable IN ITEMS SOFTWARE_DATABASE FIRMWARE_DATABASE OUTPUT_DATABASE)
    if(NOT DEFINED ${variable} OR "${${variable}}" STREQUAL "")
        message(FATAL_ERROR "${variable} must be provided.")
    endif()
endforeach()

set(merged_entries "")
foreach(database IN ITEMS "${SOFTWARE_DATABASE}" "${FIRMWARE_DATABASE}")
    file(READ "${database}" entries)
    string(STRIP "${entries}" entries)
    if(NOT entries MATCHES "^\\[.*\\]$")
        message(FATAL_ERROR "Expected a compilation database JSON array: ${database}")
    endif()

    string(REGEX REPLACE "^\\[" "" entries "${entries}")
    string(REGEX REPLACE "\\]$" "" entries "${entries}")
    string(STRIP "${entries}" entries)
    if(NOT entries STREQUAL "")
        if(NOT merged_entries STREQUAL "")
            string(APPEND merged_entries ",\n")
        endif()
        string(APPEND merged_entries "${entries}")
    endif()
endforeach()

# Keep the timestamp stable when commands have not changed, avoiding needless
# LSP re-indexing on every build.
get_filename_component(output_directory "${OUTPUT_DATABASE}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")
file(WRITE "${OUTPUT_DATABASE}.tmp" "[\n${merged_entries}\n]\n")
configure_file("${OUTPUT_DATABASE}.tmp" "${OUTPUT_DATABASE}" COPYONLY)
file(REMOVE "${OUTPUT_DATABASE}.tmp")
