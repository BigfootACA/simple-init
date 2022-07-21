cmake_minimum_required(VERSION 3.10)
project(simple-init-wasm C)
set(CMAKE_C_STANDARD 99)
include(json-c.cmake)
include(zlib.cmake)
add_executable(
	web_gui_render
	web_gui_render.c
	websocket.c
)
target_link_libraries(
	web_gui_render
	websocket.js
	jsonc
	z
)
set_target_properties(
	web_gui_render PROPERTIES LINK_FLAGS "
	-s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]'
	-s EXPORTED_FUNCTIONS='[\"_malloc\"]'
	-s WASM=1 -gsource-map"
)
