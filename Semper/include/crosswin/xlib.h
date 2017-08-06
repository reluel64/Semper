#pragma once
#include <crosswin/crosswin.h>
int  xlib_message_dispatch(crosswin *c);
int  xlib_set_opacity(window *w);
void xlib_draw(window* w);
void xlib_init_window(window *w);
void xlib_init_display(crosswin *c);
void xlib_set_dimmension(window *w);
void xlib_set_position(window *w);
void xlib_destroy_window(window **w);
void xlib_set_mask(window *w);
void xlib_set_zpos(window *w);