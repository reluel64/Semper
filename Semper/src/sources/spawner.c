#include <sources/source.h>
#include <mem.h>
#include <semper_api.h>
#include <pthread.h>
#include <string_util.h>
#include <parameter.h>
#include <xpander.h>

#if defined(__linux__)
#define __USE_GNU
#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

typedef struct
{
#if defined(WIN32)
        unsigned char *command;
#elif defined(__linux__)
        unsigned char *bin_path;
        unsigned char *params;
#endif
        unsigned char *finish_command;
        unsigned char *working_dir;
        unsigned char *file_out;
        pthread_mutex_t mtx;
        pthread_t thread;
        int status;
        void *th_active;
        void *kill;
        unsigned char *ret_str;
        unsigned char *raw_str;
        size_t raw_str_len;
        size_t ph;
        size_t th;
        void *ip;
}spawner_state;

static void *spawner_worker(void *pv);

#if defined(WIN32)
static int spawner_kill_by_window(HWND window, LPARAM lpm);
#elif defined(__linux__)
static unsigned char **spawner_param_to_argv(unsigned char *str);
#endif

void spawner_init(void **spv,void *ip)
{

    spawner_state *st =zmalloc(sizeof(spawner_state));
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&st->mtx, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    st->th_active =  safe_flag_init();
    st->kill = safe_flag_init();
    st->ph = -1;
    st->th = -1;
    st->ip = ip;
    *spv = st;
}

void spawner_reset(void *spv, void *ip)
{
    spawner_state *st = spv;
    pthread_mutex_lock(&st->mtx);
#if defined(WIN32)
    sfree((void**)&st->command);
    sfree((void**)&st->file_out);
#elif defined(__linux__)
    sfree((void**)&st->params);
    sfree((void**)&st->bin_path);
#endif
    sfree((void**)&st->working_dir);

    unsigned char *app=  parameter_string(ip,"Application",NULL,XPANDER_SOURCE);
    unsigned char *param = parameter_string(ip,"Parameter",NULL,XPANDER_SOURCE);


    if(app == NULL)
    {
#if defined(WIN32)
        app = expand_env_var("%COMSPEC% /u /c"); //no need to call uniform_slashes
#elif defined(__linux__)
        app = expand_env_var("$SHELL");
        size_t pm_len = string_length(param);
        unsigned char *tmp = zmalloc(pm_len+4);
        snprintf(tmp,pm_len+4,"-c %s",param);
        sfree((void**)&param);
        param = tmp;
#endif
    }
    else
    {
        uniform_slashes(app);
    }
#if defined(WIN32)
    size_t len = string_length(app) + string_length(param)+2;
    st->command = zmalloc(string_length(app) + string_length(param)+2);
    snprintf(st->command,len,"%s %s",app,param);
    sfree((void**)&app);
    sfree((void**)&param);

#elif defined(__linux__)
    st->bin_path = app;
    st->params = param;
#endif

    st->working_dir = parameter_string(ip,"WorkingDir",NULL,XPANDER_SOURCE);
    st->file_out = clone_string(parameter_string(ip,"OutputFile",NULL,XPANDER_SOURCE));

    pthread_mutex_unlock(&st->mtx);
}

double spawner_update(void *spv)
{
    spawner_state *st = spv;

    pthread_mutex_lock(&st->mtx);
    double status =(double) st->status;
    if(st->raw_str)
    {
        sfree((void**)&st->ret_str);
        st->ret_str=zmalloc(st->raw_str_len+1);
        memcpy(st->ret_str,st->raw_str,st->raw_str_len);
    }
    pthread_mutex_unlock(&st->mtx);

    if(safe_flag_get(st->th_active) == 0 && st->thread)
    {
        sfree((void**)&st->raw_str);
        st->raw_str_len=0;
        pthread_join(st->thread,NULL);
        st->thread = 0;
    }

    return(status);
}

unsigned char *spawner_string(void *spv)
{
    spawner_state *st = spv;
    return(st->ret_str);
}

void spawner_command(void *spv, unsigned char *comm)
{
    spawner_state *st = spv;
    if(comm)
    {
        if(!strcasecmp(comm,"Run") && !safe_flag_get(st->th_active))
        {
            if(safe_flag_get(st->th_active) == 0 && st->thread)
            {
                pthread_join(st->thread,NULL);
                st->thread = 0;
            }
            sfree((void**)&st->raw_str);
            st->raw_str_len=0;
            safe_flag_set(st->th_active,1);

            if(pthread_create(&st->thread,NULL,spawner_worker,st))
            {
                safe_flag_set(st->th_active,0);
            }
            else
            {
                while(safe_flag_get(st->th_active) != 2)
                    sched_yield();
            }
        }
        else if(!strcasecmp(comm,"Stop") && safe_flag_get(st->th_active) == 2 && st->ph >0)
        {
#if defined(WIN32)
            EnumWindows(spawner_kill_by_window,GetProcessId((void*)st->ph));
#elif defined(__linux__)
            pthread_mutex_lock(&st->mtx);
            pid_t pid = st->ph;
            pthread_mutex_unlock(&st->mtx);
            kill(pid,SIGTERM);
#endif
        }
        else if(!strcasecmp(comm,"ForceStop") && safe_flag_get(st->th_active) == 2 && st->ph >0)
        {
#if defined(WIN32)
            TerminateProcess((void*)st->ph,0);
            safe_flag_set(st->kill,1);
            pthread_join(st->thread,NULL);
            safe_flag_set(st->kill,0);

#elif defined(__linux__)
            pthread_mutex_lock(&st->mtx);
            pid_t pid = st->ph;
            pthread_mutex_unlock(&st->mtx);
            kill(pid,SIGKILL);
            safe_flag_set(st->kill,1);
            pthread_join(st->thread,NULL);
            safe_flag_set(st->kill,0);
#endif
        }
    }
}

void spawner_destroy(void **spv)
{
    spawner_state *st = *spv;

    safe_flag_set(st->kill,1);

    if(st->thread)
    {
        pthread_join(st->thread,NULL);
    }

    sfree((void**)&st->raw_str);
    sfree((void**)&st->ret_str);

    sfree((void**)&st->working_dir);

#if defined(WIN32)
    sfree((void**)&st->command);
#elif defined(__linux__)
    sfree((void**)&st->params);
    sfree((void**)&st->bin_path);
#endif


    safe_flag_destroy(&st->kill);
    safe_flag_destroy(&st->th_active);
    pthread_mutex_destroy(&st->mtx);
    sfree(spv);

}

#if defined(WIN32)
static int spawner_kill_by_window(HWND window, LPARAM lpm)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(window,&pid);

    if(lpm == pid)
    {
        PostMessageA(window,WM_CLOSE,0,0);
    }
    return(TRUE);
}
#endif

static int is_utf16(unsigned char *buf,size_t len)
{
    for(size_t i=0;i<len;i++)
    {
        if((i>0 && buf[i-1]!=0 && buf[i]==0))
            return(1);
    }
    return(0);
}

static void spawner_convert_and_append(unsigned char *raw, size_t raw_len, unsigned char **buf, size_t *buf_pos)
{
    unsigned char *utf8 = raw;
    size_t bn = raw_len;
    unsigned char *temp = NULL;

    if(is_utf16(raw,raw_len))
    {
        utf8 = ucs_to_utf8((unsigned short*)raw,&bn,0);
    }


    temp = realloc(*buf,(*buf_pos) + bn + 3);

    if(temp)
    {
        memcpy(temp+(*buf_pos),utf8,bn);
        *buf=temp;
        (*buf_pos)+=bn;
        memset(&temp[*buf_pos],0,3);
    }

    if(utf8!=raw)
    {
        sfree((void**)&utf8);
    }

}

static void spawner_write_to_file(unsigned char *path, void *buf, size_t buf_len)
{
    FILE *fp = NULL;
    unsigned char bom[3] = {0xEF,0xBB,0xBF};
#if defined(WIN32)
    unsigned short *upath =utf8_to_ucs(path);
    if(upath)
    {
        fp = _wfopen(upath,L"wb");
        sfree((void**)&upath);
    }
#elif defined(__linux__)
    fp = fopen(path,"wb");
#endif

    if(fp)
    {
        fwrite(bom, 1, sizeof(bom), fp);
        fwrite(buf, 1, buf_len, fp);
        fclose(fp);
    }
}

static void *spawner_worker(void *pv)
{
    spawner_state *st = pv;
#if defined(WIN32)
    void *app_std_in = NULL;
    void *app_std_out = NULL;
    void *app_std_err = NULL;
    void *std_in = NULL;
    void *std_out = NULL;
    void *std_err = NULL;
#elif defined(__linux__)
    int app_std_in = -1;
    int app_std_out = -1;
    int app_std_err = -1;
    int std_in = -1;
    int std_out = -1;
    int std_err = -1;
#endif

    unsigned char *raw_str = NULL;
    size_t raw_len = 0;
    safe_flag_set(st->th_active,2);

#if defined(WIN32)

    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if(!CreatePipe(&std_out, &app_std_out, &saAttr, 0x100000)) //from application to Semper
    {
        safe_flag_set(st->th_active,0);
        return(NULL);
    }
    if(!CreatePipe(&app_std_in, &std_in, &saAttr,0x100000)) //to application from Semper
    {
        CloseHandle(std_out);
        CloseHandle(app_std_out);
        safe_flag_set(st->th_active,0);
        return(NULL);
    }

    if(!CreatePipe(&std_err, &app_std_err, &saAttr, 0x100000)) //from application to Semper
    {
        CloseHandle(std_in);
        CloseHandle(app_std_out);
        CloseHandle(app_std_in);
        CloseHandle(std_out);
        safe_flag_set(st->th_active,0);
        return(NULL);
    }

    si.hStdError = app_std_err;
    si.hStdInput = app_std_in;
    si.hStdOutput = app_std_out;
    si.wShowWindow=SW_SHOW;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.cb = sizeof(STARTUPINFOW);
    pthread_mutex_lock(&st->mtx);
    unsigned short *cmd = utf8_to_ucs(st->command);
    unsigned short *wd  = utf8_to_ucs(st->working_dir);
    unsigned char *fout = clone_string(st->file_out);
    pthread_mutex_unlock(&st->mtx);

    if(CreateProcessW(NULL,cmd,NULL,NULL,1,0,NULL,wd,&si,&pi))
    {
        void *harr[3] = {std_out,std_err,pi.hProcess};

        pthread_mutex_lock(&st->mtx);
        st->ph =(size_t) pi.hProcess;
        st->th =(size_t) pi.hThread;
        pthread_mutex_unlock(&st->mtx);

        DWORD written = 0;

        WriteFile(std_in,cmd,wcslen(cmd)*2,&written,NULL);


        while(safe_flag_get(st->kill) == 0)
        {
            WaitForMultipleObjects(3, harr, 0, -1);



            if(!WaitForSingleObject(std_out,0))
            {
                DWORD buf_sz = 0;

                if(PeekNamedPipe(std_out,NULL,0,NULL,&buf_sz,NULL) && buf_sz)
                {
                    unsigned char *buf = zmalloc(buf_sz+3);

                    if(buf)
                    {
                        DWORD readed = 0;
                        ReadFile(std_out,buf ,buf_sz,&readed,NULL);
                        spawner_convert_and_append(buf,buf_sz,&raw_str,&raw_len);
                    }
                    sfree((void**)&buf);
                }
            }

            if(!WaitForSingleObject(std_err,0))
            {
                DWORD buf_sz = 0;
                if(PeekNamedPipe(std_err,NULL,0,NULL,&buf_sz,NULL) && buf_sz)
                {
                    unsigned char *buf = zmalloc(buf_sz+3);

                    if(buf)
                    {
                        DWORD readed = 0;
                        ReadFile(std_err,buf ,buf_sz,&readed,NULL);

                        spawner_convert_and_append(buf,buf_sz,&raw_str,&raw_len);

                    }
                    sfree((void**)&buf);
                }
            }

            if(WaitForSingleObject(pi.hProcess,100)==0)
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                pi.hProcess = NULL;
                break;
            }
        }
    }

    CloseHandle(std_in);
    CloseHandle(app_std_out);
    CloseHandle(app_std_in);
    CloseHandle(std_out);
    CloseHandle(app_std_err);
    CloseHandle(std_err);

    if(pi.hProcess)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }


#elif defined(__linux__)

    int temp_p[2];
    int have_pwd = 0;
    int env_cnt = 0;
    unsigned char **env = NULL;
    pid_t pid = -1;
    /*STD_IN*/
    if(pipe2(temp_p,O_NONBLOCK))
    {
        return(NULL);
    }
    app_std_in = temp_p[1];
    std_in = temp_p[0];

    /*STD_OUT*/
    if(pipe2(temp_p,O_NONBLOCK))
    {
        close(app_std_in);
        close(std_in);
        return(NULL);
    }

    app_std_out =temp_p[0];
    std_out = temp_p[1];

    /*STD_ERR*/
    if(pipe2(temp_p,O_NONBLOCK))
    {
        close(app_std_in);
        close(std_in);
        close(app_std_out);
        close(std_out);
        return(NULL);
    }

    app_std_err = temp_p[0];
    std_err = temp_p[1];
    pthread_mutex_lock(&st->mtx);
    unsigned char *fout = clone_string(st->file_out);
    unsigned char *bin = clone_string(st->bin_path);
    unsigned char *params = clone_string(st->params);
    unsigned char *wd  = clone_string(st->working_dir);
    unsigned char **args = spawner_param_to_argv(params);
    extern char **environ;

    for(int i=0;environ[i];i++)
    {
        unsigned char **tenv = realloc(env,sizeof(char*)*(env_cnt+2));

        if(tenv)
        {
            env = tenv;

            if(strncasecmp(environ[i], "PWD", 3) || !wd)
            {
                env[env_cnt] = zmalloc(string_length(environ[i])+1);
                strncpy(env[env_cnt], environ[i], string_length(environ[i]));

            }
            else
            {
                env[env_cnt] = zmalloc(string_length(wd)+5);
                sprintf(env[env_cnt],"PWD=%s",wd);
                have_pwd = 1;
            }
            env_cnt++;
        }
    }

    if(have_pwd == 0)
    {
        unsigned char **tenv = realloc(env,sizeof(char*)*(env_cnt+2));
        if(tenv)
        {
            env = tenv;
            env[env_cnt] = zmalloc(string_length(wd)+1);
            strcpy(env[env_cnt], wd);
            have_pwd = 1;
            env_cnt++;
        }
    }

    env[env_cnt] = 0;

    for(size_t i=0;env[i];i++)
    {
        printf("%s\n",env[i]);
    }


    pthread_mutex_unlock(&st->mtx);

    if(args != NULL)
    {
        args[0] =  bin;
    }
    else
    {
        args = zmalloc(sizeof(char*)*2);
        args[0] = bin;
    }
#if defined(DEBUG)
    for(int  i=1;args[i];i++)
    {
        diag_info("%s: %s",__FUNCTION__,args[i]);
    }
#endif
    posix_spawn_file_actions_t file_act;
    posix_spawnattr_t attr;
    posix_spawn_file_actions_init(&file_act);
    posix_spawn_file_actions_adddup2(&file_act, std_out, 1);
    posix_spawn_file_actions_adddup2(&file_act, std_err, 2);
    posix_spawnattr_init(&attr);

    if(!posix_spawn(&pid,bin,&file_act,&attr,(char**)args,(char**)env))
    {
        pthread_mutex_lock(&st->mtx);
        st->ph = pid;
        pthread_mutex_unlock(&st->mtx);

        while(1)
        {
            struct pollfd events[2];
            int status = 0;
            int ret = 0;
            memset(events,0,sizeof(events));
            events[0].fd=(int)(size_t)app_std_out;
            events[0].events = POLLIN;
            events[1].fd=(int)(size_t)app_std_err;
            events[1].events = POLLIN;
            poll(events, 2, 1);


            if(events[0].revents)
            {
                char buf[4096];
                ssize_t tr = 0;

                while((tr = read(app_std_out,buf,4096))>0)
                {
                    spawner_convert_and_append(buf,tr,&raw_str,&raw_len);
                }
            }

            if(events[1].revents)
            {
                char buf[4096];
                ssize_t tr = 0;

                while((tr = read(app_std_err,buf,4096))>0)
                {
                    spawner_convert_and_append(buf,tr,&raw_str,&raw_len);
                }
            }

            ret = waitpid(pid,&status,WNOHANG);

            if(ret == -1)
            {
                break;
            }

        }
    }

    close(std_in);
    close(app_std_out);
    close(app_std_in);
    close(std_out);
    close(app_std_err);
    close(std_err);
#endif

#if defined(WIN32)
    sfree((void**)&cmd);
#elif defined(__linux__)
    sfree((void**)&params);
    sfree((void**)&bin);

    for(size_t i = 1;args[i];i++)
    {
        sfree((void**)&args[i]);
    }

    sfree((void**)&args);

    for(size_t i = 0; i<env_cnt;i++)
    {
        sfree((void**)&env[i]);
    }

    sfree((void**)&env);
#endif

    sfree((void**)&wd);

    spawner_write_to_file(fout,raw_str,raw_len);
    sfree((void**)&fout);
    pthread_mutex_lock(&st->mtx);
    st->raw_str = raw_str;
    st->raw_str_len = raw_len;
    st->ph = -1;
    st->th = -1;
    pthread_mutex_unlock(&st->mtx);
    safe_flag_set(st->th_active,0);
    return(NULL);
}



#if defined(__linux__)
static int spawner_filter(string_tokenizer_status *sts, void* pv)
{
    size_t *tokens = pv;

    if(sts->reset)
    {
        memset(tokens,0,sizeof(size_t)*2);

        return(0);
    }

    if(sts->buf[sts->pos]=='\\')
    {
        tokens[1]++;
        return(0);
    }

    if(sts->buf[sts->pos] == '"' || sts->buf[sts->pos] == '\'')
    {
        if(tokens[1])
        {
            tokens[1] = 0;
            return(0);
        }
        tokens[0]++;

    }

    if(tokens[0] % 2 && sts->buf[sts->pos] == ' ')
    {
        return (0);
    }
    else if(sts->buf[sts->pos] == ' ')
    {
        return (1);
    }

    return (0);
}



static unsigned char **spawner_param_to_argv(unsigned char *str)
{
    if(str == NULL)
    {
        return(NULL);
    }


    unsigned char **res = NULL;
    string_tokenizer_info sti = {0};
    size_t tokens[2]={0};
    sti.buffer = str;
    sti.string_tokenizer_filter = spawner_filter;
    sti.filter_data = &tokens;
    string_tokenizer(&sti);
    size_t pc = 2;
    unsigned char **temp = NULL;
    for(size_t i = 0; i < sti.oveclen / 2; i++)
    {

        size_t start = sti.ovecoff[i * 2];
        size_t end = sti.ovecoff[i * 2 + 1];

        if(string_strip_space_offsets(sti.buffer, &start, &end) == 0)
        {
            if(sti.buffer[start] == '"' || sti.buffer[start] == '\'')
                start++;

            if(sti.buffer[end - 1] == '"' || sti.buffer[end - 1] == '\'')
                end--;
        }

        if(end == start)
            continue;

        temp = realloc(res,sizeof(char*) * (pc+1));

        if(temp)
        {
            temp[pc-1] = zmalloc((end - start) + 1);
            strncpy(temp[pc-1],sti.buffer+start,end-start);
            res = temp;
            res[pc] = NULL;
            pc++;
        }
    }

    sfree((void**)&sti.ovecoff);


    return(res);
}
#endif
