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
#include <time.h>
#ifdef WIN32
#include <crosswin/win32.h>
#elif __linux__
#include <crosswin/xlib.h>
#endif
#include <semper.h>
#include <mem.h>


static int crosswin_mouse_handle(crosswin_window *cw);
crosswin_monitor *crosswin_get_monitor(crosswin *c, size_t index);


void crosswin_init(crosswin* c)
{
    list_entry_init(&c->windows);
    c->handle_mouse = crosswin_mouse_handle;
    
#ifdef WIN32
    win32_init_class();
#elif __linux__
    xlib_init_display(c);
#endif

    c->update = 1;
    crosswin_update(c);
}

int crosswin_update(crosswin* c)
{
    unsigned char up_status = c->update;

    if(up_status)
    {
        c->update = 0;
        c->mon_cnt = 0;
        sfree((void**)&c->pm);
        crosswin_get_monitors(c, &c->pm, &c->mon_cnt);
    }

    return(up_status);
}

void crosswin_monitor_resolution(crosswin* c, crosswin_window *cw, long* w, long* h)
{
    crosswin_monitor *cm = crosswin_get_monitor(c, cw->mon);
    *w = cm->w;
    *h = cm->h;
}

void crosswin_monitor_origin(crosswin *c, crosswin_window *cw, long *x, long *y)
{
    crosswin_monitor *cm = crosswin_get_monitor(c, cw->mon);
    *x = cm->x;
    *y = cm->y;

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
    linked_list_add(&w->current, &c->windows);
    w->c = c;
#ifdef WIN32
    win32_init_window(w);
#elif __linux__
    xlib_init_window(w);
#endif


    return (w);
}

int crosswin_get_monitors(crosswin *c, crosswin_monitor **cm, size_t *len)
{
#ifdef WIN32
    unused_parameter(c);
    return(win32_get_monitors(cm, len));
#elif __linux__
    return(xlib_get_monitors(c, cm, len));
#endif
}


void crosswin_set_window_data(crosswin_window* w, void* pv)
{
    if(w)
        w->user_data = pv;
}

void* crosswin_get_window_data(crosswin_window* w)
{
    return (w ? w->user_data : NULL);
}

void crosswin_click_through(crosswin_window* w, unsigned char state)
{
    if(w)
    {
#ifdef WIN32
        win32_click_through(w, state);
#elif __linux__
        w->click_through = state;
#endif
    }
}
void crosswin_draw(crosswin_window* w)
{
    if(w)
    {
        crosswin_set_position(w, w->x, w->y);

#ifdef WIN32
        win32_draw(w);
#elif __linux__
        xlib_draw(w);
#endif
    }
}

void crosswin_set_render(crosswin_window* w, void (*render)(crosswin_window* pv, void* cr))
{
    if(w)
        w->render_func = render;
}

crosswin_monitor *crosswin_get_monitor(crosswin *c, size_t index)
{
    if(c->mon_cnt <= index)
        return(c->pm);
    else
        return(c->pm + index);
}


void crosswin_set_position(crosswin_window* w, long x, long y)
{

    if(w && (w->x!=x)||(w->y != y)) 
    {
        crosswin* c = w->c;
        crosswin_monitor *cm = crosswin_get_monitor(c, w->mon);
        w->x = x;
        w->y = y;

        if(w->keep_on_screen == 1)
        {
            if(w->x < cm->x)
                w->x = cm->x;

            else if(w->x + w->w > cm->w + cm->x)
                w->x = (cm->w + cm->x) - w->w;

            if(w->y < cm->y)
                w->y = cm->y;

            else if(w->y + w->h > cm->h + cm->y)
                w->y = (cm->h + cm->y) - w->h;
        }
        else
        {

            for(size_t i = 0; i < c->mon_cnt; i++)
            {
                cm = crosswin_get_monitor(w->c, i);

                if(w->x >= cm->x && w->x <= (cm->w + cm->x) && w->y >= cm->y && w->y <= cm->h + cm->y)
                {
                    w->mon = i;
                    break;
                }
            }
        }

#ifdef WIN32
        win32_set_position(w);
#elif __linux__
        xlib_set_position(w);
#endif

    }
}

void crosswin_get_position(crosswin_window* w, long* x, long* y, size_t *monitor)
{
    if(w)
    {
        if(x)
            *x = w->x;

        if(y)
            *y = w->y;

        if(monitor)
            *monitor = w->mon;
    }
}

void crosswin_set_opacity(crosswin_window* w, unsigned char opacity)
{
    if(w && w->opacity!=opacity)
    {
        w->opacity = opacity;
#ifdef WIN32
        win32_set_opacity(w);
#elif __linux__
        xlib_set_opacity(w);
#endif

    }
}

void crosswin_set_dimension(crosswin_window* w, long width, long height)
{
    if(w && (w->w!=width ||w->h != height))
    {
        w->w = labs(width);
        w->h = labs(height);
    

#ifdef __linux__
    xlib_set_dimmension(w);
#endif
    }
}

void crosswin_set_monitor(crosswin_window *w, size_t mon)
{
    if(w)
    {
        w->mon = mon;
    }
}

void crosswin_set_mouse_handler(crosswin_window* w, int (*mouse_handler)(crosswin_window* w, mouse_status* ms))
{
    if(w)
    {
        w->mouse_func = mouse_handler;
    }
}

void crosswin_set_kbd_handler(crosswin_window *w, int(*kbd_func)(unsigned  int key_code, void *p), void *kb_data)
{
    if(w)
    {
        w->kbd_func = kbd_func;
        w->kb_data = kb_data;
#ifdef __linux__
        w->kbd_func ? xlib_create_input_context(w) : xlib_destroy_input_context(w);
#endif
    }
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
    if(w && *w)
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
    if(w)
        w->draggable = draggable;
}

void crosswin_keep_on_screen(crosswin_window* w, unsigned char keep_on_screen)
{
    if(w)
    {
        w->keep_on_screen = keep_on_screen;
        crosswin_set_position(w, w->x, w->y);
    }
}

static int crosswin_sort_callback(list_entry *le1, list_entry *le2, void *pv)
{
    crosswin_window *cw1 = element_of(le1, crosswin_window, current);
    crosswin_window *cw2 = element_of(le2, crosswin_window, current);

    if(cw1->zorder < cw2->zorder)
        return(-1);
    else if(cw1->zorder > cw2->zorder)
        return(1);
    else
        return(0);
}
void crosswin_set_window_z_order(crosswin_window* w, unsigned char zorder)
{
    if(w)
    {
        if(w->zorder != zorder)
        {
            w->zorder = zorder;
            merge_sort(&w->c->windows, crosswin_sort_callback, NULL);
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

        c->update_z = 0;
    }
}


static size_t crosswin_get_time(void)
{
#ifdef WIN32
    return(clock());
#elif __linux__
    struct timespec t = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return(t.tv_sec * 1000 + t.tv_nsec / 1000000);
#endif
}

static int crosswin_mouse_handle(crosswin_window *cw)
{
    crosswin *c = cw->c;
    mouse_data *md = &c->md;
    mouse_status ms = {0};
    long lx = md->cposx;
    long ly = md->cposy;
    md->cposx = md->root_x;
    md->cposy = md->root_y;

    if(md->state == mouse_button_state_unpressed && md->drag)
    {
        md->drag = 0;
        return(0);
    }

    if(!md->ctrl && md->drag == 0)
    {
        unsigned char was_double = 0;
        long delta = 0;

        if(md->button != mouse_button_none)
        {
            if(md->state == mouse_button_state_pressed)
            {
                size_t current_press = crosswin_get_time();

                if(md->button == md->obtn)
                    delta = labs(current_press - md->last_press);
                else
                    md->obtn = md->button;

                md->last_press = current_press;
            }
        }

        /*Pass the status*/
        ms.hover = md->hover;
        ms.x = md->x;
        ms.y = md->y;

        /*Validate the command by checking if the coordinates have changed*/
        if(lx == md->cposx && ly == md->cposy)
        {
            ms.state = md->state;
            ms.button = md->button;
            ms.scroll_dir = md->scroll_dir;

            /*Was double click? if yes, the handle it*/
            if(delta >= 100 && delta <= 500)
            {
                md->last_press = 0;
                ms.state = mouse_button_state_2x;
                cw->mouse_func(cw, &ms);
                was_double = !!ms.handled;

                if(was_double == 0)
                    ms.state = mouse_button_state_pressed;
            }
        }

        if(was_double == 0)
        {
            mouse_button_state lmbs = md->state;
            mouse_button lmb = md->button;

            if(ms.hover == mouse_unhover)
            {
                ms.button = mouse_button_none;
                ms.state = mouse_button_state_none;
            }

            md->button = mouse_button_none;
            md->state = mouse_button_state_none;
            cw->mouse_func(cw, &ms);
            md->button = lmb;
            md->state = lmbs;
        }

        md->scroll_dir = 0;

        if(!!ms.handled)
        {
            md->button = mouse_button_none;
            md->state = mouse_button_state_none;
            md->last_press = 0;
        }
    }
    else
    {
        md->last_press = 0;
    }

    if(md->hover == mouse_unhover)
    {
        return(0);
    }

    /*left mouse was not handled so if the user will move the pointer while clicking,
     * then we'll move the window. Obvious, isn't it?
     * */
    if(md->button == mouse_button_left &&
            !ms.handled &&
            cw->draggable &&
            md->state == mouse_button_state_pressed)
    {
        long x = cw->x + (md->root_x - lx);
        long y = cw->y + (md->root_y  - ly);

        if(cw->x != x || cw->y != y)
        {
            md->drag = 1;


            crosswin_set_position(cw, x, y);
        }
    }

    return(0);
}


#if 0
static void crosswin_move_window(crosswin_window *w, long cx, long cy, unsigned char move)
{
    if(move > 1)
    {
        w->cposx =  w->cposx == 0 ? cx : w->cposx;
        w->cposy =  w->cposy == 0 ? cy : w->cposy;
    }

    if(move == 0)
    {
        w->dragging = 0;

    }
    else if(w->draggable && !w->mouse.handled)
    {
        long x = w->x + (cx - w->cposx);
        long y = w->y + (cy - w->cposy);

        if(w->x != x || w->y != y)
        {
            w->dragging = 1;
        }

        crosswin_set_position(w, x, y);
    }

    w->cposx = cx;
    w->cposy = cy;
}

#endif
