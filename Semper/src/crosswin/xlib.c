#ifdef __linux__
#include <crosswin/crosswin.h>
#include <crosswin/xlib.h>
#include <string.h>
#include <stdlib.h>
#include <string_util.h>
#include <surface.h>
#include <event.h>
#include <mem.h>
#include <unistd.h>
#include <X11/extensions/Xinerama.h>
#define CONTEXT_ID 0x2712

typedef struct Hints
{
        unsigned long   flags;
        unsigned long   functions;
        unsigned long   decorations;
        long            inputMode;
        unsigned long   status;
} Hints;

static void xlib_render(crosswin_window *w);
void xlib_set_zpos(crosswin_window *w);

static int xlib_fixup_zpos(crosswin *c);

void xlib_init_display(crosswin *c)
{
    c->display = XOpenDisplay(NULL);
    c->disp_fd = (void*)(size_t)XConnectionNumber(c->display);
    XMatchVisualInfo(c->display, DefaultScreen(c->display), 32, TrueColor, &c->vinfo);
    c->colormap = (void*)XCreateColormap(c->display, DefaultRootWindow(c->display), c->vinfo.visual, AllocNone);
}

int xlib_get_monitors(crosswin *c, crosswin_monitor **cm, size_t *cnt)
{
    if(cm == NULL || cnt == NULL || c == NULL || c->display == NULL)
    {
        return(-1);
    }

    XineramaScreenInfo *sinfo = NULL;
    int lcount = 0;
    sinfo = XineramaQueryScreens(c->display, &lcount);

    if(sinfo)
    {
        crosswin_monitor *tcm = realloc(*cm,sizeof(crosswin_monitor) * ((*cnt)+lcount));
        if(tcm)
        {
            for(size_t i = 1; i < lcount+1; i++)
            {
                (tcm)[i].h = sinfo[i-1].height;
                (tcm)[i].w = sinfo[i-1].width;
                (tcm)[i].x = sinfo[i-1].x_org;
                (tcm)[i].y = sinfo[i-1].y_org;
                (tcm)[i].index = i;
            }


            (*cnt)+=lcount;
            *cm=tcm;
        }
        XFree(sinfo);
        return(0);
    }

    return(-1);

}

static void xlib_window_attributes(XSetWindowAttributes *xa,crosswin *c)
{
    memset(xa, 0, sizeof(XSetWindowAttributes));
    xa->colormap = (Colormap)c->colormap;
    xa->background_pixel = 0;
    xa->border_pixel = 0;
    xa->override_redirect = 0;
    xa->win_gravity=StaticGravity;
    xa->event_mask =
            KeyPressMask             |
            KeyReleaseMask           |
            KeymapStateMask          |
            StructureNotifyMask      |
            SubstructureNotifyMask   |
            SubstructureRedirectMask |
            ButtonReleaseMask        |
            EnterWindowMask          |
            LeaveWindowMask          |
            PointerMotionMask        |
            ButtonMotionMask         |
            VisibilityChangeMask     |
            FocusChangeMask          |
            ExposureMask             |
            PropertyChangeMask       |
            ButtonPressMask;
}

void xlib_init_window(crosswin_window *w)
{
    Atom type =0;
    Atom value = 0;
    XSetWindowAttributes attr;

    xlib_window_attributes(&attr,w->c);

    w->window = (void*)XCreateWindow(w->c->display ,
            DefaultRootWindow(w->c->display)         ,
            1                                        ,
            1                                        ,
            1                                        ,
            1                                        ,
            0                                        ,
            w->c->vinfo.depth                        ,
            InputOutput                              ,
            w->c->vinfo.visual                       ,
            CWBorderPixel                            |
            CWBackPixmap                             |
            CWBackPixel                              |
            CWColormap                               |
            CWEventMask                              |
            CWDontPropagate                          |
            CWOverrideRedirect                       |
            CWCursor 							     ,
            &attr
    );

    XSaveContext(w->c->display, (XID)w->window, CONTEXT_ID, (const char*)w);
    size_t pid = (size_t)getpid();

    Hints hints;
    Atom property;
    hints.flags = 2;
    hints.decorations = 0;
    property = XInternAtom(w->c->display, "_MOTIF_WM_HINTS", 1);
    XChangeProperty(w->c->display, (Window)w->window, property, property, 32, PropModeReplace, (unsigned char *)&hints, 5);


    type =  XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE",1);
    value = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE_UTILITY", 1);

    XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);

    type = XInternAtom(w->c->display,"_NET_WM_PID",1);
    XChangeProperty(w->c->display,  (Window)w->window, type, XA_CARDINAL, 32,PropModeReplace,  (unsigned char*) &pid, 1);
}

void xlib_set_dimmension(crosswin_window *w)
{
    if(w->w && w->h)
    {
        XWindowChanges wc = {0};
        wc.width = w->w;
        wc.height = w->h;
        XConfigureWindow(w->c->display,(Window) w->window, CWWidth | CWHeight, &wc);
    }
}

int xlib_set_opacity(crosswin_window *w)
{
    if(w->offscreen_buffer)
    {
        cairo_t *cr = cairo_create(w->xlib_surface);
        cairo_push_group(cr);
        cairo_set_source_surface(cr, w->offscreen_buffer, 0.0, 0.0);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint_with_alpha(cr,(double)w->opacity/255.0); //render it to the window
        cairo_pop_group_to_source(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);
        cairo_surface_flush(w->xlib_surface);
        cairo_destroy(cr); //destroy the context
        XSync(w->c->display,0);
        XFlush(w->c->display);
    }
    return(0);
}

void  xlib_set_position(crosswin_window *w)
{
    Atom type = XInternAtom(w->c->display, "_NET_MOVERESIZE_WINDOW", 1);
    XEvent ev={0};
    crosswin_monitor *cm = crosswin_get_monitor(w->c, w->mon);

    ev.xclient.data.l[0] = (StaticGravity) | (1<<8)|(1<<9);
    ev.xclient.message_type  = type;
    ev.xclient.type = ClientMessage;
    ev.xclient.format = 32;
    ev.xclient.data.l[1] = w->x+cm->x;
    ev.xclient.data.l[2] = w->y+cm->y;
    ev.xclient.window = (Window)w->window;
    ev.xclient.send_event=1;

    XSendEvent(w->c->display,DefaultRootWindow(w->c->display),0,  StructureNotifyMask|SubstructureNotifyMask   |  SubstructureRedirectMask,&ev);

    XSync(w->c->display,0);
}

void xlib_set_visible(crosswin_window *w)
{
    if(w->visible)
    {
        XMapWindow(w->c->display, (Window)w->window);
    }
    else
    {
        XUnmapWindow(w->c->display, (Window)w->window);
    }
}

static void xlib_render(crosswin_window *w)
{
    if(w->render_func)
    {
        if(w->offscreen_buffer == NULL)
        {
            w->pixmap = XCreatePixmap(w->c->display, (Window)w->window, w->w, w->h, 1);
            w->offscreen_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w->w, w->h);
            w->xlib_surface=cairo_xlib_surface_create(w->c->display, (Window)w->window, w->c->vinfo.visual, w->w, w->h);
            w->xlib_bitmap = cairo_xlib_surface_create_for_bitmap(w->c->display, w->pixmap, DefaultScreenOfDisplay(w->c->display), w->w, w->h);
            w->offw = (size_t)w->w;
            w->offh = (size_t)w->h;
        }
        else if(w->offscreen_buffer && (w->offw != (size_t)w->w || w->offh != (size_t)w->h))
        {
            cairo_surface_destroy(w->offscreen_buffer);
            cairo_surface_destroy(w->xlib_surface);
            cairo_surface_destroy(w->xlib_bitmap);
            XFreePixmap(w->c->display, w->pixmap);

            w->pixmap = XCreatePixmap(w->c->display,(Window) w->window, w->w, w->h, 1);
            w->xlib_bitmap = cairo_xlib_surface_create_for_bitmap(w->c->display, w->pixmap, DefaultScreenOfDisplay(w->c->display), w->w, w->h);
            w->offscreen_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w->w, w->h);
            w->xlib_surface=cairo_xlib_surface_create(w->c->display, (Window)w->window, w->c->vinfo.visual, w->w, w->h);

            w->offw = (size_t)w->w;
            w->offh = (size_t)w->h;
        }

        cairo_t* cr = cairo_create(w->offscreen_buffer); // create a cairo context to gain access to surface rendering routine
        w->render_func(w, cr);
        cairo_destroy(cr);

        /*Render everything to the window*/

        cr = cairo_create(w->xlib_surface);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_push_group(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(cr, w->offscreen_buffer, 0.0, 0.0);
        cairo_paint_with_alpha(cr,(double)w->opacity/255.0); //render it to the window
        cairo_pop_group_to_source(cr);
        cairo_paint(cr);

        cairo_surface_flush(w->xlib_surface);
        cairo_destroy(cr); //destroy the context
    }
}

void xlib_set_mask(crosswin_window *w)
{
    if(w->xlib_bitmap)
    {
        cairo_t *cr = cairo_create(w->xlib_bitmap);

        if(w->click_through == 0 && w->offscreen_buffer)
        {
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(cr, w->offscreen_buffer, 0.0, 0.0);
        }
        else
        {
            cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        }
        cairo_paint(cr); //render it to the window
        cairo_surface_flush(w->xlib_bitmap);
        XShapeCombineMask(w->c->display,(Window) w->window, ShapeInput, 0, 0, w->pixmap, ShapeSet);
        cairo_destroy(cr);
    }
}

void xlib_click_through(crosswin_window *w)
{
    xlib_set_mask(w);
}

void xlib_draw(crosswin_window* w)
{
    xlib_render(w);
    xlib_set_mask(w);
    XFlush(w->c->display);
}

int xlib_message_dispatch(crosswin *c)
{
    XEvent ev = {0};

    while(XPending(c->display)>0)
    {
        XEvent dev={0};
        crosswin_window *w = NULL;
        XNextEvent(c->display, &ev);
        XFindContext(c->display, ev.xany.window, CONTEXT_ID, (char**)&w);

        if(w == NULL)
        {
            continue;
        }

        switch(ev.xany.type)
        {
            case KeyPress:
                /*This is pretty ugly as I did not want to call the utf8_to_ucs() but for the moment it does the job*/
                if(ev.xkey.keycode == 105 || ev.xkey.keycode == 37)
                    c->md.ctrl = 1;

                if(w->xInputContext)
                {
                    unsigned short symbol = 0;

                    if(c->md.ctrl && ev.xkey.keycode == 36)
                        symbol = '\n';
                    else
                    {
                        unsigned char buf[9] = {0};
                        unsigned short *temp = NULL;
                        Status status = 0;

                        Xutf8LookupString(w->xInputContext, &ev.xkey, (char*)&buf, 8, NULL, &status);

                        temp = utf8_to_ucs(buf);
                        symbol = temp ? temp[0] : 0;

                        sfree((void**)&temp);
                    }

                    if(symbol && w->kbd_func)
                        w->kbd_func(symbol, w->kb_data);
                }
                break;

            case KeyRelease:
                if(ev.xkey.keycode == 105 || ev.xkey.keycode == 37)
                    c->md.ctrl = 0;
                break;

            case EnterNotify:
            case MotionNotify:
            {

                c->flags|=CROSSWIN_CHECK_ZORDER_FOR_FIX;
                /*Get the latest MotionNotify*/
                while(XCheckTypedWindowEvent(w->c->display,(Window)w->window,MotionNotify,&dev))
                {
                    if(dev.xmotion.time>=ev.xmotion.time)
                    {
                        ev=dev;
                    }
                }
                c->md.x = ev.xmotion.x;
                c->md.y = ev.xmotion.y;
                c->md.root_x = ev.xmotion.x_root;
                c->md.root_y = ev.xmotion.y_root;
                c->md.hover = mouse_hover;
                w->c->handle_mouse(w);
                break;
            }
            case ButtonRelease:
            {
                c->md.x = ev.xbutton.x;
                c->md.y = ev.xbutton.y;
                c->md.state = mouse_button_state_unpressed;

                switch(ev.xbutton.button)
                {
                    case Button1:
                        c->md.button = mouse_button_left;
                        break;

                    case Button2:
                        c->md.button = mouse_button_middle;
                        break;

                    case Button3:
                        c->md.button = mouse_button_right;
                        break;

                    case Button4:
                        c->md.state = mouse_button_state_none;
                        c->md.scroll_dir = -1;
                        break;

                    case Button5:
                        c->md.state = mouse_button_state_none;
                        c->md.scroll_dir = 1;
                        break;
                }

                w->c->handle_mouse(w);
                break;
            }

            case ButtonPress:
            {
               // XSetInputFocus(w->c->display, (Window)w->window, RevertToParent, 0);
                c->md.x = ev.xbutton.x;
                c->md.y = ev.xbutton.y;
                c->md.state = mouse_button_state_pressed;
                c->flags|=CROSSWIN_CHECK_ZORDER_FOR_FIX;
                switch(ev.xbutton.button)
                {
                    case Button1:
                        c->md.button = mouse_button_left;
                        break;

                    case Button2:
                        c->md.button = mouse_button_middle;
                        break;

                    case Button3:
                        c->md.button = mouse_button_right;
                        break;
                }

                w->c->handle_mouse(w);
                break;
            }
            case LeaveNotify:
                XSetInputFocus(w->c->display, None, RevertToParent, 0);
                c->md.hover = mouse_unhover;
                w->c->handle_mouse(w);
                break;

            case DestroyNotify:
            {
                surface_data *sd = w->user_data;
                surface_destroy(sd);
                break;
            }
            case Expose:
                xlib_set_position(w);
                c->flags = CROSSWIN_UPDATE_MONITORS;
                break;
            case FocusIn:
            case ReparentNotify:
            case ConfigureNotify:
                c->flags|=CROSSWIN_CHECK_ZORDER_FOR_FIX;
                break;

        }
    }

    return(0);
}

void xlib_destroy_window(crosswin_window **w)
{
    Display *disp =(*w)->c->display;
    xlib_destroy_input_context(*w);

    cairo_surface_destroy((*w)->xlib_surface);
    cairo_surface_destroy((*w)->xlib_bitmap);

    XDeleteContext((*w)->c->display, (XID)(*w)->window, CONTEXT_ID);
    XDestroyWindow((*w)->c->display, (Window)(*w)->window);
    XFreePixmap((*w)->c->display, (*w)->pixmap);

    XFlush(disp);
}

static void xlib_set_window_active(crosswin_window *w)
{
    Atom active_win =  XInternAtom(w->c->display, "_NET_ACTIVE_WINDOW", 1);
    XEvent ev={0};

    ev.xclient.message_type  = active_win;
    ev.xclient.format = 32;
    ev.xclient.type=ClientMessage;
    ev.xclient.window = (Window)w->window;
    ev.xclient.send_event=1;
    XSendEvent(w->c->display,DefaultRootWindow(w->c->display),0,  StructureNotifyMask|SubstructureNotifyMask   ,&ev);

}

static void xlib_set_window_below(crosswin_window *w)
{
    Atom state =  XInternAtom(w->c->display, "_NET_WM_STATE", 1);
    Atom actual_state =  XInternAtom(w->c->display, "_NET_WM_STATE_BELOW", 1);
    XEvent ev={0};

    ev.xclient.message_type  = state;
    ev.xclient.format = 32;
    ev.xclient.type=ClientMessage;
    ev.xclient.window = (Window)w->window;
    ev.xclient.send_event=1;
    ev.xclient.data.l[0] = 1;
    ev.xclient.data.l[1] = actual_state;
    XSendEvent(w->c->display,DefaultRootWindow(w->c->display),0,  StructureNotifyMask|SubstructureNotifyMask   ,&ev);
}

static void xlib_set_window_above(crosswin_window *w)
{
    Atom state =  XInternAtom(w->c->display, "_NET_WM_STATE", 1);
    Atom actual_state =  XInternAtom(w->c->display, "_NET_WM_STATE_ABOVE", 1);
    XEvent ev={0};

    ev.xclient.message_type  = state;
    ev.xclient.format = 32;
    ev.xclient.type=ClientMessage;
    ev.xclient.window = (Window)w->window;
    ev.xclient.send_event=1;
    ev.xclient.data.l[0] = 1;
    ev.xclient.data.l[1] = actual_state;
    XSendEvent(w->c->display,DefaultRootWindow(w->c->display),0,  StructureNotifyMask|SubstructureNotifyMask, &ev);
}


static int xlib_fixup_zpos(crosswin *c)
{

    int ret_val = 0;
    Window *stack = NULL;
    Window *server = NULL;
    size_t server_cnt=0;
    size_t stack_cnt = 0;
    unsigned int chld_count=0;

    Atom type = XInternAtom(c->display, "_NET_CLIENT_LIST_STACKING", 1);
    Atom ret=0;
    int fmt_ret=0;
    unsigned long item_ret=0;
    unsigned long bret=0;
    Window * chld=NULL;

    if(XGetWindowProperty(c->display,DefaultRootWindow(c->display),type,0,10000,0,XA_WINDOW,&ret,&fmt_ret,&item_ret,&bret,(unsigned char**)&chld))
    {
        return(0);
    }

    chld_count=item_ret;
    crosswin_window *cw = NULL;

    list_enum_part(cw,&c->windows,current)
    {
        Window *tmp = realloc(stack,sizeof(Window)*(stack_cnt+1));

        if(tmp)
        {
            stack = tmp;
            stack[stack_cnt++] = (Window)cw->window;
        }
    }

    for(unsigned int i=0;i<chld_count;i++)
    {

        list_enum_part(cw,&c->windows,current)
        {
            if((Window)cw->window == chld[i])
            {
                Window *tmp = realloc(server,sizeof(Window)*(server_cnt+1));

                if(tmp)
                {
                    server = tmp;
                    server[server_cnt++] = (Window)cw->window;
                }
                break;
            }
         }
    }

    XFree(chld);

    ret_val = (server_cnt==stack_cnt) && !!memcmp(stack,server,sizeof(Window)*stack_cnt);
    sfree((void**)&stack);
    sfree((void**)&server);

    return(ret_val);
}


void xlib_set_zpos(crosswin_window *w)
{
    Atom state = XInternAtom(w->c->display, "_NET_WM_STATE", 1);
    Atom abv = XInternAtom(w->c->display, "_NET_WM_STATE_ABOVE", 1);
    Atom type =  XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE",1);
    Atom value = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE_UTILITY", 1);
    Atom skip_taskbar = XInternAtom(w->c->display,"_NET_WM_STATE_SKIP_TASKBAR",1);
    Atom skip_pager = XInternAtom(w->c->display,"_NET_WM_STATE_SKIP_PAGER",1);
    Atom win_style[2] = {skip_pager,skip_taskbar};

    XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);
    XChangeProperty(w->c->display, (Window)w->window, state, XA_ATOM, 32, PropModeReplace, (unsigned char*)win_style, sizeof(win_style)/sizeof(Atom));

    switch(w->zorder)
    {
        case crosswin_normal:
        {
            XEvent ev={0};
            ev.xclient.message_type  = state;
            ev.xclient.format = 32;
            ev.xclient.type=ClientMessage;
            ev.xclient.window = (Window)w->window;
            ev.xclient.send_event=1;
            ev.xclient.data.l[0] = !!(w->c->flags&CROSSWIN_SHOW_DESKTOP);
            ev.xclient.data.l[1] = abv;
            XSendEvent(w->c->display,DefaultRootWindow(w->c->display),0,  StructureNotifyMask|SubstructureNotifyMask   ,&ev);
        }
        break;

        case crosswin_desktop:
        case crosswin_bottom:
            xlib_set_window_below(w);
            if((w->c->flags & CROSSWIN_FIX_ZORDER))
            {
                xlib_set_window_active(w);
            }
            break;
        case crosswin_top:
        case crosswin_topmost:
            xlib_set_window_above(w);
            if((w->c->flags & CROSSWIN_FIX_ZORDER))
            {
                xlib_set_window_active(w);
            }

            break;
    }
    XSync(w->c->display,0);
    XFlush(w->c->display);
}


void xlib_check_desktop(crosswin *c)
{
    if((c->flags & CROSSWIN_CHECK_ZORDER_FOR_FIX) && xlib_fixup_zpos(c))
    {
        c->flags |= (CROSSWIN_UPDATE_ZORDER|CROSSWIN_FIX_ZORDER);
    }

    c->flags&=~CROSSWIN_CHECK_ZORDER_FOR_FIX;
    Atom type = XInternAtom(c->display, "_NET_SHOWING_DESKTOP", 1);
    Atom ret=0;
    int fmt_ret=0;
    unsigned long item_ret=0;
    unsigned long bret=0;
    unsigned char * re=0;
    if(!XGetWindowProperty(c->display,DefaultRootWindow(c->display),type,0,1,0,XA_CARDINAL,&ret,&fmt_ret,&item_ret,&bret,&re))
    {
        if(re)
        {
            if(((!(*re) && (c->flags&CROSSWIN_SHOW_DESKTOP))||((*re) && !(c->flags&CROSSWIN_SHOW_DESKTOP))))
            {
                c->flags|=CROSSWIN_UPDATE_ZORDER|CROSSWIN_FIX_ZORDER;

                if(c->flags&CROSSWIN_SHOW_DESKTOP)
                    c->flags&=~CROSSWIN_SHOW_DESKTOP;
                else
                    c->flags|=CROSSWIN_SHOW_DESKTOP;
            }
        }
        XFree(re);
    }
}

int xlib_create_input_context(crosswin_window *w)
{
    if(w->xInputContext || w->xInputMethod)
        return(0);

    w->xInputMethod = XOpenIM(w->c->display, 0, 0, 0);

    if(w->xInputMethod)
    {
        XIMStyles* styles = 0;

        if(XGetIMValues(w->xInputMethod, XNQueryInputStyle, &styles, NULL) == 0)
        {
            XIMStyle bestMatchStyle = 0;

            for(int i = 0; i < styles->count_styles; i++)
            {
                if(styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing))
                {
                    bestMatchStyle =  styles->supported_styles[i];
                    break;
                }
            }

            XFree(styles);
            w->xInputContext = XCreateIC(w->xInputMethod, XNInputStyle, bestMatchStyle,  XNClientWindow, w->window, XNFocusWindow, w->window, NULL);
        }
    }

    if(w->xInputContext == NULL && w->xInputMethod)
    {
        XCloseIM(w->xInputMethod);
        w->xInputMethod = NULL;
        return(-1);
    }

    return(0);
}

int xlib_destroy_input_context(crosswin_window *w)
{
    w->xInputContext ? XDestroyIC(w->xInputContext) : 0;
    w->xInputMethod ? XCloseIM(w->xInputMethod) : 0;
    w->xInputContext = NULL;
    w->xInputMethod = NULL;

    return(0);
}

#endif
