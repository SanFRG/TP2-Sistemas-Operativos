#include <stdint.h>

void putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
void drawChar(char c, uint64_t x, uint64_t y, uint32_t color, int scale);
void drawString(const char* str, uint64_t x, uint64_t y, uint32_t color, int scale);
uint16_t getScreenWidth();
uint16_t getScreenHeight();
void fillRect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color);
void drawRectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color);
void clearScreen(uint32_t color);
void scrollUp(uint64_t pixels, uint32_t bgColor);
void scrollUpLines(uint64_t lines, int scale, uint32_t bgColor);

