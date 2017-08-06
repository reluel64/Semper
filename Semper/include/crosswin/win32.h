#pragma once
#include <crosswin/crosswin.h>
void win32_init_class();
window* win32_init_window();
void win32_click_through(window* w, unsigned char state);
void win32_draw(window* w);
void win32_message_dispatch(crosswin *c);
void win32_set_position(window* w);
void win32_set_dimension(window* w, long width, long height);
void win32_set_opacity(window* w);
void win32_hide(window* w);
void win32_show(window* w);
void win32_destroy_window(window** w);
void win32_show_silent(window* w);
void win32_set_zpos(window *w);
