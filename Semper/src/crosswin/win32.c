#ifdef WIN32
#include <stdio.h>
#include <windows.h>
#include <cairo/cairo.h>
#include <cairo/cairo-win32.h>
#include <windowsx.h>
#include <crosswin/crosswin.h>
#include <semper.h>
#include <surface.h>
#include <event.h>
static int win32_prepare_mouse_event(window* w, unsigned int message, WPARAM wpm, LPARAM lpm)
{
    memset(&w->mouse, 0, sizeof(mouse_status));

    w->mouse.x = GET_X_LPARAM(lpm);
    w->mouse.y = GET_Y_LPARAM(lpm);
    w->mouse.state = -1;
    if(w->dragging)
        return (0);
    switch(message)
    {
    case WM_MOUSEWHEEL:
        w->mouse.x -= w->x;
        w->mouse.y -= w->y;
        w->mouse.scroll_dir = GET_WHEEL_DELTA_WPARAM(wpm) > 0 ? 1 : -1;
        break;
    case WM_LBUTTONDBLCLK:
        w->mouse.button = MOUSE_LEFT_BUTTON;
        w->mouse.state = 2;
        break;
    case WM_RBUTTONDBLCLK:
        w->mouse.button = MOUSE_RIGHT_BUTTON;
        w->mouse.state = 2;
        break;
    case WM_MBUTTONDBLCLK:
        w->mouse.button = MOUSE_MIDDLE_BUTTON;
        w->mouse.state = 2;
        break;
    case WM_LBUTTONDOWN:
        w->mouse.button = MOUSE_LEFT_BUTTON;
        w->mouse.state = 1;
        break;
    case WM_LBUTTONUP:
        w->mouse.button = MOUSE_LEFT_BUTTON;
        w->mouse.state = 0;
        break;
    case WM_RBUTTONDOWN:
        w->mouse.button = MOUSE_RIGHT_BUTTON;
        w->mouse.state = 1;
        break;
    case WM_RBUTTONUP:
        w->mouse.button = MOUSE_RIGHT_BUTTON;
        w->mouse.state = 0;
        break;
    case WM_MBUTTONDOWN:
        w->mouse.button = MOUSE_MIDDLE_BUTTON;
        w->mouse.state = 1;
        break;
    case WM_MBUTTONUP:
        w->mouse.button = MOUSE_MIDDLE_BUTTON;
        w->mouse.state = 0;
        break;
    }

    switch(message)
    {
    case WM_MBUTTONDBLCLK:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEHOVER:
        w->mouse.hover = 1;
    case WM_MOUSELEAVE:
        if(w->mouse_func)
        {
            w->mouse_func(w, &w->mouse);
            return (1);
        }
        break;
    }
    return (0);
}

static inline HWND win32_zpos(window* w)
{
    switch(w->zorder)
    {
    case crosswin_desktop:
    {
        return(HWND_BOTTOM);
    }
    case crosswin_bottom:
        return(HWND_BOTTOM);
    case crosswin_normal:
        return (HWND_NOTOPMOST);
    case crosswin_top:
        return (HWND_TOP);
    case crosswin_topmost:
        return (HWND_TOPMOST);
    default:
        return (0);
    }
}



static LRESULT CALLBACK win32_message_callback(HWND win, unsigned int message, WPARAM wpm, LPARAM lpm)
{

    window* w = (window*)GetWindowLongPtrW(win, GWLP_USERDATA);

    if(w == NULL)
        return (DefWindowProc(win, message, wpm, lpm));
    crosswin* c = w->c;

    switch(message)
    {

    case WM_MBUTTONDBLCLK:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        if(w->dragging == 1)
        {
            ReleaseCapture();
            win32_prepare_mouse_event(w, message, wpm, lpm);
            w->dragging = 0;
            return (0);
        }
    }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEHOVER:
    case WM_MOUSELEAVE:
    {
        if(message == WM_LBUTTONUP)
        {
            ReleaseCapture();
        }
        if(message == WM_LBUTTONDOWN)
        {
            SetCapture(w->window);
        }
        if(!(wpm & MK_CONTROL))
        {
            win32_prepare_mouse_event(w, message, wpm, lpm);
        }
        return (0);
    }

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        //((WINDOWPOS*)lpm)->flags|=SWP_NOZORDER;
        ((WINDOWPOS*)lpm)->hwndInsertAfter =(HWND)(size_t)win32_zpos(w);
        return(0);
    }
    case WM_KEYDOWN:
    {
        if(lpm&1000000)
        {
            return(1);
        }
        return(0);
    }
    case WM_CHAR:
    case WM_UNICHAR:
    case WM_SYSCHAR:
    {

        if(wpm==UNICODE_NOCHAR)
        {
            return(0);
        }
        else if(w->kbd_func)
        {
            w->kbd_func(wpm,w->kb_data);
        }
        return(0);
    }

    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT ev = { 0 };
        ev.dwFlags = 0x3;
        ev.cbSize = sizeof(TRACKMOUSEEVENT);
        ev.hwndTrack = win;
        ev.dwHoverTime = 1;
        TrackMouseEvent(&ev);

        if(wpm & MK_LBUTTON)
        {
            if(w->draggable)
            {
                w->ccposx = GET_X_LPARAM(lpm);
                w->ccposy = GET_Y_LPARAM(lpm);
                long x = w->x + (w->ccposx - w->cposx);
                long y = w->y + (w->ccposy - w->cposy);

                if(w->x != x || w->y != y)
                {
                    w->dragging = 1;
                }

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
                SetWindowPos(w->window, NULL, w->x, w->y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
        else
        {
            w->dragging = 0;
            w->cposx = GET_X_LPARAM(lpm);
            w->cposy = GET_Y_LPARAM(lpm);
        }

        w->mouse.x = GET_X_LPARAM(lpm);
        w->mouse.y = GET_Y_LPARAM(lpm);
        return (0);
    }
    case WM_CLOSE:
    {
        surface_data *sd=w->user_data;
        event_push(sd->cd->eq, (event_handler)surface_destroy, (void*)sd,0,EVENT_REMOVE_BY_DATA);
        return(0);
    }
    }

    return (DefWindowProc(win, message, wpm, lpm));
}

void win32_init_class(void)
{
    WNDCLASSEXW wclass = { 0 };
    memset(&wclass, 0, sizeof(WNDCLASSEX));
    wclass.style = CS_DBLCLKS | CS_NOCLOSE | CS_OWNDC;
    wclass.lpfnWndProc = win32_message_callback;
    wclass.lpszClassName = L"SemperSurface";
    wclass.hCursor = LoadImageA(NULL, MAKEINTRESOURCE(32512), IMAGE_CURSOR, 0, 0, LR_SHARED);
    wclass.cbSize = sizeof(WNDCLASSEXW);
    RegisterClassExW(&wclass);
}

void win32_init_window(window* w)
{
    w->window = CreateWindowExW( WS_EX_TOOLWINDOW | WS_EX_LAYERED, L"SemperSurface", L"Surface", WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    SetWindowLongPtrW(w->window, GWLP_USERDATA, (LONG_PTR)w);
}

void win32_click_through(window* w, unsigned char state)
{
    unsigned int cstatus=GetWindowLongPtrW(w->window,GWL_EXSTYLE);
    cstatus=state?cstatus|WS_EX_TRANSPARENT:cstatus&~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(w->window, GWL_EXSTYLE, cstatus);
}

void win32_message_dispatch(crosswin *c)
{
    MSG msg = { 0 };
    if(PeekMessageW(&msg, NULL, 0, 0, 1)!=WM_QUIT)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    else
    {
        c->quit=1;
    }
}

void win32_draw(window* w)
{
    if(w->offscreen_buffer == NULL)
    {
        w->offscreen_buffer = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_ARGB32, w->w, w->h); // create a surface to render on
        w->offw = (size_t)w->w;
        w->offh = (size_t)w->h;
    }
    else if(w->offscreen_buffer && (w->offw != (size_t)w->w || w->offh != (size_t)w->h))
    {
        cairo_surface_destroy(w->offscreen_buffer);
        w->offscreen_buffer = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_ARGB32, w->w, w->h);
        w->offw = (size_t)w->w;
        w->offh = (size_t)w->h;
    }

    cairo_t* cr =  cairo_create(w->offscreen_buffer); // create a cairo context to gain access to surface rendering routine
    HDC dc = cairo_win32_surface_get_dc(w->offscreen_buffer); // get the device context that will be used by UpdateLayeredWindow

    w->render_func ? w->render_func(w, cr) : 0;

    BLENDFUNCTION bf;
    bf.AlphaFormat = AC_SRC_ALPHA;
    bf.BlendFlags = 0;
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = w->opacity;
    POINT pt = { 0, 0 };

    SIZE sz = { w->w, w->h };

    UpdateLayeredWindow(w->window, NULL, NULL, &sz, dc, &pt, 0, &bf, ULW_ALPHA);

    /*Everything has been drawn so we will free up the resources*/

    cairo_destroy(cr); // destroy the context
}

void win32_set_position(window* w)
{
    POINT ptd = { w->x, w->y };
    UpdateLayeredWindow(w->window, NULL, &ptd, NULL, NULL, NULL, 0, NULL, 0);
}

void win32_set_opacity(window* w)
{
    BLENDFUNCTION bf;
    bf.AlphaFormat = AC_SRC_ALPHA;
    bf.BlendFlags = 0;
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = w->opacity;
    UpdateLayeredWindow(w->window, NULL, NULL, NULL, NULL, NULL, 0, &bf, ULW_ALPHA);
}

void win32_hide(window* w)
{
    SetWindowPos(w->window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_HIDEWINDOW | SWP_NOACTIVATE);
}

void win32_show(window* w)
{
    SetWindowPos(w->window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

void win32_destroy_window(window** w)
{
    SetWindowLongPtrW((*w)->window, GWLP_USERDATA, 0);
    DestroyWindow((*w)->window);
    cairo_surface_destroy((*w)->offscreen_buffer);
    free(*w);
    *w = NULL;
}

void win32_set_zpos(window *w)
{
    SetWindowPos(w->window, (HWND)(size_t)win32_zpos(w), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
}

#endif
