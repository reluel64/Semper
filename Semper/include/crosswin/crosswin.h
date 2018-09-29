#pragma once
#ifdef WIN32
#include <windows.h>
#endif

#include <stddef.h>
#include <linked_list.h>
#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>
#include <X11/extensions/shape.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#endif

#include <event.h>
#define CROSSWIN_UPDATE_MONITORS     (1<<0)
#define CROSSWIN_UPDATE_ZORDER       (1<<1)
#define CROSSWIN_UPDATE_SHOW_DESKTOP (1<<2)
#define CROSSWIN_UPDATE_FIX_ZORDER   (1<<3)
typedef struct _crosswin_window crosswin_window;

typedef enum
{
    crosswin_desktop,
    crosswin_bottom,
    crosswin_normal,
    crosswin_top,
    crosswin_topmost
} crosswin_position;

typedef enum
{
    mouse_button_none,
    mouse_button_left,
    mouse_button_middle,
    mouse_button_right
} mouse_button;

typedef enum
{
    mouse_button_state_none,
    mouse_button_state_unpressed,
    mouse_button_state_pressed,
    mouse_button_state_2x
} mouse_button_state;

typedef enum
{
    mouse_none,
    mouse_unhover,
    mouse_hover
} mouse_hover_state;

typedef struct _mouse_status
{
    long x;                 // x coordinate of the pointer
    long y;                 // y coordinate of the pointer
    char scroll_dir;        // scroll direction 1->up -1->down
    mouse_button button;
    mouse_button_state state;
    mouse_hover_state hover;
    unsigned char handled;
} mouse_status;

typedef struct
{
    long x;                 // x coordinate of the pointer
    long y;                 // y coordinate of the pointer
    long root_x;
    long root_y;
    char scroll_dir;        // scroll direction 1->up -1->down
    long cposx; // current cursor x
    long cposy; // current cursor y
    mouse_button button;
    mouse_button obtn;
    size_t last_press;
    mouse_button_state state;
    mouse_hover_state hover;
    unsigned char ctrl;
    unsigned char drag;
} mouse_data;

/*Proper declaration*/

typedef struct
{
    long x;
    long y;
    long w;
    long h;
    size_t index;
} crosswin_monitor;


typedef struct
{
    unsigned char update;
    unsigned char show_desktop;
    event_queue *eq;
    size_t top_win_id;
    mouse_data md;
    int (*handle_mouse)(crosswin_window *w);
    list_entry windows;
    crosswin_monitor *pm;            //parent monitor (used for keep on screen)
    size_t mon_cnt;
    void *cd;
#if defined(WIN32)
    void *show_desktop_window;
#endif
#if defined(__linux__)
    void *display;
    void *disp_fd;
    XVisualInfo vinfo;
    void *colormap;
#endif
} crosswin;

typedef struct _crosswin_window
{
    crosswin* c;
    list_entry current;
    size_t mon;

    unsigned char visible;
    unsigned char opacity;
    void *user_data;
    void *kb_data;
    void *offscreen_buffer;
    long x;
    long y;
    long h;
    long w;
    unsigned char keep_on_screen;
    crosswin_position zorder;
    size_t offw;
    size_t offh;
    void (*render_func)(crosswin_window* w, void* cr);
    int (*mouse_func)(crosswin_window* w, mouse_status* ms);
    int (*kbd_func)(unsigned  int key_code, void* ms);
    unsigned char draggable;
    unsigned char click_through;
    void *window;

#if defined(__linux__)
    unsigned char ctrl_down;
    unsigned char upper;
    void *xlib_surface;
    void *xlib_bitmap;
    Pixmap pixmap;
    XIM xInputMethod;
    XIC xInputContext;

#endif

} crosswin_window;

void crosswin_update_done(crosswin *c, unsigned char flag);
void crosswin_init(crosswin* c);
void crosswin_message_dispatch(crosswin *c);
void crosswin_set_window_data(crosswin_window* w, void* pv);
void* crosswin_get_window_data(crosswin_window* w);
void crosswin_set_click_through(crosswin_window* w, unsigned char state);
void crosswin_get_click_through(crosswin_window* w, unsigned char *state);
void crosswin_draw(crosswin_window* w);
void crosswin_set_render(crosswin_window* w, void (*render)(crosswin_window* w, void* cr));
void crosswin_set_position(crosswin_window* w, long x, long y);
void crosswin_set_opacity(crosswin_window* w, unsigned char opacity);
void crosswin_get_opacity(crosswin_window* w, unsigned char *opacity);
crosswin_window* crosswin_init_window(crosswin* c);
void crosswin_set_size(crosswin_window* w, long width, long height);
void crosswin_set_mouse_handler(crosswin_window* w, int (*mouse_handler)(crosswin_window* w, mouse_status* ms));
void crosswin_destroy(crosswin_window** w);
void crosswin_get_position(crosswin_window* w, long* x, long* y, size_t *monitor);
void crosswin_get_size(crosswin_window* cw, long* w, long* h);
void crosswin_get_draggable(crosswin_window* w, unsigned char *draggable);
void crosswin_set_draggable(crosswin_window* w, unsigned char draggable);
void crosswin_set_keep_on_screen(crosswin_window* w, unsigned char keep_on_screen);
void crosswin_get_keep_on_screen(crosswin_window* w, unsigned char *keep_on_screen);
int crosswin_update(crosswin* c);
void crosswin_monitor_resolution(crosswin* c, crosswin_window *cw, long* w, long* h);
void crosswin_monitor_origin(crosswin *c, crosswin_window *cw, long *x, long *y);
void crosswin_set_kbd_handler(crosswin_window *w, int(*kbd_handler)(unsigned  int key_code, void *p), void *kb_data);
void crosswin_set_zorder(crosswin_window* w, unsigned char zorder);
void crosswin_get_zorder(crosswin_window* w, unsigned char *zorder);
int crosswin_get_monitors(crosswin *c, crosswin_monitor **cm, size_t *len);
void crosswin_set_monitor(crosswin_window *w, size_t mon);
void crosswin_set_visible(crosswin_window* w,unsigned char visible);
void crosswin_get_visible(crosswin_window* w,unsigned char *visible);
crosswin_monitor *crosswin_get_monitor(crosswin *c, size_t index);
