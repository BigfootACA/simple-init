cmake_minimum_required(VERSION 3.6)
project(simple-init LANGUAGES C)

add_definitions("-Wall -Wextra -Wno-frame-address -rdynamic")
file(GLOB SRCS src/*/*.c)
include_directories(include)
add_executable(init src/main.c ${SRCS})
set(DEPENDS libkmod mount blkid readline)

option(DISABLE_WERROR "Avoid treating compiler warnings as fatal errors")
option(DISABLE_DEBUG  "Disable debug infomation generate")
option(ENABLE_STRIP   "Strip target")
option(ENABLE_STATIC  "Enable static link")
option(ENABLE_PIE     "Enable Position-Independent-Executable")
option(ENABLE_ASAN    "Enable Address Sanitizer")

if("${DISABLE_WERROR}" STREQUAL "OFF")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

if("${DISABLE_DEBUG}" STREQUAL "OFF")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()

if(NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    message(FATAL_ERROR "Need GNU/Linux and GCC")
endif()

if("${ENABLE_PIE}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie")
endif()

if("${ENABLE_ASAN}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize-recover=all -fno-omit-frame-pointer -fno-stack-protector -fsanitize=leak")
    target_link_libraries(init asan)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDS})
if("${ENABLE_STATIC}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
    target_link_libraries(init ${DEPS_STATIC_LIBRARIES})
else()
    target_link_libraries(init ${DEPS_LIBRARIES})
endif()

if("${ENABLE_STRIP}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
endif()
