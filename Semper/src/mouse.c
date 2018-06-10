/*
 * Mouse actions
 * Part of project "Semper"
 * Written by Alexandru-Daniel Mărgărit
 * */

#include <mouse.h>
#include <surface.h>
#include <objects/object.h>
#include <mem.h>
#include <skeleton.h>
#include <xpander.h>
#include <string_util.h>
#include <parameter.h>
#include <surface_builtin.h>

typedef struct
{
    unsigned char *name;
    unsigned char **act;
} mouse_action_tbl;

static int mouse_set_actions_internal(void *pv, unsigned char tgt, unsigned char destroy)
{
    mouse_actions *ma = NULL;

    switch(tgt)
    {
        case MOUSE_OBJECT:
            ma = &((object*)pv)->ma;
            break;

        case MOUSE_SURFACE:
            ma = &((surface_data*)pv)->ma;
            break;

        case MOUSE_SURFACE|MOUSE_OBJECT:
            ma = pv;
    }

    if(ma == NULL)
    {
        return(-1);
    }

    mouse_action_tbl mtbl[] =
    {
        { "MouseLeftUp",           &ma->lcu },
        { "MouseMiddleUp",         &ma->mcu },
        { "MouseRightUp",          &ma->rcu },
        { "MouseLeftDown",         &ma->lcd },
        { "MouseMiddleDown",       &ma->mcd },
        { "MouseRightDown",        &ma->rcd },
        { "MouseLeftDouble",      &ma->lcdd },
        { "MouseMiddleDouble",    &ma->rcdd },
        { "MouseRightDouble",     &ma->mcdd },
        { "MouseScrollUp",     &ma->scrl_up },
        { "MouseScrollDown", &ma->scrl_down },
        { "MouseOverAction",        &ma->ha },
        { "MouseLeaveAction",      &ma->nha }
    };

    for(size_t i = 0; i < sizeof(mtbl) / sizeof(mouse_action_tbl); i++)
    {
        sfree((void**)mtbl[i].act);

        if(destroy == 0)
        {
            (*mtbl[i].act) = parameter_string(pv, mtbl[i].name, NULL, (tgt == MOUSE_OBJECT ? XPANDER_OBJECT : XPANDER_SURFACE));
        }
    }

    return(0);
}

int mouse_set_actions(void *pv, unsigned char tgt)
{
    return(mouse_set_actions_internal(pv, tgt, 0));
}

void mouse_destroy_actions(mouse_actions* ma)
{
    mouse_set_actions_internal(ma, 3, 1);
}

int mouse_handle_button(void* pv, unsigned char mode, mouse_status* ms)
{
    mouse_actions* ma = NULL;
    surface_data* sd = NULL;
    int ret = 0;
    /*Will be used to obtain mouse coordinates*/
    long x = 0;
    long y = 0;
    long w = 0;
    long h = 0;

    unsigned char *mcomm = NULL;
    unsigned char tbuf[256] = {0};

    /****************/
    if(pv == NULL)
    {
        return (0);
    }

    if(mode == MOUSE_OBJECT)
    {
        object* o = pv;
        x = o->x;
        y = o->y;
        w = o->w < 0 ? o->auto_w : o->w;
        h = o->h < 0 ? o->auto_h : o->h;
        ma = &o->ma;
        sd = o->sd;

    }
    else if(mode == MOUSE_SURFACE)
    {
        sd = pv;
        ma = &sd->ma;
        x = 0;
        y = 0;
        crosswin_get_size(sd->sw,&w,&h);
    }

    if(ma == NULL)
    {
        return (-1);
    }

    snprintf(tbuf, 255, "(@MouseX,%ld);(@MouseY,%ld);(@%%MouseX,%ld);(@%%MouseY,%ld)", \
             ms->x - x, ms->y - y, w > 0 ? (((ms->x - x) * 100) / w) : 0, h > 0 ? (((ms->y - y) * 100) / h) : 0);

    switch(ms->button)
    {
        case mouse_button_left:
            switch(ms->state)
            {
                case mouse_button_state_pressed:
                    mcomm = ma->lcd;
                    break;

                case mouse_button_state_unpressed:
                    mcomm = ma->lcu;
                    break;

                case mouse_button_state_2x:
                    mcomm = ma->lcdd;

                default:
                    break;
            }

            break;

        case mouse_button_middle:
            switch(ms->state)
            {
                case mouse_button_state_pressed:
                    mcomm = ma->mcd;
                    break;

                case mouse_button_state_unpressed:
                    mcomm = ma->mcu;
                    break;

                case mouse_button_state_2x:
                    mcomm = ma->mcdd;

                default:
                    break;
            }

            break;

        case mouse_button_right:
            switch(ms->state)
            {
                case mouse_button_state_pressed:
                    mcomm = ma->rcd;
                    break;

                case mouse_button_state_unpressed:
                    mcomm = ma->rcu;
                    break;

                case mouse_button_state_2x:
                    mcomm = ma->rcdd;

                default:
                    break;

            }

            break;

        default:
            if(ms->scroll_dir)
            {
                mcomm = (ms->scroll_dir == -1 ? ma->scrl_down : ma->scrl_up);
            }
            else if(ms->hover != ma->omhs)
            {
                if(ms->hover == mouse_hover)
                {
                    if(mode == MOUSE_OBJECT)
                    {
                        surface_builtin_init(pv, 1);
                    }

                    mcomm = ma->ha;
                }
                else if(ms->hover == mouse_unhover && ma->omhs != mouse_none)
                {
                    if(mode == MOUSE_OBJECT)
                    {
                        surface_builtin_destroy(&((object*)pv)->ttip);
                    }

                    mcomm = ma->nha;
                }

                ma->omhs = ms->hover;
            }

            break;

    }

    if(mcomm)
    {
        unsigned char *fcomm = replace(mcomm, tbuf, 0);

        if(fcomm)
        {
            ms->handled = 1;
            ret = 1;
            command(sd, &fcomm);
            sfree((void**)&fcomm);
        }
    }

    return (ret);
}
