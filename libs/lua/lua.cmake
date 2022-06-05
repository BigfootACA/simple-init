include_directories(libs/lua)
add_library(lua STATIC
	libs/lua/loslib.c
	libs/lua/liolib.c
	libs/lua/lapi.c
	libs/lua/lauxlib.c
	libs/lua/lbaselib.c
	libs/lua/lcode.c
	libs/lua/lcorolib.c
	libs/lua/lutf8lib.c
	libs/lua/lctype.c
	libs/lua/ldblib.c
	libs/lua/ldebug.c
	libs/lua/ldo.c
	libs/lua/ldump.c
	libs/lua/lfunc.c
	libs/lua/lua.c
	libs/lua/lgc.c
	libs/lua/llex.c
	libs/lua/lmathlib.c
	libs/lua/lmem.c
	libs/lua/loadlib.c
	libs/lua/lobject.c
	libs/lua/lopcodes.c
	libs/lua/lparser.c
	libs/lua/lstate.c
	libs/lua/lstring.c
	libs/lua/lstrlib.c
	libs/lua/ltable.c
	libs/lua/ltablib.c
	libs/lua/ltm.c
	libs/lua/lundump.c
	libs/lua/lvm.c
	libs/lua/lzio.c
)
target_compile_definitions(lua PRIVATE LUA_USE_LINUX=1 main=lua_main)
target_compile_options(lua PRIVATE -Wno-implicit-fallthrough)
if ("${ENABLE_READLINE}" STREQUAL "ON")
	target_compile_definitions(lua PRIVATE LUA_USE_READLINE=1)
endif()
