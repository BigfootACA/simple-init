if("${SYSTEM_FREETYPE2}" STREQUAL "OFF")
	include(CheckIncludeFile)
	find_package(ZLIB)
	find_package(BZip2)
	check_include_file("unistd.h" HAVE_UNISTD_H)
	check_include_file("fcntl.h" HAVE_FCNTL_H)
	set(FTCONFIG_H_NAME "${PROJECT_BINARY_DIR}/freetype/ftconfig.h")
	set(FTOPTION_H_NAME "${PROJECT_BINARY_DIR}/freetype/ftoption.h")
	file(READ "${PROJECT_SOURCE_DIR}/libs/freetype/builds/unix/ftconfig.h.in" FTCONFIG_H)
	file(READ "${PROJECT_SOURCE_DIR}/libs/freetype/include/freetype/config/ftoption.h" FTOPTION_H)
	if(HAVE_UNISTD_H)
		string(REGEX REPLACE "#undef +(HAVE_UNISTD_H)" "#define \\1 1" FTCONFIG_H "${FTCONFIG_H}")
		set(FTOPTION_H "${FTOPTION_H}\n#define HAVE_UNISTD_H 1")
	endif()
	if(HAVE_FCNTL_H)
		string(REGEX REPLACE "#undef +(HAVE_FCNTL_H)" "#define \\1 1" FTCONFIG_H "${FTCONFIG_H}")
		set(FTOPTION_H "${FTOPTION_H}\n#define HAVE_FCNTL_H 1")
	endif()
	if(ZLIB_FOUND)
		string(REGEX REPLACE "/\\* +(#define +FT_CONFIG_OPTION_SYSTEM_ZLIB) +\\*/" "\\1" FTOPTION_H "${FTOPTION_H}")
	endif()
	if(BZIP2_FOUND)
		string(REGEX REPLACE "/\\* +(#define +FT_CONFIG_OPTION_USE_BZIP2) +\\*/" "\\1" FTOPTION_H "${FTOPTION_H}")
	endif()
	if(EXISTS "${FTCONFIG_H_NAME}")
		file(READ "${FTCONFIG_H_NAME}" ORIGINAL_FTCONFIG_H)
	else()
		set(ORIGINAL_FTCONFIG_H "")
	endif()
	if(EXISTS "${FTOPTION_H_NAME}")
		file(READ "${FTOPTION_H_NAME}" ORIGINAL_FTOPTION_H)
	else()
		set(ORIGINAL_FTOPTION_H "")
	endif()
	if(NOT (ORIGINAL_FTCONFIG_H STREQUAL FTCONFIG_H))
		file(WRITE "${FTCONFIG_H_NAME}" "${FTCONFIG_H}")
	endif()
	if(NOT (ORIGINAL_FTOPTION_H STREQUAL FTOPTION_H))
		file(WRITE "${FTOPTION_H_NAME}" "${FTOPTION_H}")
	endif()
	set(FT2_SRCS
		libs/freetype/src/autofit/autofit.c
		libs/freetype/src/base/ftbase.c
		libs/freetype/src/base/ftbbox.c
		libs/freetype/src/base/ftbdf.c
		libs/freetype/src/base/ftbitmap.c
		libs/freetype/src/base/ftcid.c
		libs/freetype/src/base/ftdebug.c
		libs/freetype/src/base/ftfstype.c
		libs/freetype/src/base/ftgasp.c
		libs/freetype/src/base/ftglyph.c
		libs/freetype/src/base/ftgxval.c
		libs/freetype/src/base/ftinit.c
		libs/freetype/src/base/ftmm.c
		libs/freetype/src/base/ftotval.c
		libs/freetype/src/base/ftpatent.c
		libs/freetype/src/base/ftpfr.c
		libs/freetype/src/base/ftstroke.c
		libs/freetype/src/base/ftsynth.c
		libs/freetype/src/base/fttype1.c
		libs/freetype/src/base/ftwinfnt.c
		libs/freetype/src/bdf/bdf.c
		libs/freetype/src/bzip2/ftbzip2.c
		libs/freetype/src/cache/ftcache.c
		libs/freetype/src/cff/cff.c
		libs/freetype/src/cid/type1cid.c
		libs/freetype/src/gzip/ftgzip.c
		libs/freetype/src/lzw/ftlzw.c
		libs/freetype/src/pcf/pcf.c
		libs/freetype/src/pfr/pfr.c
		libs/freetype/src/psaux/psaux.c
		libs/freetype/src/pshinter/pshinter.c
		libs/freetype/src/psnames/psnames.c
		libs/freetype/src/raster/raster.c
		libs/freetype/src/sfnt/sfnt.c
		libs/freetype/src/smooth/smooth.c
		libs/freetype/src/truetype/truetype.c
		libs/freetype/src/type1/type1.c
		libs/freetype/src/type42/type42.c
		libs/freetype/src/winfonts/winfnt.c
		libs/freetype/builds/unix/ftsystem.c
	)
	add_library(freetype2 STATIC ${FT2_SRCS})
	target_compile_definitions(freetype2 PRIVATE FT2_BUILD_LIBRARY=1 FT_CONFIG_OPTIONS_H=<ftoption.h>)
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		target_compile_options(freetype2 PRIVATE -Wno-dangling-pointer)
	endif()
	include_directories(libs/freetype/include)
	target_include_directories(freetype2 PRIVATE ${PROJECT_BINARY_DIR}/freetype)
	if(ZLIB_FOUND)
		target_link_libraries(freetype2 z)
	endif()
	if(BZIP2_FOUND)
		target_link_libraries(freetype2 bz2)
	endif()
endif()
