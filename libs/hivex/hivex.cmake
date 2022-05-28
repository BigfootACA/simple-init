add_library(hivex STATIC
	libs/hivex/lib/util.c
	libs/hivex/lib/utf16.c
	libs/hivex/lib/node.c
	libs/hivex/lib/value.c
	libs/hivex/lib/handle.c
	libs/hivex/lib/offset-list.c
	libs/hivex/lib/visit.c
	libs/hivex/lib/write.c
)
include_directories(libs/hivex/include)
