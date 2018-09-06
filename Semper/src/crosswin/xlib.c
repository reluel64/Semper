#ifdef __linux__
#include <crosswin/crosswin.h>
#include <crosswin/xlib.h>
#include <string.h>
#include <stdlib.h>
#include <string_util.h>
#include <surface.h>
#include <event.h>
#include <mem.h>
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

void xlib_init_display(crosswin *c)
{
    c->display = XOpenDisplay(NULL);
    c->disp_fd = (void*)(size_t)XConnectionNumber(c->display);
    XMatchVisualInfo(c->display, DefaultScreen(c->display), 32, TrueColor, &c->vinfo);
}

int xlib_get_monitors(crosswin *c, crosswin_monitor **cm, size_t *cnt)
{
    if(cm == NULL || cnt == NULL || c == NULL || c->display == NULL)
    {
        return(-1);
    }

    XineramaScreenInfo *sinfo = NULL;

    sinfo = XineramaQueryScreens(c->display, (int*)cnt);

    if(sinfo)
    {
        *cm = zmalloc(sizeof(crosswin_monitor) * (*cnt));

        for(size_t i = 0; i < *cnt; i++)
        {
            (*cm)[i].h = sinfo[i].height;
            (*cm)[i].w = sinfo[i].width;
            (*cm)[i].x = sinfo[i].x_org;
            (*cm)[i].y = sinfo[i].y_org;
            (*cm)[i].index = i;
        }

        XFree(sinfo);
        return(0);
    }

    return(-1);

}


void xlib_init_window(crosswin_window *w)
{
    XSetWindowAttributes attr;
    Atom type =0;
    Atom value = 0;
    memset(&attr, 0, sizeof(attr));
    attr.colormap = XCreateColormap(w->c->display, DefaultRootWindow(w->c->display), w->c->vinfo.visual, AllocNone);
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.override_redirect = 0;
    attr.win_gravity=StaticGravity;
    attr.event_mask =
            KeyPressMask             |
            KeymapStateMask          |
            StructureNotifyMask      |
            SubstructureNotifyMask   |
            SubstructureRedirectMask |
            ButtonReleaseMask        |
            KeyReleaseMask           |
            EnterWindowMask          |
            LeaveWindowMask          |
            PointerMotionMask        |
            ButtonMotionMask         |
            VisibilityChangeMask     |
            FocusChangeMask          |
            ExposureMask             |
            PropertyChangeMask       |
            ButtonPressMask;

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
            CWCursor 							     ,
            &attr
    );


    XSaveContext(w->c->display, (XID)w->window, CONTEXT_ID, (const char*)w);

    type = XInternAtom(w->c->display, "_NET_WM_STATE", 1);
    value = XInternAtom(w->c->display, "_NET_WM_STATE_SKIP_TASKBAR", 1);
    XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);

    type = XInternAtom(w->c->display, "_NET_WM_STATE", 1);
    value = XInternAtom(w->c->display, "_NET_WM_STATE_SKIP_PAGER", 1);
    XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeAppend, (unsigned char*)&value, 1);

    Hints hints;
    Atom property;
    hints.flags = 2;
    hints.decorations = 0;
    property = XInternAtom(w->c->display, "_MOTIF_WM_HINTS", 1);
    XChangeProperty(w->c->display, (Window)w->window, property, property, 32, PropModeReplace, (unsigned char *)&hints, 5);

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
        XFlush(w->c->display);
    }
    return(0);
}

void  xlib_set_position(crosswin_window *w)
{
    Atom type = XInternAtom(w->c->display, "_NET_MOVERESIZE_WINDOW", 1);
    XEvent ev={0};

    ev.xclient.data.l[0] = (StaticGravity) | (1<<8)|(1<<9);
    ev.xclient.message_type  = type;
    ev.xclient.type = ClientMessage;
    ev.xclient.format = 32;
    ev.xclient.data.l[1] = w->x;
    ev.xclient.data.l[2] = w->y;
    ev.xclient.window = (Window)w->window;
    ev.xclient.send_event=1;

    XSendEvent(w->c->display,DefaultRootWindow(w->c->display),0,  StructureNotifyMask|SubstructureNotifyMask   |  SubstructureRedirectMask,&ev);

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
        cairo_set_source_surface(cr, w->offscreen_buffer, 0.0, 0.0);
        cairo_paint_with_alpha(cr,(double)w->opacity/255.0); //render it to the window
        cairo_destroy(cr); //destroy the context
        cairo_surface_flush(w->xlib_surface);
    }
}

void xlib_set_mask(crosswin_window *w)
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
        XSync(c->display,0);
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
                XEvent dev={0};
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
                XSetInputFocus(w->c->display, (Window)w->window, RevertToParent, 0);
                c->md.x = ev.xbutton.x;
                c->md.y = ev.xbutton.y;
                c->md.state = mouse_button_state_pressed;

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
                c->update = 1;
                break;
            case ReparentNotify:
            case ConfigureNotify:
                xlib_set_zpos(w);
                break;

            case FocusIn:
                xlib_set_zpos(w);
                break;
            case FocusOut:
            {
                    unsigned int tzo = w->zorder;
                    w->zorder = crosswin_normal;
                    xlib_set_zpos(w);
                    w->zorder = tzo;

            }
            break;
        }
        XSync(c->display,0);
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


static void xlib_set_zpos_stage2(crosswin_window *w)
{
    Atom type =0;
       Atom value = 0;

       switch(w->zorder)
       {
           case crosswin_top:


               XSync(w->c->display,0);
               XLowerWindow(w->c->display,w->window);
               XSync(w->c->display,0);
               break;


       }
}

void xlib_set_zpos(crosswin_window *w)
{
#if 0
    Window root;
    Window parent;
    Window *chld;
    int chld_count=0;

    XQueryTree(w->c->display,DefaultRootWindow(w->c->display),&root,&parent,&chld,&chld_count);
#endif
    Atom type =0;
    Atom value = 0;
    surface_data *sd = w->user_data;

    switch(w->zorder)
    {
        case crosswin_normal:
            type = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE", 1);
            value = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE_NORMAL", 1);
            XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);
            XSync(w->c->display,0);
            break;

        case crosswin_desktop:
            type = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE", 1);
            value = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE_DESKTOP", 1);
            XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);
            break;

        case crosswin_bottom:
            break;

        case crosswin_topmost:
            type = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE", 1);
            value = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE_DOCK", 1);
            XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);
            XSync(w->c->display,0);
            break;

        case crosswin_top:
            type = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE", 1);
            value = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE_DOCK", 1);
            XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);
            XSync(w->c->display,0);
            type = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE", 1);
            value = XInternAtom(w->c->display, "_NET_WM_WINDOW_TYPE_NORMAL", 1);
            XChangeProperty(w->c->display, (Window)w->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&value, 1);
            break;
    }
}


void xlib_check_desktop(crosswin *c)
{

    #warning "Not implemented"
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
