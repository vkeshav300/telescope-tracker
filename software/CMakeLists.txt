enable_language(C)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.5.1
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw imgui)

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

add_library(dear_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)

add_library(dear_imgui::dear_imgui ALIAS dear_imgui)

target_include_directories(dear_imgui
    PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
)

target_compile_features(dear_imgui PUBLIC cxx_std_20)
target_link_libraries(dear_imgui PUBLIC glfw OpenGL::GL)

if(APPLE)
    target_compile_definitions(dear_imgui PRIVATE GL_SILENCE_DEPRECATION)
endif()

# Prefer pkg-config when available, but do not require it. The fallback
# supports Homebrew's Apple Silicon and Intel prefixes as well as standard
# Unix installation locations and a user-provided LIBSERIALPORT_ROOT.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBSERIALPORT QUIET IMPORTED_TARGET libserialport)
endif()

if(TARGET PkgConfig::LIBSERIALPORT)
    add_library(libserialport::libserialport ALIAS PkgConfig::LIBSERIALPORT)
else()
    find_path(LIBSERIALPORT_INCLUDE_DIR
        NAMES libserialport.h
        HINTS ENV LIBSERIALPORT_ROOT
        PATHS /opt/homebrew /usr/local
        PATH_SUFFIXES include
    )

    find_library(LIBSERIALPORT_LIBRARY
        NAMES serialport libserialport
        HINTS ENV LIBSERIALPORT_ROOT
        PATHS /opt/homebrew /usr/local
        PATH_SUFFIXES lib
    )

    if(NOT LIBSERIALPORT_INCLUDE_DIR OR NOT LIBSERIALPORT_LIBRARY)
        message(FATAL_ERROR
            "libserialport is required but was not found. "
            "Install it on macOS with `brew install libserialport`, or set "
            "LIBSERIALPORT_ROOT to its installation prefix."
        )
    endif()

    add_library(libserialport::libserialport UNKNOWN IMPORTED)
    set_target_properties(libserialport::libserialport PROPERTIES
        IMPORTED_LOCATION "${LIBSERIALPORT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSERIALPORT_INCLUDE_DIR}"
    )
endif()

# Recheck these patterns during builds so added or removed files trigger a
# CMake reconfiguration. Include files in subdirectories as well.
file(GLOB_RECURSE TELESCOPE_SOFTWARE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE TELESCOPE_SOFTWARE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_executable(telescope_tracker_software
    ${TELESCOPE_SOFTWARE_SOURCES}
    ${TELESCOPE_SOFTWARE_HEADERS}
)

target_include_directories(telescope_tracker_software
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(telescope_tracker_software PRIVATE cxx_std_20)
set_target_properties(telescope_tracker_software PROPERTIES
    CXX_EXTENSIONS OFF
)

if(APPLE)
    target_compile_definitions(telescope_tracker_software
        PRIVATE GL_SILENCE_DEPRECATION
    )
endif()

target_link_libraries(telescope_tracker_software
    PRIVATE
        dear_imgui::dear_imgui
        libserialport::libserialport
        Threads::Threads
)

add_custom_target(software
    DEPENDS telescope_tracker_software
)
