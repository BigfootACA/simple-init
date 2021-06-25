cmake_minimum_required(VERSION 3.6)
project(simple-init LANGUAGES C)

include_directories(include lvgl)
add_definitions("-Wall -Wextra -Wno-frame-address -rdynamic")

option(DISABLE_WERROR     "Avoid treating compiler warnings as fatal errors"  OFF)
option(DISABLE_DEBUG      "Disable debug infomation generate"                 OFF)
option(ENABLE_STRIP       "Strip target"                                      OFF)
option(ENABLE_STATIC      "Enable static link"                                OFF)
option(ENABLE_PIE         "Enable Position-Independent-Executable"            OFF)
option(ENABLE_ASAN        "Enable Address Sanitizer"                          OFF)

option(ENABLE_INITSHELL   "Enable builtin shell"                              ON)
option(ENABLE_DRM         "Enable DRM"                                        ON)
option(ENABLE_GTK         "Enable GTK"                                        OFF)
option(ENABLE_GUI         "Enable LVGL based GUI"                             ON)
option(ENABLE_KMOD        "Enable kmod modules load or remove"                ON)
option(ENABLE_BLKID       "Enable blkid block detect and evalaute tag"        ON)
option(ENABLE_TERMINFO    "Enable builtin assets terminfo"                    ON)

if("${ENABLE_INITSHELL}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_INITSHELL=1")
endif()

if("${ENABLE_DRM}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_DRM=1")
endif()

if("${ENABLE_GTK}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_GTK=1")
endif()

if("${ENABLE_GUI}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_GUI=1 -DLV_CONF_PATH=\"lv_conf.h\"")
    file(GLOB_RECURSE SOURCES lvgl/src/*.c)
    add_library(lvgl STATIC ${SOURCES})
endif()

if("${ENABLE_KMOD}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_KMOD=1")
else()
    message(WARNING "libkmod disabled, kernel modules will not be loaded")
endif()

if("${ENABLE_BLKID}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_BLKID=1")
else()
    message(WARNING "blkid disabled, may not find block devices by tag")
endif()

if("${ENABLE_TERMINFO}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_TERMINFO=1")
endif()

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
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_ASAN=1 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize-recover=all -fno-omit-frame-pointer -fno-stack-protector -fsanitize=leak")
endif()

if("${ENABLE_STATIC}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
endif()

if("${ENABLE_STRIP}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
endif()

include(src/CMakeLists.txt)
