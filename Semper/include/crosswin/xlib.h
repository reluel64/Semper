#pragma once
#include <crosswin/crosswin.h>
#ifdef __linux__
int  xlib_message_dispatch(crosswin *c);
int  xlib_set_opacity(crosswin_window *w);
void xlib_draw(crosswin_window* w);
void xlib_init_window(crosswin_window *w);
void xlib_init_display(crosswin *c);
void xlib_set_dimmension(crosswin_window *w);
void xlib_set_position(crosswin_window *w);
void xlib_destroy_window(crosswin_window **w);
void xlib_set_mask(crosswin_window *w);
void xlib_set_zpos(crosswin_window *w);
int xlib_destroy_input_context(crosswin_window *w);
int xlib_create_input_context(crosswin_window *w);
int xlib_get_monitors(crosswin *c, crosswin_monitor **cm, size_t *cnt);
void xlib_set_visible(crosswin_window *w);
void xlib_check_desktop(crosswin *c);
void xlib_click_through(crosswin_window *w);
#endif
