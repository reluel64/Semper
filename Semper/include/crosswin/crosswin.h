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

#define MOUSE_LEFT_BUTTON 0x1
#define MOUSE_RIGHT_BUTTON 0x2
#define MOUSE_MIDDLE_BUTTON 0x3
typedef struct _crosswin_window crosswin_window;

typedef enum
{
    crosswin_desktop,
    crosswin_bottom,
    crosswin_normal,
    crosswin_top,
    crosswin_topmost
} crosswin_position;

typedef struct _mouse_status
{
    long x; // x coordinate of the pointer
    long y; // y coordinate of the pointer
    int scroll_dir; // scroll direction 1->up -1->down
    int button; // click button
    int state; // 0-released 1-pressed 2-double clicked
    int hover;
} mouse_status;

/*Proper declaration*/

typedef struct
{
    long sw; // screen width
    long sh; // screen height
    unsigned char update_z;
    unsigned char quit;
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

    unsigned char opacity;
    void* user_data;
    void *kb_data;
    void* offscreen_buffer;
    long x;
    long y;
    long h;
    long w;
    long cposx; // current cursor x
    long cposy; // current cursor y
    long ccposx; // current cursor x with mouse held
    long ccposy; // current cursor y with mouse held
    unsigned char keep_on_screen;

    unsigned char zorder;
    size_t offw;
    size_t offh;
    mouse_status mouse;
    unsigned char dragging;
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
void crosswin_monitor_resolution(crosswin* c, long* w, long* h);
void crosswin_set_window_z_order(crosswin_window* w, unsigned char zorder);
void crosswin_set_kbd_handler(crosswin_window *w,int(*kbd_handler)(unsigned  int key_code,void *p),void *kb_data);
void crosswin_set_window_z_order(crosswin_window* w, unsigned char zorder);
