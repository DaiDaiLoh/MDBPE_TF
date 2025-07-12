// stb image produces these warnings and it is unlikely to be fixed
#if defined(__GNUG__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <glow/detail/stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <glow/detail/stb/stb_image_write.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <glow/detail/stb/stb_truetype.h>
