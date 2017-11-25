#ifdef __linux__
#include <crosswin/crosswin.h>
#include <crosswin/xlib.h>
#include <string.h>
#include <stdlib.h>
#include <string_util.h>
#include <surface.h>
#include <event.h>
#include <mem.h>
#define CONTEXT_ID 0x201

void xlib_init_display(crosswin *c)
{
    c->display=XOpenDisplay(NULL);
    XMatchVisualInfo(c->display, DefaultScreen(c->display), 32, TrueColor, &c->vinfo);
}
typedef struct Hints
{
    unsigned long   flags;
    unsigned long   functions;
    unsigned long   decorations;
    long            inputMode;
    unsigned long   status;
} Hints;
static void xlib_set_above(window *w,unsigned char clear);
static void xlib_set_below(window *w,unsigned char clear);
void xlib_init_window(window *w)
{
    XSetWindowAttributes attr;

    memset(&attr,0,sizeof(attr));
    attr.colormap = XCreateColormap(w->c->display, DefaultRootWindow(w->c->display), w->c->vinfo.visual, AllocNone);
    attr.background_pixel=0;
    attr.border_pixel=0;
    attr.override_redirect=0;
    attr.event_mask=KeyPressMask 		    |
                    KeymapStateMask         |
                    StructureNotifyMask     |
                    SubstructureNotifyMask  |
                    SubstructureRedirectMask|
                    ButtonReleaseMask 	    |
                    KeyReleaseMask 		    |
                    EnterWindowMask 	    |
                    LeaveWindowMask 	    |
                    PointerMotionMask 	    |
                    ButtonMotionMask 	    |
                    VisibilityChangeMask    |
                    ButtonPressMask;

    w->window = XCreateWindow(w->c->display						,
                              DefaultRootWindow(w->c->display)	,
                              1									,
                              1									,
                              1									,
                              1									,
                              0									,
                              w->c->vinfo.depth					,
                              InputOutput						,
                              w->c->vinfo.visual,
                              CWBorderPixel						|
                              CWOverrideRedirect                |
                              CWBackPixmap                      |
                              CWBackPixel                       |
                              CWColormap  						|
                              CWEventMask						|
                              CWCursor 							,
                              &attr
                             );


    XSaveContext(w->c->display,(XID)w->window,CONTEXT_ID,(const char*)w);
    Atom type = XInternAtom(w->c->display,"_NET_WM_WINDOW_TYPE", False);
    Atom value = XInternAtom(w->c->display,"_NET_WM_WINDOW_TYPE_NORMAL", False);
    XChangeProperty(w->c->display, w->window, type, XA_ATOM, 32, PropModeReplace,(unsigned char*)&value, 1);

    Hints hints;
    Atom property;
    hints.flags = 2;
    hints.decorations = 0;
    property = XInternAtom(w->c->display, "_MOTIF_WM_HINTS", 1);
    XChangeProperty(w->c->display,w->window,property,property,32,PropModeReplace,(unsigned char *)&hints,5);

    XMapWindow(w->c->display, w->window);

    /*Create the input context*/


}

void xlib_set_dimmension(window *w)
{
    XWindowChanges wc= {0};
    wc.width=w->w;
    wc.height=w->h;
    XConfigureWindow(w->c->display,w->window,CWWidth|CWHeight,&wc);
}


static void xlib_render(window *w)
{
    if(w->render_func)
    {
        if(w->offscreen_buffer == NULL)
        {
            w->offscreen_buffer=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w->w,w->h);
            w->pixmap=XCreatePixmap(w->c->display,w->window,w->w,w->h,1);
            w->offw = (size_t)w->w;
            w->offh = (size_t)w->h;
        }
        else if(w->offscreen_buffer && (w->offw != (size_t)w->w || w->offh != (size_t)w->h))
        {
            cairo_surface_destroy(w->offscreen_buffer);
            XFreePixmap(w->c->display,w->pixmap);
            w->pixmap=XCreatePixmap(w->c->display,w->window,w->w,w->h,1);
            w->offscreen_buffer=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w->w,w->h);
            w->offw = (size_t)w->w;
            w->offh = (size_t)w->h;
        }

        cairo_t* cr= cairo_create(w->offscreen_buffer); // create a cairo context to gain access to surface rendering routine

        if(w->opacity!=255)
        {
            cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
            cairo_paint(cr);
            cairo_push_group(cr); //push the first group (do the rendering here)

            w->render_func(w, cr);
            cairo_pop_group_to_source(cr); //pop the second group and set it as source

            cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE); //we want the source
            cairo_paint_with_alpha(cr,(double)w->opacity/255.0); //render it with alpha

        }
        else
        {
            w->render_func(w, cr);
        }

        cairo_destroy(cr);
        /*Render everything to the window*/
        cairo_surface_t *xs=cairo_xlib_surface_create(w->c->display,w->window,w->c->vinfo.visual,w->w,w->h);
        cr=cairo_create(xs);
        cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(cr,w->offscreen_buffer,0.0,0.0);
        cairo_paint(cr); //render it to the window

        cairo_destroy(cr); //destroy the context
        cairo_surface_destroy(xs);
    }
}

void xlib_set_mask(window *w)
{

    cairo_surface_t *xs=cairo_xlib_surface_create_for_bitmap (w->c->display,w->pixmap,DefaultScreenOfDisplay(w->c->display),w->w,w->h);
    cairo_t *cr=cairo_create(xs);

    if(w->click_through==0&&w->offscreen_buffer)
    {
        cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
        unsigned int *buf=(unsigned int*)cairo_image_surface_get_data(w->offscreen_buffer);

        if(buf)
        {

            for(size_t i=0; i<(size_t)w->w*(size_t)w->h; i++)
            {
                if(buf[i]&0xff000000)
                {
                    buf[i]|=0xff000000;
                }
            }

            cairo_surface_mark_dirty(w->offscreen_buffer);
        }

        cairo_set_source_surface(cr,w->offscreen_buffer,0.0,0.0);
        cairo_paint(cr); //render it to the window
    }
    else
    {
        cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr);
    }

    XShapeCombineMask(w->c->display,w->window,ShapeInput,0,0,w->pixmap,ShapeSet);

    cairo_destroy(cr);
    cairo_surface_destroy(xs);
}

void xlib_draw(window* w)
{
    xlib_render(w);
    xlib_set_mask(w);
}

int xlib_mouse_handle(window *w,XEvent *ev)
{

    w->mouse.button=ev->xbutton.button;
    w->mouse.scroll_dir=0;
    w->mouse.x=ev->xbutton.x;
    w->mouse.y=ev->xbutton.y;
    w->mouse.state=-1;

    switch(ev->xany.type)
    {
        case ButtonPress:
        {

            w->mouse.state=1;

            if(ev->xbutton.button==2)
                w->mouse.button=3;
            else if(ev->xbutton.button==3)
                w->mouse.button=2;

            if(ev->xbutton.button==Button5||ev->xbutton.button==Button4)
            {
                w->mouse.state=-1;
                w->mouse.button=0;
                w->mouse.scroll_dir=(ev->xbutton.button==Button5?-1:1);
            }

            break;
        }

        case ButtonRelease:
        {
            if(w->dragging)
            {
                w->dragging=0;
                return(0);
            }

            XWindowChanges wc= {0};
            wc.stack_mode=Above;

            if(w->mouse.hover==0)
                XConfigureWindow(w->c->display,w->window,CWStackMode,&wc);

            w->mouse.state=0;

            if(ev->xbutton.button==2)
                w->mouse.button=3;
            else if(ev->xbutton.button==3)
                w->mouse.button=2;

            break;
        }

        case MotionNotify:
        {
            w->mouse.hover=1;
            break;
        }

        case LeaveNotify:
        {
            w->mouse.hover=0;

            memset(&w->mouse,0,sizeof(mouse_status));

        }
        break;

    }

    w->mouse_func(w,&w->mouse);
    return(0);
}

static void xlib_set_above(window *w,unsigned char clear)
{
    if(clear==0)
    {
        xlib_set_below(w,1);
    }

    XEvent xev;
    memset(&xev,0,sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.window = w->window;
    xev.xclient.message_type = XInternAtom(w->c->display,"_NET_WM_STATE", False);
    xev.xclient.serial = 0;
    xev.xclient.display = w->c->display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1; /* x coord */
    xev.xclient.data.l[1] = XInternAtom(w->c->display,"_NET_WM_STATE_ABOVE", False); /* y coord */
    xev.xclient.data.l[2] = 0; /* direction */
    xev.xclient.data.l[3] =0; /* button */
    // XSendEvent(w->c->display, DefaultRootWindow(w->c->display), False, SubstructureRedirectMask|SubstructureNotifyMask, &xev);
}

static void xlib_set_below(window *w,unsigned char clear)
{
    if(clear==0)
    {
        xlib_set_above(w,1);
    }

    XEvent xev;
    memset(&xev,0,sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.window = w->window;
    xev.xclient.message_type = XInternAtom(w->c->display,"_NET_WM_STATE", False);
    xev.xclient.serial = 0;
    xev.xclient.display = w->c->display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1; /* x coord */
    xev.xclient.data.l[1] = XInternAtom(w->c->display,"_NET_WM_STATE_BELOW", False); /* y coord */
    xev.xclient.data.l[2] = 0; /* direction */
    xev.xclient.data.l[3] =0; /* button */
    // XSendEvent(w->c->display, DefaultRootWindow(w->c->display), False, SubstructureRedirectMask|SubstructureNotifyMask, &xev);
}

static void xlib_set_normal(window *w)
{
    xlib_set_below(w,1);
    xlib_set_above(w,1);
    XRaiseWindow(w->c->display,w->window);
}

static int xlib_move_internal(int x,int y,window *w,unsigned char start)
{
    if(w->dragging==start)
        return(0);

    w->dragging=start;
    XUngrabPointer(w->c->display,0);

    XEvent xev;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = w->window;
    xev.xclient.message_type = XInternAtom(w->c->display,"_NET_WM_MOVERESIZE", False);
    xev.xclient.serial = 0;
    xev.xclient.display = w->c->display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = x; /* x coord */
    xev.xclient.data.l[1] = y; /* y coord */
    xev.xclient.data.l[2] = start?8:11; /* direction */
    xev.xclient.data.l[3] = Button1Mask; /* button */
    xev.xclient.data.l[4] = 0; /* unused */
    XSendEvent(w->c->display, DefaultRootWindow(w->c->display), False, SubstructureRedirectMask|SubstructureNotifyMask, &xev);
    return(0);
}

int xlib_message_dispatch(crosswin *c)
{
    while(XPending(c->display))
    {

        XEvent ev= {0};
        window *w=NULL;

        XNextEvent(c->display,&ev);

        XFindContext(c->display,ev.xany.window,CONTEXT_ID,(char**)&w);

        if(c==NULL||w==NULL)
            continue;

        switch(ev.xany.type)
        {
            case KeyPress:
            {
                /*This is pretty ugly as I did not want to call the utf8_to_ucs() but for the moment it does the job*/
                if(w->xInputContext)
                {
                    unsigned short symbol=0;

                    if(ev.xkey.keycode==105||ev.xkey.keycode==37)
                        w->ctrl_down=1;


                    if(w->ctrl_down&&ev.xkey.keycode==36)
                        symbol='\n';
                    else
                    {
                        unsigned char buf[9]= {0};
                        unsigned short *temp=NULL;

                        Status status = 0;
                        Xutf8LookupString(w->xInputContext, &ev.xkey, (char*)&buf,8, NULL, &status);

                        temp=utf8_to_ucs(buf);
                        symbol=temp?temp[0]:0;

                        sfree((void**)&temp);
                    }

                    if(symbol&&w->kbd_func)
                        w->kbd_func(symbol,w->kb_data);
                }

                break;
            }

            case KeyRelease:
            {
                if(ev.xkey.keycode==105||ev.xkey.keycode==37)
                    w->ctrl_down=0;

                break;
            }

            case MotionNotify:
            {

                if(ev.xmotion.state&Button1Mask)
                {
                    if(w->draggable==0)
                        continue;

                    w->ccposx = ev.xmotion.x_root;
                    w->ccposy = ev.xmotion.y_root;

                    long x = w->x + (w->ccposx - w->cposx);
                    long y = w->y + (w->ccposy - w->cposy);
                    w->cposx = ev.xmotion.x_root;
                    w->cposy = ev.xmotion.y_root;
                    w->x = x;
                    w->y = y;

                    if(w->keep_on_screen)
                    {
                        if(y >= 0 && y + w->h <= c->sh)
                            w->y = y;
                        else if(y + w->h > c->sh)
                            w->y = c->sh - w->h;
                        else
                            w->y = 0;

                        if(x >= 0 && x + w->w <= c->sw)
                            w->x = x;
                        else if(x + w->w > c->sw)
                            w->x = c->sw - w->w;
                        else
                            w->x = 0;
                    }

                    w->dragging=1;
                    XMoveWindow(c->display,w->window,w->x,w->y);
                    XSync(c->display,1);


                }
                else
                {
                    w->dragging = 0;
                    w->cposx = ev.xmotion.x_root;
                    w->cposy = ev.xmotion.y_root;
                }
            }

            case ButtonRelease:
            case ButtonPress:
            case LeaveNotify:
                xlib_mouse_handle(w,&ev);
                break;

            case DestroyNotify:
            {
                surface_data *sd=w->user_data;
                event_push(sd->cd->eq, (event_handler)surface_destroy, (void*)sd,0,EVENT_REMOVE_BY_DATA);
                break;
            }

        }
    }

    return(0);
}

int xlib_set_opacity(window *w)
{

    xlib_draw(w);
    return(0);
}

void  xlib_set_position(window *w)
{
    XMoveWindow(w->c->display,w->window,w->x,w->y);
}

void xlib_destroy_window(window **w)
{
    xlib_destroy_input_context(*w);
    XDestroyWindow((*w)->c->display,(*w)->window);
    XDeleteContext((*w)->c->display,(XID)*w,CONTEXT_ID);
    XFreePixmap((*w)->c->display,(*w)->pixmap);
    cairo_surface_destroy((*w)->offscreen_buffer);
    free(*w);
}

void xlib_set_zpos(window *w)
{
    switch(w->zorder)
    {
        case crosswin_topmost:
            xlib_set_above(w,0);
            break;

        case crosswin_desktop:
            xlib_set_below(w,0);
            break;

        case crosswin_normal:
            xlib_set_normal(w);
            break;

        case crosswin_bottom:
        case crosswin_top:
            diag_error("%s crosswin_bottom and crosswin_top not implemented",__FILE__);
            break;
    }
}


int xlib_create_input_context(window *w)
{
    if(w->xInputContext||w->xInputMethod)
        return(0);

    w->xInputMethod = XOpenIM(w->c->display, 0, 0, 0);

    if(w->xInputMethod)
    {
        XIMStyles* styles = 0;

        if(XGetIMValues(w->xInputMethod, XNQueryInputStyle, &styles, NULL)==0)
        {
            XIMStyle bestMatchStyle = 0;

            for(int i=0; i<styles->count_styles; i++)
            {

                if ( styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing))
                {
                    bestMatchStyle =  styles->supported_styles[i];
                    break;
                }
            }

            XFree(styles);
            w->xInputContext = XCreateIC(w->xInputMethod, XNInputStyle, bestMatchStyle,  XNClientWindow, w->window, XNFocusWindow, w->window, NULL);
        }
    }

    if(w->xInputContext==NULL&&w->xInputMethod)
    {
        XCloseIM(w->xInputMethod);
        w->xInputMethod=NULL;
        return(-1);

    }

    return(0);
}

int xlib_destroy_input_context(window *w)
{
    w->xInputContext?XDestroyIC(w->xInputContext):0;
    w->xInputMethod?XCloseIM(w->xInputMethod):0;
    w->xInputContext=NULL;
    w->xInputMethod=NULL;
    return(0);
}

#endif
