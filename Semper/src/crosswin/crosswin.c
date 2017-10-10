/*
 * This aims to be a super slim layer for Project 'Semper'
 * The purpose of this component is to bring an abstraction
 * layer for window creation on Windows and Linux.
 * Note: This is a work in progress so features will be added when they'll be needed
 *
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */

#include <crosswin/crosswin.h>
#ifdef WIN32
#include <crosswin/win32.h>
#elif __linux__
#include <crosswin/xlib.h>
#endif
#include <semper.h>
#include <mem.h>

void crosswin_init(crosswin* c)
{
#ifdef WIN32
    win32_init_class();
    c->sh = GetSystemMetrics(SM_CYSCREEN);
    c->sw = GetSystemMetrics(SM_CXSCREEN);

#elif __linux__

    xlib_init_display(c);
    Screen *s=DefaultScreenOfDisplay(c->display);
    if(s)
    {
        c->sw=s->width;
        c->sh=s->height;
    }
#endif
}

int crosswin_update(crosswin* c)
{
    long oh=c->sh;
    long ow=c->sw;
#ifdef WIN32
    c->sh = GetSystemMetrics(SM_CYSCREEN);
    c->sw = GetSystemMetrics(SM_CXSCREEN);
#elif __linux__
    Screen *s=DefaultScreenOfDisplay(c->display);
    if(s)
    {
        c->sw=s->width;
        c->sh=s->height;
    }
#endif
    return(oh!=c->sh||ow!=c->sw);
}

void crosswin_monitor_resolution(crosswin* c, long* w, long* h)
{
    *w = c->sw;
    *h = c->sh;
}
void crosswin_message_dispatch(crosswin *c)
{
#ifdef WIN32
    unused_parameter(c);
    win32_message_dispatch(c);
#elif __linux__
    xlib_message_dispatch(c);
#endif
}

window* crosswin_init_window(crosswin* c)
{
    window* w = zmalloc(sizeof(window));
    w->c = c;
#ifdef WIN32
    win32_init_window(w);
#elif __linux__
    xlib_init_window(w);
#endif
    return (w);
}

void crosswin_set_window_data(window* w, void* pv)
{
    w->user_data = pv;
}

void* crosswin_get_window_data(window* w)
{
    return (w->user_data);
}

void crosswin_click_through(window* w, unsigned char state)
{
#ifdef WIN32
    win32_click_through(w, state);
#elif __linux__
    w->click_through=state;

#endif
}

void crosswin_draw(window* w)
{
    crosswin* c = w->c;
    if(w->keep_on_screen == 1)
    {
        if(w->x < 0)
            w->x = 0;
        else if(w->x + w->w > c->sw)
            w->x = max(c->sw - w->w, 0);
        if(w->y < 0)
            w->y = 0;
        else if(w->y + w->h > c->sh)
            w->y = max(c->sh - w->h, 0);
        crosswin_set_position(w, w->x, w->y);
    }
#ifdef WIN32

    win32_draw(w);

#elif __linux__

    xlib_draw(w);
#endif
}

void crosswin_set_render(window* w, void (*render)(window* pv, void* cr))
{
    w->render_func = render;
}

void crosswin_set_position(window* w, long x, long y)
{
    w->x = x;
    w->y = y;
#ifdef WIN32
    win32_set_position(w);
#elif __linux__
    xlib_set_position(w);
#endif
}

void crosswin_get_position(window* w, long* x, long* y)
{
    *x = w->x;
    *y = w->y;
}

void crosswin_set_opacity(window* w, unsigned char opacity)
{
    w->opacity=opacity;
#ifdef WIN32
    win32_set_opacity(w);
#elif __linux__
    xlib_set_opacity(w);
#endif
}

void crosswin_set_dimension(window* w, long width, long height)
{
    w->w = labs(width);
    w->h = labs(height);
#ifdef __linux__
    xlib_set_dimmension(w);
#endif
}

void crosswin_set_mouse_handler(window* w, int (*mouse_handler)(window* w, mouse_status* ms))
{
    w->mouse_func = mouse_handler;
}

void crosswin_set_kbd_handler(window *w,int(*kbd_func)(unsigned  int key_code,void *p),void *kb_data)
{
    w->kbd_func=kbd_func;
    w->kb_data=kb_data;
#ifdef __linux__
    w->kbd_func?xlib_create_input_context(w):xlib_destroy_input_context(w);
#endif
}

void crosswin_hide(window* w)
{
#ifdef WIN32
    win32_hide(w);
#endif
}

void crosswin_show(window* w)
{
#ifdef WIN32
    win32_show(w);
#endif
}

void crosswin_destroy(window** w)
{
#ifdef WIN32
    win32_destroy_window(w);
#elif __linux__
    xlib_destroy_window(w);
#endif
}

void crosswin_draggable(window* w, unsigned char draggable)
{
    w->draggable = draggable;
}

void crosswin_keep_on_screen(window* w, unsigned char keep_on_screen)
{
    w->keep_on_screen = keep_on_screen;
}

void crosswin_set_window_z_order(window* w, unsigned char zorder)
{
    w->zorder = zorder;
#ifdef WIN32
    win32_set_zpos(w);
#elif __linux__
    xlib_set_zpos(w);
#endif
}
