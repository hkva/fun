#include "engine.h"
#include "engine.c"

int main(int argc, char* argv[]) {
    e_create("Test");

    do {
        e_poll_events();
    
        fun_msg("+%fs | Hello world", e_now());

        e_begin_frame();
        e_clear(E_CBLACK);

        e_draw_set_color(E_CWHITE);
        float points[2*3] = {
            0, 0,
            0, 500,
            500, 0,
        };
        e_draw_triangle(points);
        e_draw_set_color(E_CRED);
        e_draw_line(10, 10, 200, 200);
        e_draw_line_ex(210, 210, 400, 400, 5);

        e_end_frame();
        e_present();
    } while (!e_wants_quit());

    e_destroy();
    return 0;
}
