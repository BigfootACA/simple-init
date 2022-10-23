include(CheckTypeSize)
check_type_size("long long" HAVE_LONG_LONG)
include(CheckFunctionExists)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(vasprintf HAVE_VASPRINTF)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strlcat HAVE_STRLCAT)
check_function_exists(strlcpy HAVE_STRLCPY)
set(MXML_VERSION "Mini-XML v3.3")
configure_file(libs/mxml/mxml.config.h.in mxml/config.h)
set(MXML_HEADERS
	libs/mxml/mxml-private.h
	${CMAKE_CURRENT_BINARY_DIR}/mxml/config.h
)
set(MXML_SOURCES
	libs/mxml/mxml-attr.c
	libs/mxml/mxml-entity.c
	libs/mxml/mxml-file.c
	libs/mxml/mxml-get.c
	libs/mxml/mxml-index.c
	libs/mxml/mxml-node.c
	libs/mxml/mxml-private.c
	libs/mxml/mxml-search.c
	libs/mxml/mxml-set.c
	libs/mxml/mxml-string.c
)
add_library(mxml ${MXML_SOURCES} ${MXML_HEADERS})
target_include_directories(mxml PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/mxml)
target_compile_options(mxml PRIVATE
	-Wno-use-after-free
	-Wno-sign-compare
	-Wno-implicit-fallthrough
)
include_directories(libs/mxml)
