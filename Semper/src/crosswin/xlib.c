#ifdef __linux__
#include <crosswin/crosswin.h>
#include <crosswin/xlib.h>
#include <string.h>
#include <stdlib.h>
#include <string_util.h>
#include <surface.h>
#include <event.h>
#include <mem.h>
#define CONTEXT_ID 0x2712

void xlib_init_display(crosswin *c)
{
    c->display=XOpenDisplay(NULL);

    XMatchVisualInfo(c->display, DefaultScreen(c->display), 32, TrueColor, &c->vinfo);
    c->disp_fd=(void*)(size_t)XConnectionNumber(c->display);
}
typedef struct Hints
{
    unsigned long   flags;
    unsigned long   functions;
    unsigned long   decorations;
    long            inputMode;
    unsigned long   status;
} Hints;

void xlib_init_window(crosswin_window *w)
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
                    ExposureMask            |
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
                              CWBackPixmap                      |
                              CWBackPixel                       |
                              CWColormap  						|
                              CWEventMask						|
                              CWCursor 							,
                              &attr
                             );


    XSaveContext(w->c->display,(XID)w->window,CONTEXT_ID,(const char*)w);
    Atom type = XInternAtom(w->c->display,"_NET_WM_WINDOW_TYPE", False);
    Atom value = XInternAtom(w->c->display,"_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(w->c->display, w->window, type, XA_ATOM, 32, PropModeReplace,(unsigned char*)&value, 1);

    Hints hints;
    Atom property;
    hints.flags = 2;
    hints.decorations = 0;
    property = XInternAtom(w->c->display, "_MOTIF_WM_HINTS", 1);
    XChangeProperty(w->c->display,w->window,property,property,32,PropModeReplace,(unsigned char *)&hints,5);

    XMapWindow(w->c->display, w->window);

}

void xlib_set_dimmension(crosswin_window *w)
{
    XWindowChanges wc= {0};
    wc.width=w->w;
    wc.height=w->h;
    XConfigureWindow(w->c->display,w->window,CWWidth|CWHeight,&wc);
}


static void xlib_render(crosswin_window *w)
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

void xlib_set_mask(crosswin_window *w)
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

void xlib_draw(crosswin_window* w)
{
    xlib_render(w);
    xlib_set_mask(w);
    XSync(w->c->display,1);
}

int xlib_message_dispatch(crosswin *c)
{

  
    do
    {

        XEvent ev= {0};
        crosswin_window *w=NULL;

        XNextEvent(c->display,&ev);

        XFindContext(c->display,ev.xany.window,CONTEXT_ID,(char**)&w);

        if(c==NULL||w==NULL)
            continue;
        switch(ev.xany.type)
        {
        case KeyPress:
        {
            /*This is pretty ugly as I did not want to call the utf8_to_ucs() but for the moment it does the job*/
            if(ev.xkey.keycode==105||ev.xkey.keycode==37)
                w->md.ctrl=1;
            if(w->xInputContext)
            {
                unsigned short symbol=0;
                
                if(w->md.ctrl&&ev.xkey.keycode==36)
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
                w->md.ctrl=0;

            break;
        }

        case MotionNotify:
        {
            
            
            w->md.x=ev.xmotion.x;
            w->md.y=ev.xmotion.y;
            w->md.root_x=ev.xmotion.x_root;
            w->md.root_y=ev.xmotion.y_root;
            w->md.hover=mouse_hover;
            w->c->handle_mouse(w);
            break;
        }

        case ButtonRelease:
            w->md.x=ev.xbutton.x;
            w->md.y=ev.xbutton.y;
            w->md.state=mouse_button_state_unpressed;
            switch(ev.xbutton.button)
            {
            case Button1:
                w->md.button=mouse_button_left;
                break;
            case Button2:
                w->md.button=mouse_button_middle;
                break;
            case Button3:
                w->md.button=mouse_button_right;
                break;
            case Button4:
                w->md.state=mouse_button_state_none;
                w->md.scroll_dir=-1;
                break;
            case Button5:
                w->md.state=mouse_button_state_none;
                w->md.scroll_dir=1;
                break;

            }
            w->c->handle_mouse(w);
            break;
        case ButtonPress:
      
            XSetInputFocus(w->c->display,w->window,RevertToParent,0);
            w->md.x=ev.xbutton.x;
            w->md.y=ev.xbutton.y;
            w->md.state=mouse_button_state_pressed;
            switch(ev.xbutton.button)
            {
            case Button1:
           
                w->md.button=mouse_button_left;
                break;
            case Button2:
                w->md.button=mouse_button_middle;
                break;
            case Button3:
                w->md.button=mouse_button_right;
                break;
            }
            w->c->handle_mouse(w);
            break;
        case LeaveNotify:
            XSetInputFocus(w->c->display,None,RevertToParent,0);
            w->md.hover=mouse_unhover;
            w->c->handle_mouse(w);
            break;

        case DestroyNotify:
        {
            surface_data *sd=w->user_data;
            event_push(sd->cd->eq, (event_handler)surface_destroy, (void*)sd,0,EVENT_REMOVE_BY_DATA);
            break;
        }


        }
        
    }
    while(XPending(c->display));

    return(0);
}

int xlib_set_opacity(crosswin_window *w)
{
    xlib_draw(w);
    return(0);
}

void  xlib_set_position(crosswin_window *w)
{
   
   /* int found=0;
    size_t evt_count=0;
    XEvent *evts=NULL;*/
   XMoveWindow(w->c->display,w->window,w->x,w->y); /*Move the window*/
  /*  while(XPending(w->c->display))
    {
        
        XEvent *tmp=realloc(evts,sizeof(XEvent)*(evt_count+1));
        
        if(tmp)
            evts=tmp;
        else
            break;
        
        
        XNextEvent(w->c->display,evts+evt_count);
evt_count++; 
     }
    
  
     //XSync(w->c->display,1);
    
    
  for(size_t i=0;i<evt_count;i++)
  {
    XSendEvent(w->c->display,w->window,1,KeyPressMask 		    |
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
                    ExposureMask            |
                    ButtonPressMask,evts+i);
    
  }
  
  sfree((void**)&evts);*/
}

void xlib_destroy_window(crosswin_window **w)
{
    xlib_destroy_input_context(*w);
    XDestroyWindow((*w)->c->display,(*w)->window);
    XDeleteContext((*w)->c->display,(XID)*w,CONTEXT_ID);
    XFreePixmap((*w)->c->display,(*w)->pixmap);
    cairo_surface_destroy((*w)->offscreen_buffer);
    free(*w);
}

void xlib_set_zpos(crosswin_window *w)
{
    return;
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


int xlib_create_input_context(crosswin_window *w)
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

int xlib_destroy_input_context(crosswin_window *w)
{
    w->xInputContext?XDestroyIC(w->xInputContext):0;
    w->xInputMethod?XCloseIM(w->xInputMethod):0;
    w->xInputContext=NULL;
    w->xInputMethod=NULL;
    return(0);
}

#endif
