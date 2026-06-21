#pragma once

#define RGB565(R, G, B) (((R & (0b11111)) << (5+6)) | ((G & (0b111111)) << 5) | (B & (0b11111)))
#define COLOR_WHITE RGB565(31, 63, 31)
#define COLOR_BLACK RGB565(0, 0, 0)
#define COLOR_BLUE RGB565(1, 27, 19)
#define COLOR_RED RGB565(27, 15, 8)
#define COLOR_GREEN RGB565(5, 41, 13)
#define COLOR_GRAY RGB565(15, 31, 15)