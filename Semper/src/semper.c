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
#include <watcher.h>

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
        swkd->sect_end_off = ftello(swkd->nf) - swkd->new_lines;
        fseeko(swkd->nf, -(((long)swkd->new_lines) - 1), SEEK_CUR);
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
            swkd->sect_off = ftello(swkd->nf); /*save the offset of the section start*/
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



    if(swkd.nf == NULL)
        return (-2);
    setvbuf(swkd.nf,NULL,_IONBF,0);
    fwrite(&utf8_bom, 3, 1, swkd.nf);
    ini_parser_parse_file(file, semper_write_key_handler, &swkd);

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
#elif __linux__
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


static int semper_single_instance(control_data *cd)
{
    FILE *f=NULL;
    unsigned char *lock_file=zmalloc(cd->root_dir_length+7);
    snprintf(lock_file,cd->root_dir_length+7,"%s/.lock",cd->root_dir);

#ifdef WIN32
    windows_slahses(lock_file);
    unsigned short *uc=utf8_to_ucs(lock_file);
    if(_wremove(uc)==0||_waccess(uc,0)!=0)
    {
        f=_wfopen(uc,L"w");
        unsigned int attr=GetFileAttributesW(uc);
        SetFileAttributesW(uc,attr|FILE_ATTRIBUTE_HIDDEN);
    }
    sfree((void**)&uc);
#elif __linux__
    if(remove(lock_file)==0||access(lock_file,0)!=0)
    {
        f=fopen(lock_file,"w");
    }
#endif
    sfree((void**)&lock_file);

    return(f!=NULL);
}


static int semper_watcher_callback(void *pv,void *wait)
{
    surface_data* sd = NULL;
    control_data *cd=pv;
    list_enum_part(sd, &cd->surfaces, current)
    {
        if(sd->rim && surface_modified(sd))
        {
            surface_reload(sd);
        }
    }
    watcher_next(cd->watcher);
    return(0);
}

static int semper_check_screen(control_data *cd)
{
    if(crosswin_update(&cd->c))
    {
        surface_data *sd=NULL;
        list_enum_part(sd,&cd->surfaces,current)
        {
            surface_reload(sd);
        }
    }
    return(0);
}


int main(void)
{
    control_data* cd = zmalloc(sizeof(control_data));
    semper_create_paths(cd);

    if(semper_single_instance(cd)==0)
    {
        return(0);
    }

    crosswin_init(&cd->c);
    list_entry_init(&cd->shead);
    list_entry_init(&cd->surfaces);
    cd->eq = event_queue_init();
    cd->watcher=watcher_init(cd->surface_dir);


    event_add_wait(cd->eq,(event_wait_handler)crosswin_message_dispatch,&cd->c,cd->c.disp_fd,0x1);
    event_add_wait(cd->eq,(event_wait_handler)semper_check_screen,cd,NULL,0);

    if(cd->watcher)
    {
        event_add_wait(cd->eq,(event_wait_handler)semper_watcher_callback,cd,(void*)((size_t*)cd->watcher)[0],0x1);
    }
    semper_load_configuration(cd);

#ifdef WIN32
    diag_info("Initializing font cache...this takes time on some machines");
    semper_init_fonts(cd);
    diag_info("Fonts initialized");
#endif


    if(semper_load_surfaces(cd)==0)
    {
        /*launch the catalog if no surface has been loaded due to various reasons*/
        surface_builtin_init(cd,catalog);
    }

    while(cd->c.quit == 0) //nothing fancy, just the main event loop
    {
        if(event_wait(cd->eq))                 // wait for an event to occur
        {
            event_process(cd->eq);             // process the queue
        }
    }

    semper_save_configuration(cd);
    return (0);
}
