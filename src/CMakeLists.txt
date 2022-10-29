if("${ENABLE_READLINE}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} readline)
endif()

if("${ENABLE_DRM}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libdrm)
endif()

if(("${ENABLE_FREETYPE2}" STREQUAL "ON") AND ("${SYSTEM_FREETYPE2}" STREQUAL "ON"))
	set(DEPENDS ${DEPENDS} freetype2)
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

if("${ENABLE_LIBJPEG}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libjpeg)
endif()

if("${ENABLE_MICROHTTPD}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libmicrohttpd)
endif()

if("${ENABLE_FFMPEG}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libavcodec libavutil libswscale)
endif()

if("${ENABLE_LIBCURL}" STREQUAL "ON")
	set(DEPENDS ${DEPENDS} libcurl)
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
		"${CMAKE_CURRENT_BINARY_DIR}/rootfs.bin"
	COMMAND env NOBUILD=1 USEASM=1 bash
		"${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen-rootfs-source.sh"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/root"
	DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/root"
		"${CMAKE_CURRENT_SOURCE_DIR}/root/usr/share/locale"
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/rootfs_data.s"
	COMMAND sed
		-e "s@%DIR%@${CMAKE_CURRENT_BINARY_DIR}@"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/rootfs_data.s.in"
		> "${CMAKE_CURRENT_BINARY_DIR}/rootfs_data.s"
	DEPENDS
		"${CMAKE_CURRENT_BINARY_DIR}/rootfs.bin"
)

add_library(
	rootfs_data STATIC
	"${CMAKE_CURRENT_BINARY_DIR}/rootfs_data.s"
	"${CMAKE_CURRENT_BINARY_DIR}/rootfs.bin"
)

if(NOT "${DEPENDS}" STREQUAL "")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDS})
	include_directories(${DEPS_INCLUDE_DIRS} include)
endif()

if("${BUILD_SHARED}" STREQUAL "ON")
	add_library(simple-init SHARED src/main/main.c)
else()
	add_library(simple-init STATIC src/main/main.c)
endif()
add_executable(simple-init-bin src/main/wrapper.c)
target_link_libraries(simple-init-bin simple-init)
set_target_properties(simple-init-bin PROPERTIES OUTPUT_NAME simple-init)
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
	src/bcd
	src/compress
	src/lua
	src/filesystem
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

add_library(init_rootfs STATIC ${CMAKE_CURRENT_BINARY_DIR}/rootfs.c)
target_link_libraries(init_rootfs rootfs_data)
target_link_libraries(simple-init init_rootfs)

if("${ENABLE_LODEPNG}" STREQUAL "ON")
	target_link_libraries(simple-init lodepng)
endif()

if("${ENABLE_JSONC}" STREQUAL "ON")
	target_link_libraries(simple-init jsonc)
endif()

if("${ENABLE_MXML}" STREQUAL "ON")
	target_link_libraries(simple-init mxml)
endif()

if("${ENABLE_LUA}" STREQUAL "ON")
	target_link_libraries(simple-init lua dl m)
endif()

if("${ENABLE_LIBTSM}" STREQUAL "ON")
	target_link_libraries(simple-init libtsm)
endif()

if("${ENABLE_NANOSVG}" STREQUAL "ON")
	target_link_libraries(simple-init nanosvg m)
endif()

if("${ENABLE_STB}" STREQUAL "ON")
	target_link_libraries(simple-init stb)
endif()

if("${ENABLE_HIVEX}" STREQUAL "ON")
	target_link_libraries(simple-init hivex)
endif()

if("${ENABLE_LIBZIP}" STREQUAL "ON")
	target_link_libraries(simple-init ziptools zip)
endif()

target_link_libraries(simple-init kmod mount uuid blkid fdisk smartcols md)

target_link_libraries(simple-init -Wl,--end-group)

if(NOT "${DEPS_STATIC_LIBRARIES}" STREQUAL "" OR NOT "${DEPS_LIBRARIES}" STREQUAL "")
	if("${ENABLE_STATIC}" STREQUAL "ON")
		target_link_libraries(simple-init ${DEPS_STATIC_LIBRARIES})
	else()
		target_link_libraries(simple-init ${DEPS_LIBRARIES})
	endif()
endif()

target_link_libraries(simple-init pthread crypt regex z)
