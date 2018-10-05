/*
Source generic routines
Part of Project 'Semper'
Written by Alexandru-Daniel Mărgărit
*/

#include <surface.h>
#include <math.h>
#include <semper.h>
#include <xpander.h>
#include <string_util.h>
#include <mem.h>
#include <sources/action.h>
#include <semper_api.h>
#include <bind.h>
#include <skeleton.h>
#include <sources/disk_space.h>
#include <sources/string.h>
#include <sources/calculator.h>
#include <sources/memory.h>
#include <sources/source.h>
#include <sources/processor.h>
#include <sources/iterator.h>
#include <sources/folderinfo.h>
#include <sources/internal/_surfaces_collector.h>
#include <sources/internal/_surface_info.h>
#include <sources/internal/_surface_lister.h>
#include <sources/script.h>
#include <sources/webget.h>
#include <sources/timed_action.h>
#include <sources/network.h>
#include <sources/time_s.h>
#include <sources/wifi.h>
#include <sources/input.h>
#include <sources/internal/_diag_show.h>
#include <sources/folderview.h>
#include <parameter.h>
#ifdef __linux__
#include <dlfcn.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif
typedef unsigned char *(*src_str_rtn)(void *ip, unsigned char **pms, size_t pms_len);
typedef struct
{
    unsigned char *source_name;
    void (*source_init_rtn)(void** spv, void* ip);
    void (*source_destroy_rtn)(void** spv);
    double (*source_update_rtn)(void* spv);
    void (*source_reset_rtn)(void* spv, void* ip);
    unsigned char* (*source_string_rtn)(void* spv);
    void (*source_command_rtn)(void* spv, unsigned char* comm);

} source_table;

typedef struct _avg_val
{
    double value;
    list_entry current;
} source_average_val;

typedef struct _average
{
    size_t count; // current number of elements in list
    list_entry values;
    double total;
} source_average;


static size_t source_routines_table(unsigned char* s, source_table **st)
{
    static source_table table[] =
    {
        // Source name          Init routine                Destroy routine             Update routine              Reset routine               String routine                          Command routine
        { "Extension",          NULL,                       NULL,                       NULL,                       NULL,                       NULL,                                             NULL },
        { "Calculator",         calculator_init,            calculator_destroy,         calculator_update,          calculator_reset,           NULL,                                             NULL },
        { "Memory",             memory_init,                memory_destroy,             memory_update,              memory_reset,               NULL,                                             NULL },
        { "DiskSpace",          disk_space_init,            disk_space_destroy,         disk_space_update,          disk_space_reset,           disk_space_string,                                NULL },
        { "String",             string_source_init,         string_source_destroy,      string_source_update,       string_source_reset,        string_source_string,                             NULL },
        { "Processor",          processor_init,             processor_destroy,          processor_update,           processor_reset,            NULL,                                             NULL },
        { "Iterator",           iterator_init,              iterator_destroy,           iterator_update,            iterator_reset,             NULL,                                 iterator_command },
        { "FolderInfo",         folderinfo_init,            folderinfo_destroy,         folderinfo_update,          folderinfo_reset,           NULL,                                             NULL },
        { "WebGet",             webget_init,                webget_destroy,             webget_update,              webget_reset,               webget_string,                                    NULL },

        { "TimedAction",        timed_action_init,          timed_action_destroy,       NULL,                       timed_action_reset,         NULL,                             timed_action_command },
        { "Time",               time_init,                  time_destroy,               time_update,                time_reset,                 time_string,                                      NULL },
        { "Script",             script_init,                script_destroy,             script_update,              script_reset,               script_string,                          script_command },
        { "Network",            network_init,               network_destroy,            network_update,             network_reset,              NULL,                                             NULL },
        { "Wifi",               wifi_init,                  wifi_destroy,               wifi_update,                wifi_reset,                 wifi_string,                                      NULL },
        { "TextInput",          input_init,                 input_destroy,              input_update,               input_reset,                input_string,                            input_command },
        { "FolderView",         folderview_init,            folderview_destroy,         folderview_update,          folderview_reset,           folderview_string,                  folderview_command },
        //sources for internal use only
        { "_SurfaceCollector_", surfaces_collector_init,    surfaces_collector_destroy, surfaces_collector_update,  surfaces_collector_reset,   surfaces_collector_string,  surfaces_collector_command },
        { "_SurfaceInfo_",      surface_info_init,          surface_info_destroy,       surface_info_update,        surface_info_reset,         surface_info_string,                              NULL },
        { "_SurfaceLister_",    surface_lister_init,        surface_lister_destroy,     surface_lister_update,      surface_lister_reset,       surface_lister_string,          surface_lister_command },
        { "_DiagShow_",         diag_show_init,             diag_show_destroy,          diag_show_update,           diag_show_reset,            diag_show_string,                    diag_show_command }

    };

    if(s == NULL || st == NULL)
        return(0);

    for(size_t i = 0; i < sizeof(table) / sizeof(source_table); i++)
    {
        if(!strcasecmp(s, table[i].source_name))
        {
            *st = &table[i];
            return(i + 1);
        }
    }

    return(0);
}

void source_average_destroy(source* s)
{
    source_average* sa = s->avg_pv;

    if(sa)
    {
        source_average_val* sav = NULL;
        source_average_val* tsav = NULL;

        list_enum_part_backward_safe(sav, tsav, &sa->values, current)
        {
            linked_list_remove(&sav->current);
            sfree((void**)&sav);
        }
        sfree((void**)&s->avg_pv);
    }
}

static double source_average_update(source *s, double value)
{
    source_average* sa = s->avg_pv;
    source_average_val* sav = NULL;
    source_average_val* tsav = NULL;

    if(s->avg_count < 2)
    {
        if(s->avg_pv)
        {
            source_average_destroy(s);
        }

        return (value);
    }
    else if(s->avg_pv == NULL)
    {
        s->avg_pv = zmalloc(sizeof(source_average));
        sa = s->avg_pv;
        list_entry_init(&sa->values);
    }

    /*Shrink the list if needed*/
    list_enum_part_backward_safe(sav, tsav, &sa->values, current)
    {
        if((s->avg_count >= sa->count) || sa->count == 0)
        {
            break;
        }

        linked_list_remove(&sav->current);
        sfree((void**)&sav);
        sa->count--;
    }


    if(sa->count < s->avg_count)
    {
        sav = zmalloc(sizeof(source_average_val));
        list_entry_init(&sav->current);
        sav->value = value;
        sa->count++;
    }
    else
    {
        tsav = element_of(sa->values.prev, tsav, current);
        linked_list_remove(&tsav->current);
        sa->total -= tsav->value;
        list_entry_init(&tsav->current);
        tsav->value = value;
        sav = tsav;
    }

    sa->total += sav->value;
    linked_list_add(&sav->current, &sa->values);

    return (sa->total / (double)sa->count);
}

unsigned char* source_variable(source* s, size_t* len, unsigned char flags)
{
    if(s == NULL || len == NULL || s->die)
    {
        if(len)
            *len = 0;

        return(NULL);
    }


    if(flags & SOURCE_VARIABLE_DOUBLE)
    {
        *len = s->inf_double_len;
        return (s->inf_double);
    }
    else if(flags & SOURCE_VARIABLE_EXPAND)
    {
        if(s->inf_exp)
        {
            *len = s->inf_exp_len;
            return (s->inf_exp);
        }
        else if(s->s_info)
        {
            *len = s->s_info_len;
            return (s->s_info);
        }
        else
        {
            *len = s->inf_double_len;
            return (s->inf_double);
        }
    }
    else
    {
        if(s->s_info)
        {
            *len = s->s_info_len;
            return (s->s_info);
        }
        else
        {
            *len = s->inf_double_len;
            return (s->inf_double);
        }
    }

    return (NULL);
}

static void* source_load_lib(unsigned char* path)
{
    void* lib = NULL;
    uniform_slashes(path);
#ifdef WIN32
    wchar_t* wp = utf8_to_ucs(path);
    lib = LoadLibraryW(wp);
    sfree((void**)&wp);
#elif __linux__
    unsigned char *temp_path = path;

    if(is_file_type(path, "so") == 0)
    {
        size_t len = string_length(path);
        temp_path = zmalloc(len + 5);

        if(temp_path)
        {
            snprintf(temp_path, len + 4, "%s.so", path);
        }
    }

    if(temp_path)
    {
        lib = dlopen(temp_path, RTLD_LAZY);
    }

    if(temp_path != path)
    {
        sfree((void**)&temp_path);
    }

#endif
    return (lib);
}

static void* source_get_proc(void* lib, unsigned char* proc)
{
    void* rtn = NULL;
#ifdef WIN32
    rtn = GetProcAddress(lib, proc);
#elif __linux__
    rtn = dlsym(lib, proc);
#endif
    return (rtn);
}

static void source_unload_lib(void* lib)
{
#ifdef WIN32
    FreeLibrary(lib);
#elif __linux__
    dlclose(lib);
#endif
}

unsigned char  *source_call_str_rtn(source *s, unsigned char *rtn, unsigned char **pms, size_t pm_len)
{
    unsigned char *str = NULL;


    if(s->library)
    {
         static char *rsr_rtn[6] =
    {
        "init",
        "update",
        "command",
        "destroy",
        "reset",
        "string"
    };
        char go = 1;

        for(unsigned char i = 0; i < sizeof(rsr_rtn) / sizeof(unsigned char*); i++)
        {
            if(rtn && !strcasecmp(rtn, rsr_rtn[i]))
                go = 0;
        }

        if(go)
        {
            src_str_rtn src_rtn = source_get_proc(s->library, rtn);

            if(src_rtn)
            {
                str = src_rtn(s->pv, pms, pm_len);
            }
        }
    }

    return(str);
}

static int source_load_extension(source* s)
{
    surface_data* sd = s->sd;

    if(s->extension == NULL)
    {
        return (-1);
    }

    size_t srcnl = string_length(s->extension);
    size_t pln = sd->cd->ext_dir_length + srcnl + 2;

    unsigned char* modp = zmalloc(pln);

    snprintf(modp, pln, "%s/%s", sd->cd->ext_dir, s->extension);
    s->library = source_load_lib(modp);
    sfree((void**)&modp);


    if(s->library == NULL)
    {
        pln = sd->cd->ext_app_dir_len + srcnl + 2;
        modp = zmalloc(pln);
        snprintf(modp, pln, "%s/%s", sd->cd->ext_app_dir, s->extension);
        s->library = source_load_lib(modp);
        sfree((void**)&modp);

        if(s->library == NULL)
            return (-1);
    }

    return (0);
}

static void source_destroy_routines(source* s)
{
    if(s->source_destroy_rtn)
    {
        s->source_destroy_rtn(&s->pv);
    }

    if(s->library)
    {
        source_unload_lib(s->library);
    }

    s->source_destroy_rtn = NULL;
    s->source_init_rtn = NULL;
    s->source_command_rtn = NULL;
    s->source_update_rtn = NULL;
    s->source_string_rtn = NULL;
    s->source_reset_rtn = NULL;
}

static int source_set_routines(source* s, source_table *st)
{
    int ret = -1;

    if(s == NULL || st == NULL)
        return(-1);

    if(s->type == 1 && source_load_extension(s) == 0)
    {

        s->source_init_rtn = source_get_proc(s->library, "init");
        s->source_destroy_rtn = source_get_proc(s->library, "destroy");
        s->source_update_rtn = source_get_proc(s->library, "update");
        s->source_reset_rtn = source_get_proc(s->library, "reset");
        s->source_string_rtn = source_get_proc(s->library, "string");
        s->source_command_rtn = source_get_proc(s->library, "command");
        ret = 0;
    }
    else
    {
        s->source_init_rtn = st->source_init_rtn;
        s->source_reset_rtn = st->source_reset_rtn;
        s->source_update_rtn = st->source_update_rtn;
        s->source_destroy_rtn = st->source_destroy_rtn;
        s->source_string_rtn = st->source_string_rtn;
        s->source_command_rtn = st->source_command_rtn;
        ret = 0;
    }

    if(ret == 0 && s->source_init_rtn)
    {
        s->source_init_rtn(&s->pv, s);
    }

    return (ret);
}

int source_init(section s, surface_data* sd)
{
    if(s == NULL || sd == NULL)
    {
        return (-1);
    }

    source* ss = zmalloc(sizeof(source));
    ss->sd = sd;
    ss->cs = s;
    ss->divider = 1;
    ss->vol_var = 1; // mark as volatile so the source_reset() will be called on the first update cycle
    action_init(ss);
    list_entry_init(&ss->current);
    linked_list_add_last(&ss->current, &sd->sources);
    return (0);
}

source* source_by_name(surface_data* sd, unsigned char* sn, size_t len)
{
    source* s = NULL;

    if(!sn || !sd)
    {
        return (NULL);
    }

    list_enum_part(s, &sd->sources, current)
    {
        unsigned char* sname = skeleton_get_section_name(s->cs);

        if(sname && (len == (size_t) - 1 ? !strcasecmp(sname, sn) : !strncasecmp(sname, sn, len)))
        {
            if(s->die)
            {
                return (NULL);
            }

            return (s);
        }
    }

    return (NULL);
}

void source_destroy(source** s)
{
    source* ts = *s;
    bind_unbind(ts->sd, ts);
    source_destroy_routines(ts); // call the source routines to perform internal cleanup
    action_destroy(ts);
    source_average_destroy(ts);

    /*Remove source information*/
    sfree((void**)&ts->team);
    sfree((void**)&ts->replacements);
    sfree((void**)&ts->s_info);
    sfree((void**)&ts->inf_exp);
    sfree((void**)&ts->ext_str);
    sfree((void**)&ts->extension);
    sfree((void**)&ts->update_act);
    sfree((void**)&ts->change_act);
    skeleton_remove_section(&ts->cs); //remove the source section from the skeleton

    /*Remove source from chain*/
    linked_list_remove(&ts->current);
    sfree((void**)s);
}

void source_reset(source* s)
{
    surface_data* sd = s->sd;
    source_table *st = NULL;
    double mval = 0; // for MaxValue

    sfree((void**)&s->replacements);
    sfree((void**)&s->update_act);
    sfree((void**)&s->change_act);
    sfree((void**)&s->team);
    s->vol_var = parameter_bool(s, "Volatile", 0, XPANDER_SOURCE);
    s->paused = parameter_bool(s, "Paused", 0, XPANDER_SOURCE);
    s->inverted = parameter_bool(s, "Inverted", 0, XPANDER_SOURCE);
    s->always_do = parameter_bool(s, "AlwaysDo", 0, XPANDER_SOURCE);
    s->min_val = parameter_double(s, "MinValue", 0.0, XPANDER_SOURCE);
    s->replacements = parameter_string(s, "Replace", NULL, XPANDER_SOURCE);
    s->disabled = parameter_bool(s, "Disabled", 0, XPANDER_SOURCE);
    s->divider = parameter_size_t(s, "Divider", sd->def_divider, XPANDER_SOURCE);
    s->avg_count = parameter_size_t(s, "Average", 0, XPANDER_SOURCE);
    s->update_act = parameter_string(s, "UpdateAction", NULL, XPANDER_SOURCE);
    s->change_act = parameter_string(s, "ChangeAction", NULL, XPANDER_SOURCE);
    mval = parameter_double(s, "MaxValue", 0.0, XPANDER_SOURCE);
    s->regexp = parameter_bool(s, "ReplaceRegExp", 0, XPANDER_SOURCE);
    s->team = parameter_string(s, "Team", NULL, XPANDER_SOURCE);
    unsigned char* ts = parameter_string(s, "Source", NULL, XPANDER_SOURCE);
    unsigned char type = source_routines_table(ts, &st);
    sfree((void**)&ts);

    if(s->divider == 0)
    {
        s->divider = 1;
    }

    if(type != s->type)
    {
        // Source already initialized but the type has changed
        if(s->type > 0)
        {
            source_destroy_routines(s);
        }

        s->type = type;

        if(type != 1)
        {
            source_set_routines(s, st); // do not call if the type is "Extension" as this is handled below
        }
    }

    if(s->type == 1)
    {
        unsigned char* ext_name = parameter_string(s, "Extension", NULL, XPANDER_SOURCE);

        if(!s->extension || (ext_name && strcasecmp(ext_name, s->extension)))
        {
            sfree((void**)&s->extension);
            source_destroy_routines(s);
            s->extension = ext_name;
            source_set_routines(s, st);
        }
        else
        {
            sfree((void**)&ext_name);
        }
    }

    /*the MaxValue needs some tweaks*/
    if(mval > 0.0)
    {
        if(mval <= s->min_val)
        {
            s->max_val = s->min_val + 1.0;
        }
        else
        {
            s->max_val = mval;
        }
    }
    else if(mval == 0.0 && s->max_val == 0.0)
    {
        s->max_val = 1.0;
    }

    action_reset(s);

    if(s->source_reset_rtn)
    {
        s->source_reset_rtn(s->pv, s);
    }
}

int source_update(source* s)
{
    surface_data* sd = s->sd;
    unsigned char svc = 0; // source value changed flag

    if(s->vol_var)
    {
        source_reset(s);
    }

    if(s->disabled || s->paused)
    {
        if(s->disabled)
        {
            s->d_info = 0.0;
            s->s_info_len = 0;
            s->inf_exp_len = 0;
            sfree((void**)&s->inf_exp);
            sfree((void**)&s->s_info);
        }

        return (0);
    }

    if(s->source_update_rtn)
    {
        double cv = 0.0;

        cv = s->source_update_rtn(s->pv); // call source update routine

        if(!s->hold_min)
        {
            s->min_val = cv <= s->min_val ? cv : s->min_val; // adjust min
        }

        if(!s->hold_max)
        {
            s->max_val = cv >= s->max_val ? cv : s->max_val; // adjust max
        }

        /*Calculate the new result*/
        cv = (s->inverted ? s->max_val - cv + s->min_val : cv);

        if(s->avg_pv || s->avg_count > 1)
        {
            cv = source_average_update(s, cv);
        }

        svc = (cv != s->d_info);
        s->d_info = cv;
    }

    if(s->source_string_rtn)
    {
        unsigned char* str = s->source_string_rtn(s->pv);

        if(str == NULL || s->s_info == NULL || strcmp(s->s_info, str))
        {
            sfree((void**)&s->s_info);
            s->s_info_len = 0;

            if(str)
            {
                s->s_info = (str[0] != 0 ? clone_string(str) : zmalloc(1));
                s->s_info_len = (str[0] != 0 ? string_length(s->s_info) : 0);
            }

            svc = 1;
        }
    }
    /*
     * make sure that if the source was changed and does
     * not have a string function we do clear the string information
     * */
    else if(s->s_info || s->s_info_len)
    {
        sfree((void**)&s->s_info);
        s->s_info_len = 0;
    }

    /*Create a string to help out xpander.c*/
    if(svc || s->update_cycle == 0)
    {
        s->inf_double_len = snprintf(s->inf_double, 64, "%lf", s->d_info);
        sfree((void**)&s->inf_exp);
        s->inf_exp_len = 0;

        if(s->replacements)
        {
            s->inf_exp = replace((s->s_info ? s->s_info : s->inf_double), s->replacements, s->regexp);
            s->inf_exp_len = string_length(s->inf_exp);
        }
    }

    if(s->update_act_lock == 0)
    {
        s->update_act_lock = 1;
        s->update_act != NULL ? command(sd, &s->update_act) : 0;
        s->update_act_lock = 0;
    }

    if(svc && s->change_act_lock == 0)
    {
        s->change_act_lock = 1;
        s->change_act ? command(sd, &s->change_act) : 0;
        s->change_act_lock = 0;
    }

    action_execute(s);
    s->update_cycle++;
    return (1);
}
