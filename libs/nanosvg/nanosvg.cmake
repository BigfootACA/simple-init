include_directories(libs/nanosvg/src)
add_library(nanosvg STATIC "${CMAKE_SOURCE_DIR}/libs/nanosvg/nanosvg.c")
