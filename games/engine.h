#ifndef _GAMES_ENGINE_H_
#define _GAMES_ENGINE_H_

#include "fun.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Core
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void e_create(const char* window_title);
void e_destroy(void);
void e_poll_events(void);
bool e_wants_quit(void);
void e_present(void);

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Time
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

float e_now(void);

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Color - RGBA32
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Constructors
#define E_U8(i)                 ((i) & 0xFF)
#define E_COLORA(r, g, b, a)    ((E_U8(r) << 24) | (E_U8(g) << 16) | (E_U8(b) << 8) | E_U8(a))
#define E_COLOR(r, g, b)        E_COLORA(r, g, b, 0xFF)   
// Component accessors
#define E_RED(clr)              (((clr) & 0xFF000000) >> 24)
#define E_GREEN(clr)            (((clr) & 0x00FF0000) >> 16)
#define E_BLUE(clr)             (((clr) & 0x0000FF00) >> 8)
#define E_ALPHA(clr)            (((clr) & 0x000000FF))
#define E_REDF(clr)             ((float)E_RED(clr) / (float)0xFF)
#define E_GREENF(clr)           ((float)E_GREEN(clr) / (float)0xFF)
#define E_BLUEF(clr)            ((float)E_BLUE(clr) / (float)0xFF)
#define E_ALPHAF(clr)           ((float)E_ALPHA(clr) / (float)0xFF)
// Constants
#define E_CBLACK 0x000000FF
#define E_CWHITE 0xFFFFFFFF
#define E_CRED   0xFF0000FF
#define E_CGREEN 0x00FF00FF
#define E_CBLUE  0x0000FFFF

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Asset processing
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

typedef struct E_Image {
    int width, height;
    uint32_t* pixels;
} E_Image;

typedef unsigned int E_Texture;

enum {
    E_TEXTURE_BAD = -1,
};

E_Texture e_create_texture(const E_Image* img);

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Rendering
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void e_begin_frame(void);
void e_end_frame(void);
void e_clear(uint32_t clr);
void e_draw_set_color(uint32_t clr);
void e_draw_triangle(float* restrict points);
void e_draw_line(float x1, float y1, float x2, float y2);
void e_draw_line_ex(float x1, float y1, float x2, float y2, float width);

#endif // _GAMES_ENGINE_H_
