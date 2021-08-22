#pragma once

#include <stdint.h>

#define BGA_WIDTH 1024
#define BGA_HEIGHT 768
#define BGA_BPP 32

void bga_init();
void bga_clear(u32);
void bga_draw_pixel(int, int, u32);
void bga_draw_rect(int, int, int, int, u32);
/*void bga_fill_rect(int, int, int, int, u32);*/
