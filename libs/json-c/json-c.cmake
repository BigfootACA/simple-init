if(POLICY CMP0048)
        cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0075)
	cmake_policy(SET CMP0075 NEW)
endif()
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
include(CheckTypeSize)
include(CMakePackageConfigHelpers)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
list(APPEND CMAKE_REQUIRED_LIBRARIES m)
check_include_file("fcntl.h"        HAVE_FCNTL_H)
check_include_file("inttypes.h"     HAVE_INTTYPES_H)
check_include_file(stdarg.h         HAVE_STDARG_H)
check_include_file(strings.h        HAVE_STRINGS_H)
check_include_file(string.h         HAVE_STRING_H)
check_include_file(syslog.h         HAVE_SYSLOG_H)
check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
check_include_file(unistd.h         HAVE_UNISTD_H)
check_include_file(sys/types.h      HAVE_SYS_TYPES_H)
check_include_file(sys/resource.h   HAVE_SYS_RESOURCE_H) # for getrusage
check_include_file("dlfcn.h"        HAVE_DLFCN_H)
check_include_file("endian.h"       HAVE_ENDIAN_H)
check_include_file("limits.h"       HAVE_LIMITS_H)
check_include_file("locale.h"       HAVE_LOCALE_H)
check_include_file("memory.h"       HAVE_MEMORY_H)
check_include_file(stdint.h         HAVE_STDINT_H)
check_include_file(stdlib.h         HAVE_STDLIB_H)
check_include_file(sys/cdefs.h      HAVE_SYS_CDEFS_H)
check_include_file(sys/param.h      HAVE_SYS_PARAM_H)
check_include_file(sys/random.h     HAVE_SYS_RANDOM_H)
check_include_file(sys/stat.h       HAVE_SYS_STAT_H)
check_include_file(xlocale.h        HAVE_XLOCALE_H)
if (HAVE_INTTYPES_H)
	set(JSON_C_HAVE_INTTYPES_H 1)
endif()
check_symbol_exists(_isnan          "float.h" HAVE_DECL__ISNAN)
check_symbol_exists(_finite         "float.h" HAVE_DECL__FINITE)
check_symbol_exists(INFINITY        "math.h" HAVE_DECL_INFINITY)
check_symbol_exists(isinf           "math.h" HAVE_DECL_ISINF)
check_symbol_exists(isnan           "math.h" HAVE_DECL_ISNAN)
check_symbol_exists(nan             "math.h" HAVE_DECL_NAN)
check_symbol_exists(_doprnt         "stdio.h" HAVE_DOPRNT)
check_symbol_exists(snprintf        "stdio.h" HAVE_SNPRINTF)
check_symbol_exists(vasprintf       "stdio.h" HAVE_VASPRINTF)
check_symbol_exists(vsnprintf       "stdio.h" HAVE_VSNPRINTF)
check_symbol_exists(vprintf         "stdio.h" HAVE_VPRINTF)
check_symbol_exists(arc4random      "stdlib.h" HAVE_ARC4RANDOM)
if (HAVE_FCNTL_H)
	check_symbol_exists(open        "fcntl.h" HAVE_OPEN)
endif()
if (HAVE_STDLIB_H)
	check_symbol_exists(realloc     "stdlib.h" HAVE_REALLOC)
endif()
if (HAVE_LOCALE_H)
	check_symbol_exists(setlocale   "locale.h" HAVE_SETLOCALE)
	check_symbol_exists(uselocale   "locale.h" HAVE_USELOCALE)
endif()
if (HAVE_STRINGS_H)
	check_symbol_exists(strcasecmp  "strings.h" HAVE_STRCASECMP)
	check_symbol_exists(strncasecmp "strings.h" HAVE_STRNCASECMP)
endif()
if (HAVE_STRING_H)
	check_symbol_exists(strdup      "string.h" HAVE_STRDUP)
	check_symbol_exists(strerror    "string.h" HAVE_STRERROR)
endif()
if (HAVE_SYSLOG_H)
	check_symbol_exists(vsyslog     "syslog.h" HAVE_VSYSLOG)
endif()
if (HAVE_SYS_RANDOM_H)
	check_symbol_exists(getrandom   "sys/random.h" HAVE_GETRANDOM)
endif()
if (HAVE_SYS_RESOURCE_H)
	check_symbol_exists(getrusage   "sys/resource.h" HAVE_GETRUSAGE)
endif()
check_symbol_exists(strtoll     "stdlib.h" HAVE_STRTOLL)
check_symbol_exists(strtoull    "stdlib.h" HAVE_STRTOULL)
set(json_c_strtoll "strtoll")
if (NOT HAVE_STRTOLL)
check_symbol_exists(_strtoi64   "stdlib.h" __have_strtoi64)
if (__have_strtoi64)
	set(json_c_strtoll      "_strtoi64")
endif()
endif()
set(json_c_strtoull "strtoull")
if (NOT HAVE_STRTOULL)
check_symbol_exists(_strtoui64  "stdlib.h" __have_strtoui64)
if (__have_strtoui64)
	set(json_c_strtoull     "_strtoui64")
endif()
endif()
check_type_size(int                 SIZEOF_INT)
check_type_size(int64_t             SIZEOF_INT64_T)
check_type_size(long                SIZEOF_LONG)
check_type_size("long long"         SIZEOF_LONG_LONG)
check_type_size("size_t"            SIZEOF_SIZE_T)
check_type_size("ssize_t"           SIZEOF_SSIZE_T)
check_c_source_compiles("int main() { int i, x = 0; i = __sync_add_and_fetch(&x,1); return x; }" HAVE_ATOMIC_BUILTINS)
if (NOT DISABLE_THREAD_LOCAL_STORAGE)
	check_c_source_compiles("__thread int x = 0; int main() { return 0; }" HAVE___THREAD)
	if (HAVE___THREAD)
		set(SPEC___THREAD __thread)
	endif()
endif()
file(WRITE ${PROJECT_BINARY_DIR}/json-c/private/config.h "#define _GNU_SOURCE\n#include\"_config.h\"\n")
configure_file(libs/json-c/cmake/config.h.in        ${PROJECT_BINARY_DIR}/json-c/private/_config.h)
configure_file(libs/json-c/cmake/json_config.h.in   ${PROJECT_BINARY_DIR}/json-c/json_config.h)
check_c_source_compiles("#define _REENTRANT 1\n#include <sys/types.h>\nint main (void){return 0;}" REENTRANT_WORKS)
set(JSON_C_PUBLIC_HEADERS
	${PROJECT_BINARY_DIR}/json-c/json_config.h
	${PROJECT_BINARY_DIR}/json-c/private/config.h
	libs/json-c/arraylist.h
	libs/json-c/debug.h
	libs/json-c/json.h
	libs/json-c/json_c_version.h
	libs/json-c/json_inttypes.h
	libs/json-c/json_object.h
	libs/json-c/json_object_iterator.h
	libs/json-c/json_tokener.h
	libs/json-c/json_types.h
	libs/json-c/json_util.h
	libs/json-c/json_visit.h
	libs/json-c/linkhash.h
	libs/json-c/printbuf.h
	libs/json-c/json_pointer.h
)
set(JSON_C_HEADERS
	${JSON_C_PUBLIC_HEADERS}
	libs/json-c/json_object_private.h
	libs/json-c/random_seed.h
	libs/json-c/strerror_override.h
	libs/json-c/strerror_override_private.h
	libs/json-c/math_compat.h
	libs/json-c/snprintf_compat.h
	libs/json-c/strdup_compat.h
	libs/json-c/vasprintf_compat.h
)
set(JSON_C_SOURCES
	libs/json-c/arraylist.c
	libs/json-c/debug.c
	libs/json-c/json_c_version.c
	libs/json-c/json_object.c
	libs/json-c/json_object_iterator.c
	libs/json-c/json_tokener.c
	libs/json-c/json_util.c
	libs/json-c/json_visit.c
	libs/json-c/linkhash.c
	libs/json-c/printbuf.c
	libs/json-c/random_seed.c
	libs/json-c/strerror_override.c
	libs/json-c/json_pointer.c
)
set(JSON_H_JSON_POINTER "#include \"json_pointer.h\"")
include_directories(libs/json-c ${PROJECT_BINARY_DIR}/json-c)
add_library(jsonc STATIC
	${JSON_C_SOURCES}
	${JSON_C_HEADERS}
)
target_include_directories(jsonc PRIVATE ${PROJECT_BINARY_DIR}/json-c/private)
if (REENTRANT_WORKS)
	target_compile_options(jsonc PRIVATE -D_REENTRANT)
endif()
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(jsonc PRIVATE -ffunction-sections -fdata-sections)
endif()
