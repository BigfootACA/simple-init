file(GLOB SRCS */*.c)
include_directories(../include)
add_executable(init main.c ${SRCS})
set(DEPENDS mount)

if("${ENABLE_INITSHELL}" STREQUAL "ON")
    set(DEPENDS ${DEPENDS} readline)
endif()

if("${ENABLE_KMOD}" STREQUAL "ON")
    set(DEPENDS ${DEPENDS} libkmod)
endif()

if("${ENABLE_BLKID}" STREQUAL "ON")
    set(DEPENDS ${DEPENDS} blkid)
endif()

if("${ENABLE_ASAN}" STREQUAL "ON")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_ASAN=1 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize-recover=all -fno-omit-frame-pointer -fno-stack-protector -fsanitize=leak")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDS})
if("${ENABLE_STATIC}" STREQUAL "ON")
    target_link_libraries(init ${DEPS_STATIC_LIBRARIES})
else()
    target_link_libraries(init ${DEPS_LIBRARIES})
endif()

target_link_libraries(init pthread)
