/*
*Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/

#include <semper.h>
#include <stdlib.h>
#include <crosswin/crosswin.h>
#include <fcntl.h>
#include <time.h>
#include <mem.h>
#include <surface.h>
#include <string_util.h>
#include <ini_parser.h>
#include <skeleton.h>
#include <event.h>
#include <fontconfig/fontconfig.h>
#include <surface_builtin.h>


#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <poll.h>
#include <limits.h>
#include <dirent.h>
#elif WIN32
#include <windows.h>
#include <winbase.h>
#include <wchar.h>
WINBASEAPI WINBOOL WINAPI QueryFullProcessImageNameW(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD lpdwSize);
#endif
extern void crosswin_update_z(crosswin *c);
static void semper_reload_surfaces_if_modified(control_data* cd);
static void semper_surface_watcher(unsigned long err, unsigned long transferred, void *pv);

typedef struct
{
    FILE* nf;
    unsigned char* sn;
    unsigned char* kv;
    unsigned char* kn;
    unsigned char sect_found;
    unsigned char key_found;
    unsigned char has_content;
    size_t sect_off;
    size_t sect_end_off;
    size_t new_lines;

} semper_write_key_data;

static int semper_skeleton_ini_handler(unsigned char* sn, unsigned char* kn, unsigned char* kv, unsigned char* comm, void* pv)
{
    unused_parameter(comm);
    skeleton_handler_data* shd = pv;
    control_data* cd = shd->pv;

    if(!sn && !kn && !kv)
    {
        return (0);
    }

    if(sn)
    {
        shd->ls = skeleton_add_section(&cd->shead, sn);
    }

    if(kn)
    {
        if(shd->ls == NULL)
        {
            shd->ls = skeleton_add_section(&cd->shead, NULL);
        }

        shd->lk = skeleton_add_key(shd->ls, kn, kv);
        sfree((void**)&shd->kv);
        shd->kv = clone_string(kv);
        return (0);
    }

    if(!kn && kv && shd->ls)
    {
        size_t sl = string_length(shd->kv);
        size_t kvl = string_length(kv);
        unsigned char* tmp = zmalloc(sl + kvl + 1);
        strncpy(tmp, shd->kv, sl);
        strncpy(tmp + sl, kv, kvl);
        sfree((void**)&shd->kv);
        shd->kv = tmp;
        shd->lk = skeleton_add_key(shd->ls, kn, shd->kv);
        return (0);
    }

    return (0);
}

int semper_get_file_timestamp(unsigned char *file,semper_timestamp *st)
{
    if(file==NULL||st==NULL)
    {
        return(-1);
    }

    int res = 0;
#ifdef WIN32
    wchar_t* uni = utf8_to_ucs(file);

    if(uni==NULL)
        return(-1);

    for(size_t i = 0; uni[i]; i++)
    {
        if(uni[i] == (wchar_t)'/')
        {
            uni[i] = (wchar_t)'\\';
        }
    }

    void* h = CreateFileW(uni, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if(h != (void*)-1)
    {
        FILETIME lw = { 0 };
        if(GetFileTime(h, NULL, NULL, &lw))
        {
            st->tm1=lw.dwLowDateTime;
            st->tm2=lw.dwHighDateTime;
        }
        CloseHandle(h);

    }
    else
    {
        res=-1;
    }

    sfree((void**)&uni);
#elif __linux__
    struct stat sts= {0};

    if(stat(file,&sts)==0)
    {
        st->tm1=sts.st_mtim.tv_sec;
        st->tm2=sts.st_mtim.tv_nsec;
    }
    else
    {
        res=-1;
    }

#endif
    return (res);
}

static size_t semper_load_surfaces(control_data* cd)
{
    size_t srf_loaded=0;
    section s = skeleton_first_section(&cd->shead);

    do
    {
        unsigned char* sn = skeleton_get_section_name(s);

        if(sn && strcasecmp("Semper", sn))
        {
            key k = skeleton_get_key(s, "Variant");
            unsigned char* kv = skeleton_key_value(k);

            if(k)
            {
                size_t var = (size_t)compute_formula(kv);

                if(var)
                {
                    if(!surface_load(cd, sn, var))
                    {
                        srf_loaded++;
                    }
                }
            }
        }
    }
    while((s = skeleton_next_section(s, &cd->shead)));

    surface_data *sd=NULL;
    /*do the last step for  initialization*/
    list_enum_part(sd,&cd->surfaces,current)
    {
        surface_init_update(sd);
    }
    return(srf_loaded);
}

static int ini_handler(semper_write_key_handler)
{
    semper_write_key_data* swkd = pv;

    /*Just treat the new line*/
    if(!kn && !kv && !sn && !com)
    {
        size_t pos=ftell(swkd->nf);

        if(pos>3)
        {
            swkd->new_lines++;
            fprintf(swkd->nf, "\n");
        }

        return (0);

    }

    swkd->has_content=1;
    /*The section is about to change so we get the ending offset*/

    if(swkd->sect_off > swkd->sect_end_off && strcasecmp(sn, swkd->sn))
    {
        swkd->sect_end_off = ftello64(swkd->nf) - swkd->new_lines;
        fseek(swkd->nf, -(((long)swkd->new_lines) - 1), SEEK_CUR);
        swkd->new_lines = 0;

        if(swkd->key_found == 0)
        {
            fprintf(swkd->nf, "%s=%s\n", swkd->kn, (swkd->kv ? swkd->kv : (unsigned char*)""));
        }
    }

    /*We have to process a section*/
    if(sn && !kn)
    {
        fprintf(swkd->nf, *com ? "[%s]\t\t%s\n" : "[%s]\n", sn, *com ? com : (unsigned char*)""); /*Spit it out*/

        /*this is the section - store the offset*/
        if(!strcasecmp(sn, swkd->sn))
        {
            swkd->sect_off = ftello64(swkd->nf); /*save the offset of the section start*/
            swkd->key_found = 0;
        }

        return (0);
    }

    /*We found the key so we replace it with the new one*/
    else if(sn && kn && !strcasecmp(sn, swkd->sn) && !strcasecmp(kn, swkd->kn)) /*we have the section and the key so let's print it out*/
    {
        fprintf(swkd->nf, (*com ? "%s=%s\t\t%s\n" : "%s=%s\n"), swkd->kn, (swkd->kv ? swkd->kv : (unsigned char*)""), (*com ? com : (unsigned char*)""));
        swkd->key_found = 1;
        return (0);
    }
    else
    {
        fprintf(swkd->nf, (*com ? "%s=%s\t\t%s\n" : "%s=%s\n"), kn, (kv ? kv : (unsigned char*)""), (*com ? com : (unsigned char*)""));
        return (0);
    }

    /*Write multi-line value*/
    if(!kn)
    {
        fprintf(swkd->nf, (*com ? "%s\t\t;%s\n" : "%s\n"), kv, (*com ? com : (unsigned char*)""));
    }

    return (0);
}

int semper_write_key(unsigned char* file, unsigned char* sn, unsigned char* kn, unsigned char* kv)
{
    if(!file || !sn || !kn || !kv)
        return (-1);

    semper_write_key_data swkd = {.nf = NULL, .sn = sn, .kv = kv, .kn = kn, .sect_found = 0, .key_found = 0 };
    size_t sz = string_length(file);
    unsigned char temp = file[sz - 1];
    static size_t utf8_bom = 0xBFBBEF; /*such UTF-8 signature, much 3 bytes*/


    file[sz - 1] = 0;
    unsigned char* tfn = clone_string(file);
    file[sz - 1] = temp;

#ifdef WIN32
    unsigned short* ucsn = utf8_to_ucs(tfn);
    swkd.nf = _wfopen(ucsn, L"wb+");
#else
    swkd.nf = fopen(tfn, "wb+");
#endif

    fwrite(&utf8_bom, 3, 1, swkd.nf);

    if(swkd.nf == NULL)
        return (-2);

    ini_parser_parse_file(file, semper_write_key_handler, &swkd);
    fflush(swkd.nf);

    if(swkd.sect_off == 0) /*section was not found*/
    {
        if(swkd.has_content)
        {
            fprintf(swkd.nf, "\n[%s]\n%s=%s", sn, kn, kv);
        }
        else
        {
            fprintf(swkd.nf, "[%s]\n%s=%s", sn, kn, kv);
        }
    }
    else if(swkd.key_found == 0 &&
            swkd.sect_end_off < swkd.sect_off) /*this should happen if it is at the end of file*/
    {
        fprintf(swkd.nf, "%s=%s", kn, kv);
    }

    fclose(swkd.nf);

#ifdef WIN32
    unsigned short* ucs = utf8_to_ucs(file);
    SetFileAttributesW(ucs, FILE_ATTRIBUTE_NORMAL);
    _wremove(ucs);
    _wrename(ucsn, ucs);
    sfree((void**)&ucs);
    sfree((void**)&ucsn);
#else
    remove(file);
    rename(tfn, file);
#endif
    sfree((void**)&tfn);

    return (0);
}


/*Loads the configuration from Semper.ini*/
static int semper_load_configuration(control_data* cd)
{
    int ret=-1;
    skeleton_handler_data shd = { cd, NULL, NULL, NULL};

    ret=ini_parser_parse_file(cd->cf, semper_skeleton_ini_handler, &shd);
    cd->smp=skeleton_get_section(&cd->shead,"Semper");

    diag_init(cd);                          /*Initialize diagnostic messages*/
    image_cache_init(cd);                   /*Initialize Unified Image Cache*/

    sfree((void**)&shd.kv);
    return(ret);
}

int semper_save_configuration(control_data* cd)
{
    section s = skeleton_first_section(&cd->shead);

    do
    {
        unsigned char* sn = skeleton_get_section_name(s);

        key k = skeleton_first_key(s);
        /*this is a less efficient method to save the configuration but it
          preserves comments if user adds any*/
        do
        {
            unsigned char* kn = skeleton_key_name(k);
            unsigned char* kv = skeleton_key_value(k);
            semper_write_key(cd->cf, sn, kn, kv);
        }
        while((k = skeleton_next_key(k, s)));
    }
    while((s = skeleton_next_section(s, &cd->shead)));

    return (1);
}

static void semper_create_paths(control_data* cd)
{
    unsigned char* buf = NULL;

#ifdef WIN32

    wchar_t* pth = zmalloc(4096 * 2);
    unsigned long mod_p_sz = 4096;

    QueryFullProcessImageNameW(GetCurrentProcess(), 0, pth, &mod_p_sz);

    for(size_t i = mod_p_sz; i && pth[i] != '\\'; i--)
    {
        pth[i] = 0;

        if(pth[i - 1] == '\\')
        {
            pth[i - 1] = 0;
            break;
        }
    }

    SetCurrentDirectoryW(pth);
    buf = ucs_to_utf8(pth, NULL, 0);
    sfree((void**)&pth);

#elif __linux__

    buf = zmalloc(4096);
    readlink("/proc/self/exe", buf, 4095);
    size_t buf_len = string_length(buf);

    for(size_t i = buf_len; i && buf[i] != '/'; i--)
    {
        buf[i] = 0;

        if(buf[i - 1] == '/')
        {
            buf[i - 1] = 0;
            break;
        }
    }

#endif
    size_t rdl = string_length(buf);
    cd->root_dir = zmalloc(rdl + 1);
    cd->root_dir_length=rdl;
    strcpy(cd->root_dir, buf);

    sfree((void**)&buf);
    //////////////////////////////////////////////////////////
    size_t tsz = rdl + string_length("/Extensions") + 1;
    cd->ext_dir = zmalloc(tsz + 1);
    snprintf(cd->ext_dir,tsz,"%s/Extensions",cd->root_dir);
    cd->ext_dir_length = tsz - 1;
#ifdef WIN32
    windows_slahses(cd->ext_dir);
#elif
    unix_slashes(cd->ext_dir);
#endif
    /////////////////////////////////////////////////////////
    tsz = rdl + string_length("/Semper.ini") + 1;
    cd->cf = zmalloc(tsz);
    snprintf(cd->cf,tsz,"%s/Semper.ini",cd->root_dir);

#ifdef WIN32
    windows_slahses(cd->cf);
#elif __linux__
    unix_slashes(cd->cf);
#endif
    /////////////////////////////////////////////////////////
    tsz = rdl + string_length("/Surfaces") + 1;
    cd->surface_dir = zmalloc(tsz);
    snprintf(cd->surface_dir,tsz,"%s/Surfaces",cd->root_dir);
    cd->surface_dir_length = tsz - 1;

#ifdef WIN32
    windows_slahses(cd->surface_dir);
#elif __linux__
    unix_slashes(cd->surface_dir);
#endif
    ////////////////////////////////////////////////////////
}

static void semper_reload_surfaces_if_modified(control_data* cd)
{
    surface_data* sd = NULL;

    list_enum_part(sd, &cd->surfaces, current)
    {
        if(sd->rim && surface_modified(sd))
        {
            surface_reload(sd);
        }
    }
}

#ifdef WIN32
static void semper_surface_watcher_init(control_data* cd)
{

    if(cd->dh == NULL)
    {
        wchar_t* s_ucs = utf8_to_ucs(cd->surface_dir);
        cd->dh = CreateFileW(s_ucs, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS, NULL);
        sfree((void**)&s_ucs);
    }

    if(cd->notify_buf == NULL)
    {
        cd->notify_buf = zmalloc(sizeof(FILE_NOTIFY_INFORMATION) * 256);
    }

    if(cd->so == NULL)
    {
        cd->so = zmalloc(sizeof(semper_overlapped));
    }

    cd->so->pv = cd;

    ReadDirectoryChangesW(cd->dh, cd->notify_buf, sizeof(FILE_NOTIFY_INFORMATION) * 256, 1,
                          FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
                          FILE_NOTIFY_CHANGE_LAST_WRITE,
                          NULL, (OVERLAPPED*)cd->so, (LPOVERLAPPED_COMPLETION_ROUTINE)semper_surface_watcher);
}
#endif


#ifdef WIN32
static int semper_font_cache_fix(unsigned char *path)
{
    if(path==NULL)
    {
        return(-1);
    }

    size_t path_len=string_length(path);

    unsigned char *fltp=zmalloc(path_len+4);
    snprintf(fltp,path_len+4,"%s/*",path);
    windows_slahses(fltp);
    uniform_slashes(fltp);
    WIN32_FIND_DATAW wfd= {0};
    unsigned short *uc=utf8_to_ucs(fltp);
    sfree((void**)&fltp);
    void *p=FindFirstFileW(uc,&wfd);
    sfree((void**)&uc);

    if(p==INVALID_HANDLE_VALUE)
    {
        return(-1);
    }

    do
    {
        unsigned char *temp=ucs_to_utf8(wfd.cFileName,NULL,0);

        if(!strcasecmp("..",temp)||!strcasecmp(".",temp))
        {
            sfree((void**)&temp);
            continue;
        }

        size_t fsz=string_length(temp);
        unsigned char *ffp=zmalloc(fsz+path_len+3);
        snprintf(ffp,fsz+path_len+3,"%s/%s",path,temp);
        uniform_slashes(ffp);
        windows_slahses(ffp);

        sfree((void**)&temp);

        if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
        {
            if(is_file_type(ffp,"NEW"))
            {
                unsigned char *s=zmalloc(fsz+path_len+3+7);
                strncpy(s,ffp,fsz+path_len-3);

                unsigned short *tmp1=utf8_to_ucs(s);
                unsigned short *tmp2=utf8_to_ucs(ffp);

                _wremove(tmp1);
                _wrename(tmp2,tmp1);

                sfree((void**)&s);
                sfree((void**)&tmp1);
                sfree((void**)&tmp2);
            }
        }

        sfree((void**)&ffp);

    }
    while(FindNextFileW(p,&wfd));

    FindClose(p);

    return(0);
}
#endif


void semper_surface_watcher(unsigned long err, unsigned long transferred, void *pv)
{

    unused_parameter(err);
    unused_parameter(transferred);
#ifdef WIN32
    semper_overlapped* so = (semper_overlapped*)pv;
    control_data* cd = so->pv;
    // semper_load_local_fonts(cd->root_dir);

    semper_reload_surfaces_if_modified(cd);
    event_push(cd->eq, (event_handler)semper_surface_watcher_init, (void*)cd, 0, 0);
#elif __linux__
    control_data *cd=pv;
    struct pollfd p= {0};

    p.events=POLLIN;
    p.fd=cd->inotify_fd;
    poll(&p,1,0);

    if(p.revents)
    {
        struct inotify_event inev= {0};
        size_t buf_len=sizeof(struct inotify_event) + NAME_MAX + 1;

        char buf[sizeof(struct inotify_event) + NAME_MAX + 1]= {0};

        while(read(cd->inotify_fd,buf,buf_len)>0); //consume the events

        semper_reload_surfaces_if_modified(cd);
    }

#endif
}


#ifdef WIN32

#endif

/*Experimental work to prevent
 *'Show Desktop' from hiding the widgets
 * */

/*A quick thought on how this should work:
 * 1) We do have a helper window that stays at the bottom of the stack - on top of the desktop window
 * 2) Every crosswin_desktop must be pushed on top of this window in reversed order (stack)
 * 3) every crosswin_bottom has to be added after crosswin_desktop
 * 4) Every crosswin_normal has to be added after crosswin_bottom
 * 5) The crosswin_top* are pushed as regular
 * 6) The process must repeat to ensure the order remains set
 **/

static void draw(void *p,void *cr)
{
    cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
    cairo_set_color(cr,0xffff0000);
    cairo_paint(cr);
}


static int semper_desktop_checker(control_data *cd)
{
    event_push(cd->eq,(event_handler)semper_desktop_checker,cd,100,EVENT_PUSH_TIMER|EVENT_PUSH_TAIL); //schedule another check
      crosswin_update_z(&cd->c);
      return(0);
    static unsigned char state=1;
    surface_data *sd=NULL;
    void *fg=NULL;
    void *sh=NULL;


#ifdef WIN32
    fg=GetForegroundWindow();
    sh=FindWindowExW(fg,NULL,L"SHELLDLL_DefView",L"");
#endif


    if(sh==NULL)
    {
        crosswin_window *cw=NULL;
        list_enum_part_backward(cw,&cd->c.windows,current)
        {
            if(cw->zorder==crosswin_bottom)
                crosswin_set_window_z_order(cw,crosswin_bottom);
        }
    }
    else
    {
        crosswin_set_window_z_order(cd->c.helper,crosswin_normal);
    }

    //The shell has the focus.
    //This could mean one of two things:
    //1) The user is on desktop (which is fine)
    //2) The user just triggered the 'ShowDesktop' command (which is fine but we must take care of it)


    if(cd->srf_reg)
    {
        sd=cd->srf_reg;
        crosswin_set_window_z_order(sd->sw,crosswin_topmost);
        crosswin_set_window_z_order(sd->sw,crosswin_normal);
    }


    return(0);
}

#ifdef WIN32
static void  semper_init_fonts(control_data *cd)
{
    FcFini();
    size_t fcroot_len=cd->root_dir_length+sizeof("/.fontcache");
    unsigned char *fcroot=zmalloc(fcroot_len);
    snprintf(fcroot,fcroot_len,"%s\\.fontcache",cd->root_dir);
    unsigned short *tmp=utf8_to_ucs(fcroot);

    if(_waccess(tmp,0)!=0)
    {
        _wmkdir(tmp);
    }
#if 1
    unsigned int attr=GetFileAttributesW(tmp);
    SetFileAttributesW(tmp,attr|FILE_ATTRIBUTE_HIDDEN);
#endif
    _wputenv_s(L"FONTCONFIG_PATH",tmp);
    _wputenv_s(L"FONTCONFIG_FILE",L".fontcfg");
    sfree((void**)&tmp);


    static unsigned char config[]=
    {
#include <font_config.xml>
    };

    size_t fcfg_len=fcroot_len+sizeof("\\.fontcfg");
    unsigned char *win_fonts=expand_env_var("%systemroot%\\fonts");
    unsigned char *fcfg_file=zmalloc(fcfg_len);
    snprintf(fcfg_file,fcfg_len,"%s/.fontcfg",fcroot);
    tmp=utf8_to_ucs(fcfg_file);
    FILE *f=_wfopen(tmp,L"wb");

    if(f)
    {
        fwrite(config,sizeof(config)-1,1,f);
        fprintf(f,"<dir>%s</dir>\n<dir>%s</dir>\n<cachedir>%s</cachedir>\n</fontconfig>",win_fonts,cd->surface_dir,fcroot);
        fclose(f);
    }

    FcInit();
    FcCacheCreateTagFile( FcConfigGetCurrent());
    semper_font_cache_fix(fcroot);
    FcFini();
    FcInit();
    sfree((void**)&tmp);
    sfree((void**)&fcfg_file);
    sfree((void**)&win_fonts);
    sfree((void**)&fcroot);
}
#endif


int main(void)
{

    control_data* cd = zmalloc(sizeof(control_data));
    crosswin_init(&cd->c);

    list_entry_init(&cd->shead);
    list_entry_init(&cd->surfaces);
    cd->eq = event_queue_init();
    semper_create_paths(cd);
    semper_load_configuration(cd);
#ifdef WIN32
    diag_info("Initializing font cache...this takes time on some machines");
    semper_init_fonts(cd);
    diag_info("Fonts initialized");
    event_push(cd->eq, (event_handler)semper_surface_watcher_init, (void*)cd, 0, 0);

#elif __linux__
    event_queue_set_window_event(cd->eq,XConnectionNumber(cd->c.display));
    cd->inotify_fd=inotify_init1(IN_NONBLOCK);
    event_queue_set_inotify_event(cd->eq,cd->inotify_fd);
#endif

    if(semper_load_surfaces(cd)==0)
    {
        /*launch the catalog if no surface has been loaded due to various reasons*/
        surface_builtin_init(cd,catalog);
    }
#if 0
    semper_desktop_checker(cd);
#endif
    while(cd->c.quit == 0) //nothing fancy, just the main event loop
    {
        event_wait(cd->eq); // wait for an event to occur
#ifdef __linux__
        semper_surface_watcher(0,0,cd);
#endif

        if(crosswin_update(&cd->c))
        {
            surface_data *sd=NULL;
            list_enum_part(sd,&cd->surfaces,current)
            {
                surface_reload(sd);
            }
        }

        crosswin_message_dispatch(&cd->c); // dispatch any windows message
        event_process(cd->eq);             // process the queue
    }

    semper_save_configuration(cd);
    return (0);
}
