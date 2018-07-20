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
#include <dwmapi.h>
#include <mem.h>
static inline HWND win32_zpos(crosswin_window* w)
{
    switch(w->zorder)
    {
        case crosswin_desktop:
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
    crosswin_window* w = (crosswin_window*)GetWindowLongPtrW(win, GWLP_USERDATA);

    if(w == NULL)
        return (DefWindowProc(win, message, wpm, lpm));

    crosswin *c = w->c;
    mouse_data *md = &c->md;

    switch(message)
    {
        case WM_MOUSEWHEEL:
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->scroll_dir = GET_WHEEL_DELTA_WPARAM(wpm) > 0 ? 1 : -1;
            w->c->handle_mouse(w);
            return(0);

        case WM_LBUTTONDOWN:
            SetCapture(w->window);
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->button = mouse_button_left ;
            md->state = mouse_button_state_pressed;
            w->c->handle_mouse(w);
            return(0);

        case WM_LBUTTONUP:
            ReleaseCapture();
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->button = mouse_button_left;
            md->state = mouse_button_state_unpressed;
            w->c->handle_mouse(w);
            return(0);

        case WM_RBUTTONDOWN:
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->button = mouse_button_right;
            md->state = mouse_button_state_pressed;
            w->c->handle_mouse(w);
            return(0);

        case WM_RBUTTONUP:
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->button = mouse_button_right;
            md->state = mouse_button_state_unpressed;
            w->c->handle_mouse(w);
            return(0);

        case WM_MBUTTONDOWN:
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->button = mouse_button_middle;
            md->state = mouse_button_state_pressed;
            w->c->handle_mouse(w);
            return(0);

        case WM_MBUTTONUP:
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->button = mouse_button_middle;
            md->state = mouse_button_state_unpressed;
            w->c->handle_mouse(w);
            return(0);

        case WM_MOUSELEAVE:
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            md->hover = mouse_unhover;
            w->c->handle_mouse(w);
            return(0);

        case WM_KEYDOWN:
        {
            if(lpm & 1000000)
            {
                return(1);
            }

            return(0);
        }

        case WM_CHAR:
        case WM_UNICHAR:
        case WM_SYSCHAR:
        {
            if(w->kbd_func && wpm != UNICODE_NOCHAR)
            {
                w->kbd_func(wpm, w->kb_data);
            }

            return(0);
        }

        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *wp = (WINDOWPOS*)lpm;
            if(w->lock_z)
            {
                wp->flags |= SWP_NOZORDER;
            }
            else
            {
                wp->hwndInsertAfter = win32_zpos(w);
            }

            break;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT ev = { 0 };
            md->x = GET_X_LPARAM(lpm);
            md->y = GET_Y_LPARAM(lpm);
            POINT p = { md->x, md->y};
            md->hover = mouse_hover;

            ev.dwFlags = 0x2;
            ev.cbSize = sizeof(TRACKMOUSEEVENT);
            ev.hwndTrack = win;
            ev.dwHoverTime = 1;
            TrackMouseEvent(&ev);

            MapWindowPoints(win, NULL, &p, 1);
            md->ctrl = !!(wpm & MK_CONTROL);
            md->root_x = p.x;
            md->root_y = p.y;
            w->c->handle_mouse(w);
            return (0);
        }

        case WM_CLOSE:
        {
            surface_data *sd = w->user_data;
            event_push(sd->cd->eq, (event_handler)surface_destroy, (void*)sd, 0, EVENT_REMOVE_BY_DATA);
            return(0);
        }

        case WM_DISPLAYCHANGE:
        {
            c->update = 1;
            break;
        }

        case WM_QUIT:
        {
            surface_data *sd = w->user_data;
            control_data *cd = sd->cd;
            safe_flag_set(cd->quit_flag,1);
            event_wake(cd->eq);
            return(0);
        }
    }

    return (DefWindowProc(win, message, wpm, lpm));
}

void win32_init_class(void)
{
    WNDCLASSEXW wclass = { 0 };
    memset(&wclass, 0, sizeof(WNDCLASSEX));
    wclass.style = (CS_DBLCLKS & 0) | CS_NOCLOSE | CS_OWNDC;
    wclass.lpfnWndProc = win32_message_callback;
    wclass.lpszClassName = L"SemperSurface";
    wclass.hCursor = LoadImageA(NULL, MAKEINTRESOURCE(32512), IMAGE_CURSOR, 0, 0, LR_SHARED);
    wclass.cbSize = sizeof(WNDCLASSEXW);
    RegisterClassExW(&wclass);

}

void win32_init_window(crosswin_window* w)
{
    size_t no_peek = 1;
    w->window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_LAYERED, L"SemperSurface", L"Surface", WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    SetWindowLongPtrW(w->window, GWLP_USERDATA, (LONG_PTR)w);
    DwmSetWindowAttribute(w->window, DWMWA_EXCLUDED_FROM_PEEK, &no_peek, sizeof(no_peek));
}

void win32_click_through(crosswin_window* w, unsigned char state)
{
    unsigned int cstatus = GetWindowLongPtrW(w->window, GWL_EXSTYLE);
    cstatus = state ? cstatus | WS_EX_TRANSPARENT : cstatus & ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(w->window, GWL_EXSTYLE, cstatus);
}

void win32_message_dispatch(crosswin *c)
{
    MSG msg = { 0 };

    while(PeekMessageW(&msg, NULL, 0, 0, 1))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static int win32_get_monitors_callback(HMONITOR mon, HDC dcmon, LPRECT prect, LPARAM pv)
{
    if(prect == NULL)
        return(1);

    void **data = (void**)pv;
    size_t *cnt = (size_t*)data[1];
    crosswin_monitor **cm = (crosswin_monitor**)data[0];

    crosswin_monitor *tcm = realloc(*cm, ((*cnt) + 1) * sizeof(crosswin_monitor));

    if(tcm)
    {
        *cm = tcm;

        tcm[*cnt].x = prect->left;
        tcm[*cnt].y = prect->top;
        tcm[*cnt].h = prect->bottom - prect->top;
        tcm[*cnt].w = prect->right - prect->left;
        tcm[*cnt].index = (*cnt);
        (*cnt)++;
    }

    return(1);

}

int win32_get_monitors(crosswin_monitor **cm, size_t *cnt)
{
    *cnt = 0;
    void *data[2] = {cm, cnt};
    return(EnumDisplayMonitors(NULL, NULL, win32_get_monitors_callback, (LPARAM)data) == 1 ? 0 : -1);
}

void win32_draw(crosswin_window* w)
{
    BLENDFUNCTION bf =
    {
        .AlphaFormat = AC_SRC_ALPHA,
        .BlendFlags = 0,
        .BlendOp = AC_SRC_OVER,
        .SourceConstantAlpha = w->opacity
    };
    static POINT pt = { 0, 0 };
    SIZE sz = { w->w, w->h };

    if(w->offscreen_buffer == NULL || (w->offw != (size_t)w->w || w->offh != (size_t)w->h))
    {
        if(w->offscreen_buffer)
        {
            cairo_surface_destroy(w->offscreen_buffer);
            w->offscreen_buffer = NULL;
        }

        w->offscreen_buffer = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_ARGB32, w->w, w->h); // create a surface to render on
        w->offw = (size_t)w->w;
        w->offh = (size_t)w->h;
    }

    cairo_t* cr =  cairo_create(w->offscreen_buffer); // create a cairo context to gain access to surface rendering routine
    HDC dc = cairo_win32_surface_get_dc(w->offscreen_buffer); // get the device context that will be used by UpdateLayeredWindow

    w->render_func ? w->render_func(w, cr) : 0;

    UpdateLayeredWindow(w->window, NULL, NULL, &sz, dc, &pt, 0, &bf, ULW_ALPHA);

    /*Everything has been drawn so we will free up the resources*/

    cairo_destroy(cr); // destroy the context
}

void win32_set_position(crosswin_window* w)
{
    POINT ptd = { w->x, w->y };
    UpdateLayeredWindow(w->window, NULL, &ptd, NULL, NULL, NULL, 0, NULL, 0);
}

void win32_set_opacity(crosswin_window* w)
{
    BLENDFUNCTION bf;
    bf.AlphaFormat = AC_SRC_ALPHA;
    bf.BlendFlags = 0;
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = w->opacity;
    UpdateLayeredWindow(w->window, NULL, NULL, NULL, NULL, NULL, 0, &bf, ULW_ALPHA);
}

void win32_set_visible(crosswin_window* w)
{
    if(w->visible)
    {
        SetWindowPos(w->window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
    else
    {
        SetWindowPos(w->window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_HIDEWINDOW | SWP_NOACTIVATE);
    }
}

void win32_destroy_window(crosswin_window** w)
{
    SetWindowLongPtrW((*w)->window, GWLP_USERDATA, 0);
    DestroyWindow((*w)->window);
    cairo_surface_destroy((*w)->offscreen_buffer);
    free(*w);
    *w = NULL;
}

void win32_set_zpos(crosswin_window *w)
{
    SetWindowPos(w->window, win32_zpos(w), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
}

#endif
