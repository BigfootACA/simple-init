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

if(("${ENABLE_FREETYPE2}" STREQUAL "ON") AND ("${SYSTEM_FREETYPE2}" STREQUAL "ON"))
	set(DEPENDS ${DEPENDS} freetype2)
endif()

if("${ENABLE_FDISK}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} fdisk)
endif()

if("${ENABLE_BLKID}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} blkid)
endif()

if("${ENABLE_HIVEX}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} hivex)
endif()

if("${ENABLE_UUID}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} uuid)
endif()

if("${ENABLE_VNCSERVER}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libvncserver)
endif()

if("${ENABLE_GTK}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} gtk+-3.0)
endif()

if("${ENABLE_SDL2}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} sdl2)
endif()

if("${ENABLE_ASAN}" STREQUAL "ON")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_ASAN=1 -fsanitize=address")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fsanitize=leak")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize-recover=all -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector -fsanitize=leak")
endif()

add_custom_command(
	OUTPUT
		"${CMAKE_CURRENT_BINARY_DIR}/rootfs.c"
		"${CMAKE_CURRENT_BINARY_DIR}/rootfs_data.o"
	COMMAND env CC="${CMAKE_C_COMPILER}" bash
		"${CMAKE_SOURCE_DIR}/scripts/gen-rootfs-source.sh"
		"${CMAKE_SOURCE_DIR}"
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${CMAKE_SOURCE_DIR}/root"
	DEPENDS
		"${CMAKE_SOURCE_DIR}/root"
		"${CMAKE_SOURCE_DIR}/root/usr/share/locale"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDS})
include_directories(${DEPS_INCLUDE_DIRS} include)
add_executable(simple-init src/main/main.c)
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
	src/gui
	src/adbd
	src/confd
	src/ttyd
)

target_link_libraries(simple-init -Wl,--start-group)

foreach(SUBDIR ${SUBDIRS})
	add_subdirectory("${SUBDIR}")
	string(REPLACE "src/" "init_" SUBLIB "${SUBDIR}")
	target_link_libraries(simple-init "${SUBLIB}")
endforeach()

if("${ENABLE_GUI}" STREQUAL "ON")
	target_link_libraries(simple-init lvgl)
endif()

if(("${ENABLE_FREETYPE2}" STREQUAL "ON") AND ("${SYSTEM_FREETYPE2}" STREQUAL "OFF"))
	target_link_libraries(simple-init freetype2)
endif()

add_library(init_rootfs STATIC ${CMAKE_CURRENT_BINARY_DIR}/rootfs.c ${CMAKE_CURRENT_BINARY_DIR}/rootfs_data.o)
target_link_libraries(simple-init init_rootfs)

target_link_libraries(simple-init -Wl,--end-group)

if("${ENABLE_LODEPNG}" STREQUAL "ON")
	target_link_libraries(simple-init lodepng)
endif()

if("${ENABLE_STATIC}" STREQUAL "ON")
	target_link_libraries(simple-init ${DEPS_STATIC_LIBRARIES})
else()
	target_link_libraries(simple-init ${DEPS_LIBRARIES})
endif()

target_link_libraries(simple-init pthread crypt)
