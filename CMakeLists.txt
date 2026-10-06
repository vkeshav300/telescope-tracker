cmake_minimum_required(VERSION 3.16)

project(telescope_tracker
    LANGUAGES CXX
)

# Keep firmware upload and serial monitoring explicit in the root build.
add_subdirectory(firmware EXCLUDE_FROM_ALL)
add_subdirectory(software)
