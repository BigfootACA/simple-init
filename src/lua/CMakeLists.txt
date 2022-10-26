add_library(init_lua STATIC
	data.c
	conf.c
	logger.c
	lua.c
	libs.c
	feature.c
	nanosvg.c
	abootimg.c
	init.c
	url.c
	recovery.c
	filesystem/fs.c
	filesystem/fsh.c
	filesystem/fs_info.c
	filesystem/string.c
)
