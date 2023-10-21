cmake_minimum_required(VERSION 3.10)
project(rsteg)
message("Setting up CMake for rsteg project.")
set(CMAKE_CXX_STANDARD 17)
set(SRC
    io_helpers.hpp
    aes_helpers.hpp
    lsb_rand.hpp
    rsteg.cpp
)

add_executable(rsteg ${SRC})
message("Creating executable 'rsteg'.")
set_target_properties(rsteg PROPERTIES OUTPUT_NAME "rsteg")
message("Setting the output name to 'rsteg'.")
find_package(OpenSSL REQUIRED)
find_package(PNG REQUIRED)
find_program(FFMPEG_EXECUTABLE ffmpeg REQUIRED)
message("-- Found FFmpeg: ${FFMPEG_EXECUTABLE}")
target_link_libraries(rsteg PRIVATE OpenSSL::SSL OpenSSL::Crypto PNG::PNG)

function(centered_message message)
    string(LENGTH "${message}" message_length)
    math(EXPR padding "(80 - ${message_length}) / 2")
    string(REPEAT "-" ${padding} left_padding)
    string(REPEAT "-" ${padding} right_padding)
    message("${left_padding} ${message} ${right_padding}")
endfunction()

if(NOT CMAKE_BUILD_TYPE)
    centered_message("Debug Build")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    centered_message("Release Build")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
endif()