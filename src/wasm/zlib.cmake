include_directories(../../libs/zlib)
add_library(
	z STATIC
	../../libs/zlib/zutil.c
	../../libs/zlib/adler32.c
	../../libs/zlib/inftrees.c
	../../libs/zlib/inflate.c
	../../libs/zlib/inffast.c
	../../libs/zlib/deflate.c
	../../libs/zlib/compress.c
	../../libs/zlib/crc32.c
	../../libs/zlib/gzlib.c
	../../libs/zlib/gzread.c
	../../libs/zlib/gzwrite.c
	../../libs/zlib/gzclose.c
	../../libs/zlib/trees.c
	../../libs/zlib/uncompr.c
)
target_compile_options(
	z PRIVATE
	-Wno-implicit-fallthrough
	-Wno-deprecated-non-prototype
)
