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
    list_entry_init(&c->windows);
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

crosswin_window* crosswin_init_window(crosswin* c)
{
    crosswin_window* w = zmalloc(sizeof(crosswin_window));
    list_entry_init(&w->current);
    linked_list_add(&w->current,&c->windows);
    w->c = c;
#ifdef WIN32
    win32_init_window(w);
#elif __linux__
    xlib_init_window(w);
#endif


    return (w);
}

void crosswin_set_window_data(crosswin_window* w, void* pv)
{
    w->user_data = pv;
}

void* crosswin_get_window_data(crosswin_window* w)
{
    return (w->user_data);
}

void crosswin_click_through(crosswin_window* w, unsigned char state)
{
#ifdef WIN32
    win32_click_through(w, state);
#elif __linux__
    w->click_through=state;

#endif
}

void crosswin_draw(crosswin_window* w)
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

void crosswin_set_render(crosswin_window* w, void (*render)(crosswin_window* pv, void* cr))
{
    w->render_func = render;
}

void crosswin_set_position(crosswin_window* w, long x, long y)
{
    w->x = x;
    w->y = y;
#ifdef WIN32
    win32_set_position(w);
#elif __linux__
    xlib_set_position(w);
#endif
}

void crosswin_get_position(crosswin_window* w, long* x, long* y)
{
    *x = w->x;
    *y = w->y;
}

void crosswin_set_opacity(crosswin_window* w, unsigned char opacity)
{
    w->opacity=opacity;
#ifdef WIN32
    win32_set_opacity(w);
#elif __linux__
    xlib_set_opacity(w);
#endif
}

void crosswin_set_dimension(crosswin_window* w, long width, long height)
{
    w->w = labs(width);
    w->h = labs(height);
#ifdef __linux__
    xlib_set_dimmension(w);
#endif
}

void crosswin_set_mouse_handler(crosswin_window* w, int (*mouse_handler)(crosswin_window* w, mouse_status* ms))
{
    w->mouse_func = mouse_handler;
}

void crosswin_set_kbd_handler(crosswin_window *w,int(*kbd_func)(unsigned  int key_code,void *p),void *kb_data)
{
    w->kbd_func=kbd_func;
    w->kb_data=kb_data;
#ifdef __linux__
    w->kbd_func?xlib_create_input_context(w):xlib_destroy_input_context(w);
#endif
}

void crosswin_hide(crosswin_window* w)
{
#ifdef WIN32
    win32_hide(w);
#endif
}

void crosswin_show(crosswin_window* w)
{
#ifdef WIN32
    win32_show(w);
#endif
}

void crosswin_destroy(crosswin_window** w)
{
    if(w&&*w)
    {
        linked_list_remove(&(*w)->current);
#ifdef WIN32
        win32_destroy_window(w);
#elif __linux__
        xlib_destroy_window(w);
#endif

    }
}

void crosswin_draggable(crosswin_window* w, unsigned char draggable)
{
    w->draggable = draggable;
}

void crosswin_keep_on_screen(crosswin_window* w, unsigned char keep_on_screen)
{
    w->keep_on_screen = keep_on_screen;
}

static int crosswin_sort_callback(list_entry *le1,list_entry *le2,void *pv)
{
    crosswin_window *cw1=element_of(le1,crosswin_window,current);
    crosswin_window *cw2=element_of(le2,crosswin_window,current);

    if(cw1->zorder<cw2->zorder)
        return(-1);
    else if(cw1->zorder>cw2->zorder)
        return(1);
    else
        return(0);
}
void crosswin_set_window_z_order(crosswin_window* w, unsigned char zorder)
{
    if(w)
    {
        if(w->zorder!=zorder)
        {
            w->zorder = zorder;
            merge_sort(&w->c->windows,crosswin_sort_callback,NULL);
        }
#ifdef WIN32
        win32_set_zpos(w);
#elif __linux__
        xlib_set_zpos(w);
#endif
    }
}

void crosswin_update_z(crosswin *c)
{
   // if(c->update_z)
    {
        crosswin_window *cw=NULL;
        list_enum_part_backward(cw,&c->windows,current)
        {
            crosswin_set_window_z_order(cw,cw->zorder);
        }
        c->update_z=0;
   }
}
