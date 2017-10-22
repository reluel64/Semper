/*
 *Surface specific routines
 *Project 'Semper'
 *Written by Alexandru-Daniel Mărgărit
*/
#include <surface.h>
#include <cairo/cairo.h>
#include <crosswin/crosswin.h>
#ifdef WIN32
#include <windows.h>
#elif __linux__
#include <sys/types.h>
#include <sys/param.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#endif

#include <math.h>
#include <semper.h>
#include <objects/object.h>
#include <sources/source.h>
#include <string_util.h>
#include <ini_parser.h>
#include <mem.h>
#include <skeleton.h>
#include <xpander.h>
#include <parameter.h>


#define SURFACE_INCLUDE_MAX_DEPTH 100
/*Forward declarations */
static surface_data* surface_init(void *pv, control_data* cd, surface_data** sd,unsigned char memory);
static void surface_render(window* w, void* cr);
static int surface_create_paths(control_data* cd, surface_paths* sp, size_t variant, unsigned char* name);
static void surface_destroy_structs(surface_data* sd, unsigned char destroy);
static int ini_handler(surface_create_skeleton_handler);

typedef struct
{
    size_t size;
    char* buf;
    size_t current;
} surface_reader_stat;

int surface_modified(surface_data *sd)
{
    int res=-1;
    semper_timestamp lst= {0};
    if(semper_get_file_timestamp(sd->sp.file_path,&lst)==0)
    {
        res=memcmp(&sd->st,&lst,sizeof(semper_timestamp))!=0;
        memmove(&sd->st,&lst,sizeof(semper_timestamp));
    }
    return (res);
}

surface_data* surface_by_name(control_data* cd, unsigned char* sn)
{
    surface_data* sd = NULL;

    if(cd == NULL || sn == NULL)
    {
        return (NULL);
    }

    list_enum_part(sd, &cd->surfaces, current)
    {
        if(sd->sp.surface_rel_dir && !strcasecmp(sd->sp.surface_rel_dir, sn))
        {
            return (sd);
        }
    }

    return (NULL);
}

void surface_reset(surface_data* sd)
{
    surface_destroy_structs(sd, 0); //clean the mess just to make things messy again
    sd->srf_col = parameter_color(sd, "SurfaceColor", 0, XPANDER_SURFACE);
    sd->srf_col_2 = parameter_color(sd, "SurfaceColor2", 0, XPANDER_SURFACE);
    sd->team = parameter_string(sd, "Team", NULL, XPANDER_SURFACE);
    sd->uf = parameter_size_t(sd, "UpdateFrequency", 1000, XPANDER_SURFACE);
    sd->focus_act =   parameter_string(sd, "SurfaceFocusAction", NULL, XPANDER_SURFACE);
    sd->unfocus_act = parameter_string(sd, "SurfaceUnfocusAction", NULL, XPANDER_SURFACE);
    sd->update_act =  parameter_string(sd, "SurfaceUpdateAction", NULL, XPANDER_SURFACE);
    sd->reload_act =  parameter_string(sd, "SurfaceReloadAction", NULL, XPANDER_SURFACE);
    sd->unload_act =  parameter_string(sd, "SurfaceUnloadAction", NULL, XPANDER_SURFACE);
    sd->h = parameter_long_long(sd, "Height", 0, XPANDER_SURFACE);
    sd->w = parameter_long_long(sd, "Width", 0, XPANDER_SURFACE);
    sd->def_divider = parameter_size_t(sd, "DefaultDivider", 1, XPANDER_SURFACE);
    sd->ia.path=parameter_string(sd,"Background",NULL,XPANDER_SURFACE);
    image_cache_image_parameters(sd,&sd->ia,XPANDER_SURFACE,"Background");

    if(image_cache_query_image(sd->cd->ich,&sd->ia,NULL,-1,-1)==0)
    {
        if(sd->w<(long)sd->ia.width)
        {
            sd->w=(long)sd->ia.width;
        }
        if(sd->h<(long)sd->ia.height)
        {
            sd->h=(long)sd->ia.height;
        }
    }
    //set the size locks
    sd->lock_w=sd->w>0?1:0;
    sd->lock_h=sd->h>0?1:0;

    if(sd->w==0||sd->h==0)
        sd->wsz = parameter_bool(sd, "AutoSize", 0, XPANDER_SURFACE);


    if(sd->w>0&&sd->h>0)
    {
        crosswin_set_dimension(sd->sw, sd->w,  sd->h);
    }


    /*Let's read the configuration from the Semper.ini*/
    if(sd->snp)
    {
        sd->x = parameter_long_long(sd, "X", 0, XPANDER_SURFACE_CONFIG);
        sd->y = parameter_long_long(sd, "Y", 0, XPANDER_SURFACE_CONFIG);
    }

    sd->keep_on_screen = parameter_bool(sd, "KeepOnScreen", 0, XPANDER_SURFACE_CONFIG);
    sd->hidden = parameter_bool(sd, "Hidden", 0, XPANDER_SURFACE_CONFIG);
    sd->draggable = parameter_bool(sd, "Draggable", 1, XPANDER_SURFACE_CONFIG);
    sd->clkt = parameter_bool(sd, "ClickThrough", 0, XPANDER_SURFACE_CONFIG);
    sd->snp = parameter_bool(sd, "KeepPosition", 1, XPANDER_SURFACE_CONFIG);
    sd->rim = parameter_bool(sd, "ReloadIfModified", 0, XPANDER_SURFACE_CONFIG);
    sd->ro = parameter_byte(sd, "Opacity", 255, XPANDER_SURFACE_CONFIG);
    sd->order = (long)parameter_long_long(sd, "Order", 0, XPANDER_SURFACE_CONFIG);
    sd->zorder = parameter_byte(sd, "ZOrder", crosswin_normal, XPANDER_SURFACE_CONFIG);

    sd->fade_direction = (sd->hidden ? -1 : 1);
    sd->uf=sd->uf==0?1000:sd->uf;
}

static int surface_mouse_handler(window* w, mouse_status* ms)
{
    surface_data* sd = crosswin_get_window_data(w);
    crosswin_get_position(sd->sw, &sd->x, &sd->y);
    if(sd->draggable&&sd->snp)
    {
        long lx=0;
        long ly=0;


        lx = parameter_long_long(sd, "X", 0, XPANDER_SURFACE_CONFIG);
        ly = parameter_long_long(sd, "Y", 0, XPANDER_SURFACE_CONFIG);

        if(sd->x!=lx||sd->y!=ly)
        {
            unsigned char buf[260] = { 0 };
            snprintf(buf, sizeof(buf), "%ld", sd->x);
            skeleton_add_key(sd->scd, "X", buf);
            memset(buf, 0, sizeof(buf));
            snprintf(buf, sizeof(buf), "%ld", sd->y);
            skeleton_add_key(sd->scd, "Y", buf);
            //semper_save_configuration(sd->cd);
        }
    }

    if(object_hit_testing(sd, ms)==0||ms->hover==0)
    {
        mouse_handle_button(sd, MOUSE_SURFACE, ms);
    }
    sd->mouse_hover=ms->hover; //this will be used by SurfaceOverAction

    return (1);
}

void surface_window_init(surface_data* sd)
{
    if(sd->sw == NULL)
    {
        control_data* cd = sd->cd;
        sd->sw = crosswin_init_window(&cd->c);
        crosswin_set_window_data(sd->sw, sd);
        crosswin_set_render(sd->sw, surface_render);
        crosswin_set_mouse_handler(sd->sw, surface_mouse_handler);
    }
}

void surface_destroy_structs(surface_data* sd, unsigned char destroy)
{
    if(destroy)
    {
        source* s = NULL;
        source* ts = NULL;
        object* o = NULL;
        object* to = NULL;
        mouse_destroy_actions(&sd->ma);
        sd->x=0;
        sd->y=0;
        sd->h=0;
        sd->w=0;
#ifdef __linux__
        inotify_rm_watch(sd->cd->inotify_fd,sd->inotify_watch_id);
#endif
        list_enum_part_safe(s, ts, &sd->sources, current)
        {
            source_destroy(&s);
        }
        list_enum_part_safe(o, to, &sd->objects, current)
        {
            object_destroy(&o);
        }
        skeleton_destroy(&sd->skhead);

        sd->sv = NULL; // just set it to NULL as the skeleton was just destroyed and the sd->sv now points to an invalid memory address
        sd->spm = NULL;

    }
    image_cache_unref_image(sd->cd->ich,&sd->ia,1);
    sfree((void**)&sd->ia.path);
    sfree((void**)&sd->team);
    sfree((void**)&sd->unfocus_act);
    sfree((void**)&sd->focus_act);
    sfree((void**)&sd->update_act);
    sfree((void**)&sd->reload_act);
    sfree((void**)&sd->unload_act);
}

int surface_destroy(surface_data* sd)
{
    if(sd == NULL)
    {
        return (-1);
    }

    if(!linked_list_empty(&sd->current))
    {
        linked_list_remove(&sd->current);
        list_entry_init(&sd->current);
    }
    if(sd->co)
    {

        sd->fade_direction = -1; // set the direction
        sd->ro = 0;

        surface_fade(sd); // the required opacity
        event_push(sd->cd->eq, (event_handler)surface_destroy, (void*)sd, ((sd->co * 2)), EVENT_PUSH_TIMER);
    }
    else
    {
        event_remove(sd->cd->eq, NULL, sd, EVENT_REMOVE_BY_DATA);
        surface_create_paths(NULL, &sd->sp, 0, NULL); // this will destroy the paths
        surface_destroy_structs(sd, 1);
        crosswin_destroy((window**)&sd->sw);
        sfree((void**)&sd);
    }
    return (0);
}

static unsigned char* surface_variant_file(unsigned char* sd, size_t variant)
{
    if(sd == NULL || variant == 0)
    {
        return (NULL);
    }
    unsigned char* ret = NULL;
#ifdef WIN32
    size_t sdl = string_length(sd);


    unsigned char* buf = zmalloc(sdl + 6 + 1);
    snprintf(buf,sdl+6+1,"%s/*.ini",sd);

    WIN32_FIND_DATAW fd = { 0 };
    wchar_t* flt = utf8_to_ucs(buf);
    void* h = FindFirstFileW(flt, &fd);

    do
    {
        if(h == INVALID_HANDLE_VALUE)
        {
            sfree((void**)&flt);
            return (NULL);
        }
        if(--variant == 0)
        {
            break;
        }
    }
    while(FindNextFileW(h, &fd));

    if(h!=NULL&&h != INVALID_HANDLE_VALUE)
    {
        FindClose(h);
    }

    sfree((void**)&flt);
    sfree((void**)&buf);

    if(variant)
    {
        return (NULL);
    }

    ret = ucs_to_utf8(fd.cFileName, NULL, 0);

#elif __linux__

    DIR* dh = opendir(sd);
    struct dirent* fi = NULL;
    if(dh == NULL)
        return (NULL);

    while((fi = readdir(dh)) != NULL)
    {
        if(!strcasecmp(fi->d_name, ".") || !strcasecmp(fi->d_name, ".."))
            continue;

        if(fi->d_type == DT_REG && is_file_type(fi->d_name, "ini"))
        {
            if(--variant == 0)
            {
                ret = clone_string(fi->d_name);
                break;
            }
        }
    }

    closedir(dh);
#endif
    return (ret);
}

size_t surface_file_variant(unsigned char* sd, unsigned char* file)
{
    int ret = 0;

    if(!sd || !file)
    {
        return (0);
    }

    for(size_t i = 1; i < (size_t)-1; i++)
    {
        unsigned char* res = surface_variant_file(sd, i);

        if(res == NULL)
        {
            return (0);
        }
        else if(strcasecmp(file, res) == 0)
        {
            ret = i;
        }

        sfree((void**)&res);

        if(ret)
        {
            break;
        }
    }
    return (ret);
}

static int surface_render_background(surface_data* sd, cairo_t* cr)
{
    unsigned char* color = (unsigned char*)&sd->srf_col;
    unsigned char *px=NULL;
    image_cache_query_image(sd->cd->ich,&sd->ia,&px,-1,-1);



    if(px)
    {
        int stride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,sd->ia.width);
        cairo_surface_t *back=cairo_image_surface_create_for_data(px,CAIRO_FORMAT_ARGB32,sd->ia.width,sd->ia.height,stride);
        cairo_set_source_surface(cr,back,0.0,0.0);
        cairo_paint(cr);
        cairo_surface_destroy(back);
        return(0);
    }

    if(color[3] == 0)
    {
        return (-1);
    }

    cairo_pattern_t* pat1 = cairo_pattern_create_linear((double)sd->w / 2.0, 0.0, (double)sd->w / 2.0, (double)sd->h);

    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    double alpha = 0.0;

    if(color[3])
    {
        red = (double)color[2] / 255.0;
        green = (double)color[1] / 255.0;
        blue = (double)color[0] / 255.0;
        alpha = (double)color[3] / 255.0;
        cairo_pattern_add_color_stop_rgba(pat1, 0, red, green, blue, alpha);
    }

    color = (unsigned char*)&sd->srf_col_2;

    if(color[3])
    {
        red = (double)color[2] / 255.0;
        green = (double)color[1] / 255.0;
        blue = (double)color[0] / 255.0;
        alpha = (double)color[3] / 255.0;
        cairo_pattern_add_color_stop_rgba(pat1, 1, red, green, blue, alpha);
    }

    cairo_rectangle(cr, 0, 0, sd->w, sd->h);
    cairo_set_source(cr, pat1);
    cairo_fill(cr);
    cairo_pattern_destroy(pat1);

    return (0);
}

int surface_adjust_size(surface_data *sd)
{
    object *o=NULL;
    if(sd->lock_w==0)
    {
        sd->w=0;
    }
    if(sd->lock_h==0)
    {
        sd->h=0;
    }

    list_enum_part(o,&sd->objects,current)
    {
        if(o->die||o->enabled==0)
        {
            continue;
        }

        object_calculate_coordinates(o);

        if(sd->lock_w==0)
        {
            long w = (((o->w!=o->auto_w||o->w < 0)&&o->auto_w!=0) ? o->auto_w : o->w);

            if(sd->w < w + o->x)
            {
                sd->w = (w + o->x);
            }
        }

        if(sd->lock_h==0)
        {
            long h = (((o->h!=o->auto_h||o->h < 0)&&o->auto_h!=0) ? o->auto_h : o->h);
            if(sd->h < h + o->y)
            {
                sd->h = h + o->y;
            }
        }
    }

    sd->w = labs(sd->w);
    sd->h = labs(sd->h);
    crosswin_set_dimension(sd->sw, sd->w ? sd->w : 1, sd->h ? sd->h : 1);
    return(0);
}

static void surface_render(window* w, void* cr)
{
    surface_data* sd = crosswin_get_window_data(w);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    if(surface_render_background(sd, cr) == -1)
    {
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_rectangle(cr, 0, 0, sd->w, sd->h);
        cairo_fill(cr);
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    object_render(sd, cr);
}

int surface_change_variant(surface_data* sd, unsigned char* vf)
{
    if(!sd || !vf)
    {
        return (0);
    }

    size_t vn = surface_file_variant(sd->sp.surface_dir, vf);

    if(vn)
    {
        unsigned char* temp_name = clone_string(sd->sp.surface_rel_dir);
        surface_create_paths(sd->cd, &sd->sp, vn, temp_name);
        sfree((void**)&temp_name);
        surface_reload(sd);
    }
    return (vn);
}

int surface_create_paths(control_data* cd, surface_paths* sp, size_t variant, unsigned char* name)
{
    if(sp == NULL)
    {
        return (-1);
    }
    sfree((void**)&sp->data_dir);
    //sfree((void**)&sp->inc);
    sfree((void**)&sp->file_path);
    sfree((void**)&sp->surface_dir);
    sfree((void**)&sp->surface_file);
    sfree((void**)&sp->surface_rel_dir);

    sp->variant = 0;

    if(cd == NULL || name == NULL || variant == 0)
    {
        return (-1);
    }
    /*Let's create the paths*/

    size_t name_len = string_length(name);
    sp->surface_dir = zmalloc(name_len + cd->surface_dir_length + 3);
    snprintf(sp->surface_dir, name_len + cd->surface_dir_length + 2, "%s/%s", cd->surface_dir, name);

    /*At this point we have only the surface absolute path,
     * without file name but we need more so let's get the surface file*/

    unsigned char* fn = surface_variant_file(sp->surface_dir, variant);

    if(fn == NULL) // if it is NULL then we do not have a file from which to create the surface and we have to abort
    {
        sfree((void**)&sp->surface_dir);
        return (-1);
    }

    size_t surface_fdir_length = string_length(sp->surface_dir);
    size_t surface_filename_length = string_length(fn);
    sp->file_path = zmalloc(surface_fdir_length + surface_filename_length + 3);
    snprintf(sp->file_path, surface_fdir_length + surface_filename_length + 2, "%s/%s", sp->surface_dir, fn);
    /*Well, we have a full path to our surface file*/

    sp->surface_file = clone_string(fn);
    /*If this is not obvious, we now have the filename*/

    sfree((void**)&fn); // we now have a copy so let's free this one

    /*
     * We created above the necessary paths to load a surface
     * Below we will create some helping paths
     */
    sp->data_dir = zmalloc(surface_fdir_length + 7); // allocate memory for dir_path/Data
    snprintf(sp->data_dir, surface_fdir_length + 6, "%s/Data", sp->surface_dir);

    // sp->inc = zmalloc(surface_fdir_length + 10); // allocate memory for dir_path/Include
    //snprintf(sp->inc, surface_fdir_length + 9, "%s/Include", sp->surface_dir);

    sp->variant = variant;
    sp->surface_rel_dir = clone_string(name);
    return (1);
}

void surface_reload(surface_data* sd)
{
    if(sd && sd->co)
    {
        sd->fade_direction = -1;
        sd->ro = 0;
        surface_fade(sd);
        event_push(sd->cd->eq, (event_handler)surface_reload, (void*)sd, ((sd->co * 2)), EVENT_PUSH_TIMER|EVENT_REMOVE_BY_DATA_HANDLER);
    }
    else if(sd)
    {
        surface_destroy_structs(sd, 1);
        surface_init(&sd->sp, sd->cd, &sd,0);
    }
}

surface_data *surface_load_memory(control_data *cd,unsigned char *buf,size_t buf_sz,surface_data **sd)
{
    surface_reader_stat srs= {.size = buf_sz, .buf = buf, .current = 0 };

    return( surface_init(&srs, cd, sd,1));
}

size_t surface_load(control_data* cd, unsigned char* name, size_t variant)
{
    surface_data* sd = surface_by_name(cd, name);

    if(sd)
    {
        return (1);
    }

    /*Let's create an initial surface path*/

    surface_paths sp = { 0 };
    surface_create_paths(cd, &sp, variant, name);
    sd = surface_init(&sp, cd, NULL,0);

    if(sd)
    {
        surface_data* loop_sd = NULL;
        surface_data* p = NULL;
        surface_data* n = NULL;

        list_enum_part(loop_sd, &cd->surfaces, current)
        {
            if(loop_sd->order > sd->order)
                n = loop_sd;
            else if(loop_sd->order < sd->order)
                p = loop_sd;
            else
                break;
        }

        if(n == NULL)
            linked_list_add_last(&sd->current, &cd->surfaces);
        else if(p == NULL)
            linked_list_add(&sd->current, &cd->surfaces);
        else
            _linked_list_add(&p->current, &sd->current, &n->current);
    }
    else
    {
        surface_create_paths(NULL, &sp, 0, NULL);
    }

    return (0);
}

/*Loads a surface that was stored in a buffer*/
static char* surface_read_from_memory(char* str, size_t ccount, surface_reader_stat* srrs)
{
    memset(str, 0, ccount);
    size_t i = 0;
    for(; srrs->buf[srrs->current] != '\n' && srrs->current < srrs->size; srrs->current++, i++)
    {
        str[i] = srrs->buf[srrs->current];
    }

    if(i < ccount && srrs->current < srrs->size)
    {
        str[i] = srrs->buf[srrs->current++];
    }
    if(srrs->current == srrs->size)
    {
        return (NULL);
    }
    return (str);
}

static void surface_init_items(surface_data* sd)
{
    section cs = skeleton_first_section(&sd->skhead);
    object *o=NULL;
    source *s=NULL;

    do
    {
        unsigned char* sn = skeleton_get_section_name(cs);

        if(sn && (!strcasecmp(sn, "Surface") 	  ||
                  !strcasecmp(sn, "Surface-Meta") ||
                  !strcasecmp(sn, "Surface-Variables"))) // if the current section is one of those, we will skip them
        {
            continue;
        }

        key ok = skeleton_get_key(cs, "Object");
        key sk = skeleton_get_key(cs, "Source");

        /*The objects have higher priority so if a section has both Object and Source declared,
         * then the section will be considered to represent an object rather than a source
         */

        if(ok)
            object_init(cs, sd);
        else if(ok == NULL && sk)
            source_init(cs, sd);
    }
    while((cs = skeleton_next_section(cs, &sd->skhead)) != NULL);

    list_enum_part(s, &sd->sources, current)
    source_reset(s);

    list_enum_part(o, &sd->objects, current)
    {
        object_reset(o);
        object_update(o); //we need to perform the first update cycle to
        //obtain the surface size in case it it not
        //volatile or is not specified by the user
    }
}

static int ini_handler(surface_validate_handler)
{

    unused_parameter(kn);
    unused_parameter(kv);
    unused_parameter(com);

    if(sn && !strcasecmp(sn, "Surface"))
    {
        *((int*)pv) = 1;
        return (1);
    }
    return (0);
}

static int ini_handler(surface_create_skeleton_handler)
{
    unused_parameter(com);
    skeleton_handler_data* shd =pv;
    surface_data* sd = shd->pv;

    if(shd->depth>=SURFACE_INCLUDE_MAX_DEPTH)
    {
        return(1);
    }
    if(!sn && !kn && !kv)
    {
        return (0);
    }

    if(sn && *sn)
    {
        shd->ls = skeleton_add_section(&sd->skhead, sn);
        if(strcasecmp(sn,"Surface-Variables")==0)
        {
            sd->sv  = shd->ls;
        }
    }

    if(kn)
    {
        if(shd->ls == NULL)
        {
            shd->ls = skeleton_add_section(&sd->skhead, NULL);
        }

        if(kn && strcasecmp(kn, "$Include") == 0)
        {
            section ts = shd->ls; // save current section
            xpander_request xr= {0};
            xr.os=kv;
            xr.requestor=shd->pv;
            xr.req_type=XPANDER_SURFACE;
            shd->depth++;
            if(xpander(&xr))
            {
                ini_parser_parse_file(xr.es, surface_create_skeleton_handler, shd);
                sfree((void**)&xr.es);
            }
            else
            {
                ini_parser_parse_file(xr.os, surface_create_skeleton_handler, shd);
            }
            shd->depth--;
            shd->ls = ts; // restore current section
            return (0);
        }

        shd->lk = skeleton_add_key(shd->ls, kn, kv);
        sfree((void**)&shd->kv);
        shd->kv = clone_string(kv);
        return (0);
    }

    if(!kn && kv && shd->ls)
    {
        kn=skeleton_key_name(shd->lk);
        size_t sl = string_length(shd->kv);
        size_t kvl = string_length(kv);
        unsigned char* tmp = zmalloc(sl + kvl + 1);

        strncpy(tmp,shd->kv,sl);
        strncpy(tmp+sl,kv,kvl);


        sfree((void**)&shd->kv);
        shd->kv = tmp;
        shd->lk = skeleton_add_key(shd->ls, kn, shd->kv);
        return (0);
    }

    return (0);
}

static surface_data* surface_init(void *pv, control_data* cd, surface_data** sd,unsigned char memory)
{
    surface_paths *sp=pv;

    surface_data* tsd = NULL;
    skeleton_handler_data shd = { 0 };
    size_t temp_cycle = 0;
    int valid = 0;

    if(sd)
    {
        tsd = *sd;
    }

    if(memory)
    {
        sp=NULL;
    }
    else if(pv==NULL)
    {
        return(NULL);
    }

    event_remove(cd->eq, NULL, tsd, EVENT_REMOVE_BY_DATA); // make sure that we do not have events for this surface

    diag_info("Validating surface file");

    if(memory==0)
    {
        ini_parser_parse_file(sp->file_path, surface_validate_handler, &valid); // call the validation handler
    }
    else
    {
        ini_parser_parse_stream((ini_reader)surface_read_from_memory,pv,surface_validate_handler,&valid);
        ((surface_reader_stat*)pv)->current=0;
    }

    if(valid == 0)
    {
        diag_error("Invalid surface file");
        return (NULL);
    }

    if(tsd == NULL)
    {
        tsd = zmalloc(sizeof(surface_data));
    }

    tsd->cd = cd;


    if(sd == NULL)
    {
        list_entry_init(&tsd->current);
    }
    /*Initialize lists*/
    list_entry_init(&tsd->skhead);
    list_entry_init(&tsd->objects);
    list_entry_init(&tsd->sources);

    if(sp)
    {
        memcpy(&tsd->sp, sp, sizeof(surface_paths));                       // create a local copy of the paths (warning: we do not free the previously allocated paths. We're just copying the addresses*/
    }

    surface_modified(tsd);                                                   // get the initial timestamp

    /* The reason to use a skeleton for the ini file and not the actual ini is flexibility and performance
     * A tree structure is easier to maintain than an acutal buffer, and is easier to modify
     */

    shd.pv = tsd;
    if(memory==0)
    {
        ini_parser_parse_file(sp->file_path, surface_create_skeleton_handler, &shd); // we will create the skeleton
    }
    else
    {
        ini_parser_parse_stream((ini_reader)surface_read_from_memory,pv,surface_create_skeleton_handler,&shd);
    }
    sfree((void**)&shd.kv);                                                      // free a small yet important to free residue

    tsd->snp = 1;                                                                // give the reset routine a chance to read X and Y from the skeleton

    tsd->spm = skeleton_get_section(&tsd->skhead, "Surface");
    if(sp)
    {
        tsd->scd = skeleton_get_section(&cd->shead, sp->surface_rel_dir);          // get the section from Semper.ini
    }

#ifdef __linux__
    if(memory==0)
    {
        tsd->inotify_watch_id=inotify_add_watch( cd->inotify_fd, tsd->sp.file_path, IN_ALL_EVENTS );
    }
#endif
    surface_window_init(tsd);                                                   // initialize window
    surface_reset(tsd); // set or reset the surface parameters
    surface_init_items(tsd);                                                    // initialize sources and objects

    crosswin_click_through(tsd->sw, tsd->clkt);
    crosswin_set_position(tsd->sw, tsd->x, tsd->y);
    crosswin_draggable(tsd->sw, tsd->draggable);
    crosswin_keep_on_screen(tsd->sw, tsd->keep_on_screen);
    crosswin_set_window_z_order(tsd->sw, tsd->zorder);
    mouse_set_actions(tsd, MOUSE_SURFACE);

    temp_cycle = tsd->cycle;
    tsd->cycle = 0;
    surface_update(tsd);
    tsd->cycle = temp_cycle;
    event_push(tsd->cd->eq, (event_handler)surface_fade, (void*)tsd,0,0);

    return (tsd);
}

void surface_fade(surface_data* sd)
{
    if((sd->hidden&&sd->co)||(sd->co!=sd->ro))
    {
        sd->co=CLAMP((int)sd->co+(16*(int)sd->fade_direction),0,(int)sd->ro?sd->ro:255);
        crosswin_set_opacity(sd->sw, sd->co);
        event_push(sd->cd->eq, (event_handler)surface_fade, (void*)sd, 16, EVENT_PUSH_TIMER|EVENT_REMOVE_BY_DATA_HANDLER);
    }

    if(sd->hidden && sd->co == 0)
    {
        crosswin_hide(sd->sw);
        sd->visible = 0;
    }
    else if(sd->hidden == 0 && sd->co && sd->visible == 0)
    {
        crosswin_show(sd->sw);
        sd->visible = 1;
    }
}


int surface_update(surface_data* sd)
{
    source* s = NULL;
    source* ts = NULL;
    object* to = NULL;
    object* o = NULL;


    if(sd == NULL)
    {
        return (-1);
    }

    /*************UPDATE SOURCES************/
    list_enum_part_safe(s, ts, &sd->sources, current)
    {
        if(s->die)
        {
            source_destroy(&s);
            continue;
        }
        if((sd->cycle%s->divider||s->disabled)&&s->vol_var==0)
        {
            continue;
        }
        source_update(s);
    }

    /************UPDATE OBJECTS***********/
    list_enum_part_safe(o, to, &sd->objects, current)
    {
        if(o->die)
        {
            object_destroy(&o);
            continue;
        }

        if((sd->cycle%o->divider||o->enabled==0)&&o->vol_var==0)
        {
            continue;
        }

        object_update(o);
    }

    if(sd->wsz||sd->cycle==0)
    {
        surface_adjust_size(sd);
    }

    sd->hidden ? 0 : crosswin_draw(sd->sw);

    if(sd->update_act_lock == 0)
    {
        sd->update_act_lock = 1;
        sd->update_act?command(sd, &sd->update_act):0;
        sd->update_act_lock = 0;
    }

    if(sd->old_mouse_hover != sd->mouse_hover)
    {
        sd->mouse_hover ?(sd->focus_act ? command(sd, &sd->focus_act):0) :
        (sd->focus_act ? command(sd, &sd->unfocus_act):0);
        sd->old_mouse_hover = sd->mouse_hover;
    }

    sd->cycle++; // increment the surface update cycle


    if(sd->uf!=(size_t)-1)
    {
        // re-schedule the update by removing any potential duplicates and push a new timed event
        event_push(sd->cd->eq, (event_handler)surface_update, (void*)sd, sd->uf, EVENT_PUSH_TIMER|EVENT_REMOVE_BY_DATA_HANDLER);
    }
    return (1);
}
