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
    crosswin_monitor *pm;            //parent monitor (used for keep on screen)
    size_t mon_cnt;
    long sw; // screen width
    long sh; // screen height
    unsigned char update_z;
    unsigned char quit;
    void *disp_fd;
    mouse_data md;
    int (*handle_mouse)(crosswin_window *w);
    crosswin_window *helper;
    list_entry windows;
#ifdef __linux__
    void *display;

    XVisualInfo vinfo;
#endif
} crosswin;

typedef struct _crosswin_window
{
    crosswin* c;
    list_entry current;
    size_t mon;

    unsigned char opacity;
    void* user_data;
    void *kb_data;
    void* offscreen_buffer;
    long x;
    long y;
    long h;
    long w;
    unsigned char keep_on_screen;
    unsigned char lock_z;
    crosswin_position zorder;
    size_t offw;
    size_t offh;
    void (*render_func)(crosswin_window* w, void* cr);
    int (*mouse_func)(crosswin_window* w, mouse_status* ms);
    int (*kbd_func)(unsigned  int key_code, void* ms);
    unsigned char draggable;

#ifdef WIN32
    HWND window;
#elif __linux__
    unsigned char click_through;
    unsigned char ctrl_down;
    Window window;
    Pixmap pixmap;
    XIM xInputMethod;
    XIC xInputContext;
#endif
} crosswin_window;

void crosswin_init(crosswin* c);
void crosswin_message_dispatch(crosswin *c);
void crosswin_set_window_data(crosswin_window* w, void* pv);
void* crosswin_get_window_data(crosswin_window* w);
void crosswin_click_through(crosswin_window* w, unsigned char state);
void crosswin_draw(crosswin_window* w);
void crosswin_set_render(crosswin_window* w, void (*render)(crosswin_window* w, void* cr));
void crosswin_set_position(crosswin_window* w, long x, long y);
void crosswin_set_opacity(crosswin_window* w, unsigned char opacity);
crosswin_window* crosswin_init_window(crosswin* c);
void crosswin_set_dimension(crosswin_window* w, long width, long height);
void crosswin_set_mouse_handler(crosswin_window* w, int (*mouse_handler)(crosswin_window* w, mouse_status* ms));
void crosswin_show(crosswin_window* w);
void crosswin_hide(crosswin_window* w);
void crosswin_destroy(crosswin_window** w);
void crosswin_get_position(crosswin_window* w, long* x, long* y);
void crosswin_draggable(crosswin_window* w, unsigned char draggable);
void crosswin_keep_on_screen(crosswin_window* w, unsigned char keep_on_screen);
int crosswin_update(crosswin* c);
void crosswin_monitor_resolution(crosswin* c, crosswin_window *cw, long* w, long* h);
void crosswin_set_window_z_order(crosswin_window* w, unsigned char zorder);
void crosswin_set_kbd_handler(crosswin_window *w,int(*kbd_handler)(unsigned  int key_code,void *p),void *kb_data);
void crosswin_set_window_z_order(crosswin_window* w, unsigned char zorder);
int crosswin_get_monitors(crosswin *c,crosswin_monitor **cm,size_t *len);
void crosswin_set_monitor(crosswin_window *w,size_t mon);
