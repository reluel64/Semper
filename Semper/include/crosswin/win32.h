#pragma once
#include <crosswin/crosswin.h>
void win32_init_class();
crosswin_window* win32_init_window();
void win32_click_through(crosswin_window* w, unsigned char state);
void win32_draw(crosswin_window* w);
void win32_message_dispatch(crosswin *c);
void win32_set_position(crosswin_window* w);
void win32_set_dimension(crosswin_window* w, long width, long height);
void win32_set_opacity(crosswin_window* w);
void win32_hide(crosswin_window* w);
void win32_show(crosswin_window* w);
void win32_destroy_window(crosswin_window** w);
void win32_show_silent(crosswin_window* w);
void win32_set_zpos(crosswin_window *w);
