file(GLOB SRCS *.c)
add_library(init_cmdline STATIC ${SRCS})
