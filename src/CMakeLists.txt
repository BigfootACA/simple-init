set(DEPENDS mount)

if("${ENABLE_INITSHELL}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} readline)
endif()

if("${ENABLE_KMOD}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libkmod)
endif()

if("${ENABLE_DRM}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libdrm)
endif()

if("${ENABLE_BLKID}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} blkid)
endif()

if("${ENABLE_GTK}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} gtk+-3.0)
endif()

if("${ENABLE_ASAN}" STREQUAL "ON")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_ASAN=1 -fsanitize=address")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fsanitize=leak")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize-recover=all -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector -fsanitize=leak")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDS})
include_directories(${DEPS_INCLUDE_DIRS} include)
add_executable(init src/main.c)
set(SUBDIRS
	src/assets
	src/boot
	src/cmdline
	src/commands
	src/devd
	src/gadget
	src/getopt
	src/hardware
	src/initd
	src/lib
	src/loggerd
	src/shell
	src/service
)

target_link_libraries(init -Wl,--start-group)

foreach(SUBDIR ${SUBDIRS})
    add_subdirectory("${SUBDIR}")
    string(REPLACE "src/" "init_" SUBLIB "${SUBDIR}")
    target_link_libraries(init "${SUBLIB}")
endforeach()

if("${ENABLE_GUI}" STREQUAL "ON")
    target_link_libraries(init lvgl)
endif()

target_link_libraries(init -Wl,--end-group)

if("${ENABLE_STATIC}" STREQUAL "ON")
    target_link_libraries(init ${DEPS_STATIC_LIBRARIES})
else()
    target_link_libraries(init ${DEPS_LIBRARIES})
endif()

target_link_libraries(init pthread)
