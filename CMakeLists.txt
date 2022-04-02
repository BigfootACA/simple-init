cmake_minimum_required(VERSION 3.6)
project(simple-init LANGUAGES C)

include_directories(include)
set(CONFIG_H "${CMAKE_CURRENT_BINARY_DIR}/config.h")
add_definitions(
	"-Wall "
	"-Wextra "
	"-Wno-frame-address "
	"-Wno-unused-parameter "
	"-Wno-format-truncation "
	"-Wno-stringop-truncation "
	"-fdiagnostics-color=always "
	"-rdynamic "
	"-include ${CONFIG_H}"
)

option(DISABLE_WERROR     "Avoid treating compiler warnings as fatal errors"  OFF)
option(DISABLE_DEBUG      "Disable debug infomation generate"                 OFF)
option(ENABLE_STRIP       "Strip target"                                      OFF)
option(ENABLE_STATIC      "Enable static link"                                OFF)
option(ENABLE_PIE         "Enable Position-Independent-Executable"            OFF)
option(ENABLE_ASAN        "Enable Address Sanitizer"                          OFF)

option(ENABLE_INITSHELL   "Enable builtin shell"                              ON)
option(ENABLE_DRM         "Enable DRM"                                        ON)
option(ENABLE_GTK         "Enable GTK"                                        OFF)
option(ENABLE_SDL2        "Enable SDL2"                                       OFF)
option(ENABLE_GUI         "Enable LVGL based GUI"                             ON)
option(ENABLE_UUID        "Enable UUID library"                               ON)
option(ENABLE_FREETYPE2   "Enable FreeType 2 for GUI fonts"                   ON)
option(ENABLE_LODEPNG     "Enable lodepng for GUI png decode"                 ON)
option(ENABLE_LIBJPEG     "Enable LIBJPEG for GUI jpeg decode"                ON)
option(ENABLE_NANOSVG     "Enable NanoSVG for GUI svg decode"                 ON)
option(ENABLE_FDISK       "Enable libfdisk for GUI partition tools"           ON)
option(ENABLE_KMOD        "Enable kmod modules load or remove"                ON)
option(ENABLE_BLKID       "Enable blkid block detect and evalaute tag"        ON)
option(ENABLE_JSONC       "Enable json-c for config files"                    ON)
option(ENABLE_MXML        "Enable mxml for config files"                      ON)
option(ENABLE_LUA         "Enable lua for scripts"                            ON)
option(ENABLE_STB         "Enable stb"                                        ON)
option(ENABLE_HIVEX       "Enable hivex for windows registry/bcd edit"        OFF)
option(ENABLE_VNCSERVER   "Enable VNC Server for remote GUI"                  OFF)
option(SYSTEM_FREETYPE2   "Use system FreeType 2 library"                     OFF)

configure_file(src/config.h.in "${CONFIG_H}")

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	message(FATAL_ERROR "Binary folder not specified")
endif()

if("${ENABLE_GUI}" STREQUAL "ON")
	include(libs/lvgl.cmake)
endif()

if("${ENABLE_FREETYPE2}" STREQUAL "ON")
	include(libs/freetype.cmake)
endif()

if("${ENABLE_LODEPNG}" STREQUAL "ON")
	include(libs/lodepng.cmake)
endif()

if("${ENABLE_NANOSVG}" STREQUAL "ON")
	include(libs/nanosvg.cmake)
endif()

if("${ENABLE_JSONC}" STREQUAL "ON")
	include(libs/json-c.cmake)
endif()

if("${ENABLE_MXML}" STREQUAL "ON")
	include(libs/mxml.cmake)
endif()

if("${ENABLE_LUA}" STREQUAL "ON")
	include(libs/lua.cmake)
endif()

if("${ENABLE_STB}" STREQUAL "ON")
	include_directories(libs/stb)
endif()

if("${ENABLE_KMOD}" STREQUAL "OFF")
	message(WARNING "libkmod disabled, kernel modules will not be loaded")
endif()

if("${ENABLE_BLKID}" STREQUAL "OFF")
	message(WARNING "blkid disabled, may not find block devices by tag")
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
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize-recover=all -fno-omit-frame-pointer -fno-stack-protector -fsanitize=leak")
endif()

if("${ENABLE_STATIC}" STREQUAL "ON")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
endif()

if("${ENABLE_STRIP}" STREQUAL "ON")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
endif()

include(libs/regex/regex.cmake)
include(po/CMakeLists.txt)
include(src/CMakeLists.txt)
