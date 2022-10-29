add_library(ziptools
	zipcmp.c
	zipmerge.c
	ziptool.c
	diff_output.c
)
target_include_directories(ziptools PRIVATE BEFORE ${PROJECT_SOURCE_DIR}/lib ${PROJECT_BINARY_DIR})
