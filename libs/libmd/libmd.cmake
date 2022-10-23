add_library(md STATIC
	libs/libmd/src/md2.c
	libs/libmd/src/md4.c
	libs/libmd/src/md5.c
	libs/libmd/src/rmd160.c
	libs/libmd/src/sha1.c
	libs/libmd/src/sha2.c
)
include_directories(libs/libmd/include)
target_compile_options(md PRIVATE -Wno-stringop-overread)
