file(GLOB SRCS *.c)
add_library(init_initd STATIC ${SRCS})
