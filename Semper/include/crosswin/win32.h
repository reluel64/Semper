#pragma once
#include <crosswin/crosswin.h>
void win32_init_class();
crosswin_window* win32_init_window();
void win32_click_through(crosswin_window* w);
void win32_draw(crosswin_window* w);
void win32_message_dispatch(crosswin *c);
void win32_set_position(crosswin_window* w);
void win32_set_dimension(crosswin_window* w);
void win32_set_opacity(crosswin_window* w);
void win32_set_visible(crosswin_window *w);
void win32_destroy_window(crosswin_window** w);
void win32_show_silent(crosswin_window* w);
void win32_set_zpos(crosswin_window *w);
int win32_get_monitors(crosswin_monitor **cm, size_t *cnt);
void win32_check_desktop(crosswin *c);
