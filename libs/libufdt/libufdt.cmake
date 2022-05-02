include_directories(libs/libufdt/include)
add_library(
	ufdt STATIC
	libs/libufdt/ufdt_convert.c
	libs/libufdt/ufdt_node.c
	libs/libufdt/ufdt_node_pool.c
	libs/libufdt/ufdt_overlay.c
	libs/libufdt/ufdt_prop_dict.c
	libs/libufdt/sysdeps/libufdt_sysdeps_posix.c
)
target_include_directories(ufdt PRIVATE libs/libufdt/sysdeps/include)