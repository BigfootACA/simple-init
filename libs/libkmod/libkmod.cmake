add_library(kmod STATIC
	libs/libkmod/libkmod.c
	libs/libkmod/libkmod-builtin.c
	libs/libkmod/libkmod-config.c
	libs/libkmod/libkmod-elf.c
	libs/libkmod/libkmod-file.c
	libs/libkmod/libkmod-index.c
	libs/libkmod/libkmod-list.c
	libs/libkmod/libkmod-module.c
	libs/libkmod/libkmod-signature.c
	libs/libkmod/hash.c
	libs/libkmod/util.c
	libs/libkmod/strbuf.c
)
target_include_directories(kmod PRIVATE libs/libkmod/include)
include_directories(libs/libkmod)
if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(kmod PRIVATE -Wno-constant-conversion)
endif()
