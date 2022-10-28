#ifdef ENABLE_UEFI
#define STBI_ASSERT(x) {ASSERT(x);if(!(x))abort();}
#define STBI_NO_GIF
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#else
#include<assert.h>
#define STBI_ASSERT(x) assert(x);
#endif
#define STBCC_GRID_COUNT_X_LOG2 10
#define STBCC_GRID_COUNT_Y_LOG2 10
#define STB_IMAGE_IMPLEMENTATION
#define STB_C_LEXER_IMPLEMENTATION
#define STB_CONNECTED_COMPONENTS_IMPLEMENTATION
#define STB_DIVIDE_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION
#define STB_DXT_IMPLEMENTATION
#define STB_HEXWAVE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_INCLUDE_IMPLEMENTATION
#define STB_LEAKCHECK_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include<unistd.h>
#include"stb_c_lexer.h"
#include"stb_connected_components.h"
#include"stb_divide.h"
#include"stb_ds.h"
#include"stb_dxt.h"
#include"stb_hexwave.h"
#include"stb_image.h"
#include"stb_image_resize.h"
#include"stb_image_write.h"
#include"stb_include.h"
#include"stb_leakcheck.h"
#include"stb_rect_pack.h"
#include"stb_sprintf.h"
#include"stb_textedit.h"
#include"stb_truetype.h"
