/*
 * WebGet
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#define CURL_STATICLIB
#ifdef WIN32
#include <winsock2.h>
#include <wchar.h>
#elif __linux__
#include <sys/stat.h>
#endif
#include <semper_api.h>
#include <pcre.h>
#include <sources/source.h>
#include <stdio.h>
#include <stdlib.h>
#include <string_util.h>
#include <mem.h>
#include <pthread.h>
#include <curl/curl.h>

#define OVECTOR_LIMIT 300
#define DOWNLOAD_TO_BUFFER 0x1
#define DOWNLOAD_TO_FILE   0x2
#define DOWNLOAD_TO_FILE_BUFFER DOWNLOAD_TO_BUFFER|DOWNLOAD_TO_FILE
typedef struct webget webget;

typedef struct
{
    unsigned char *buf;
    size_t buf_pos;
    size_t buf_sz;
    unsigned char status;
    void *stop;
    unsigned char dwl_mode;
    unsigned char *save_root_dir;
    unsigned char *fp;
    unsigned char *old_fp;
    FILE *f;
} webget_worker;

typedef enum
{
    enc_utf8, //we're not sure how to handle non-text files
    enc_ucs_le,
    enc_ucs_be
} webget_encoding;

typedef struct webget
{

    pthread_t worker;               //worker thread
    pthread_mutex_t mutex;          //mutex to protect a resource to be accessed while it is modified
    list_entry children;            //head for the children list
    list_entry current;             //used if the entry is part of a list (is a child)
    void *ip;                       //internal pointer (used for send_command())
    unsigned char *address;         //could be a url or a file
    unsigned char *srf_dir;         //used when downloading a file in the surface's directory
    unsigned char *err_str;         //error string returned if something fails
    unsigned char *regexp;          //regular expression
    unsigned char *success_act;     //success action
    unsigned char *parse_fail_act;  //parse failure action
    unsigned char *dwl_fail;        //download failure action
    unsigned char *connect_fail;    //connecton failure action (timeout)
    unsigned char *result;          //this is returned by webget_string()
    unsigned char *temp;            //temporary holder which is used to update the children (basically it's contents depend on primary_index)
    unsigned char dwl;              //should we download this?
    unsigned char dwl_local;        //should we download this to a file? (it implies dwl)
    void *update;                   //valid only for children
    void *stop;                     //signal the worker to stop
    void *work;                     //worker status
    size_t pr_index;                //primary index  (extracted from the parent)
    size_t sec_index;               //secondary index (extracted after applying regexp on the primary index)
    size_t u_rate;                  //update rate
    size_t c_rate;                  //current rate
    webget *parent;                 //used if the child has a parent
    webget_worker *ww;              //status for the worker
} webget;

static int webget_post_worker_update(webget *w);
static int webget_apply_regexp(unsigned char *buffer, size_t buf_sz, unsigned char *regexp, int *ovector, size_t ovec_sz, int *match_count);
static void *webget_worker_thread(void *p);

void webget_init(void **spv, void *ip)
{
    unused_parameter(ip);
    webget *w = zmalloc(sizeof(webget));
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&w->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    list_entry_init(&w->children);
    list_entry_init(&w->current);
    w->update = safe_flag_init();
    w->work = safe_flag_init();
    w->stop = safe_flag_init();
    *spv = w;
}

void webget_reset(void *spv, void *ip)
{
    webget *w = spv;
    void *pip = NULL;
    unsigned char *temp = NULL;

    pthread_mutex_lock(&w->mutex);
    sfree((void**)&w->address);
    sfree((void**)&w->regexp);
    sfree((void**)&w->success_act);
    sfree((void**)&w->parse_fail_act);
    sfree((void**)&w->dwl_fail);
    sfree((void**)&w->connect_fail);
    sfree((void**)&w->srf_dir);
    w->regexp = clone_string(param_string("Regexp", EXTENSION_XPAND_ALL, ip, NULL));
    w->sec_index = param_size_t("SecondaryIndex", ip, 0);
    w->pr_index = param_size_t("PrimaryIndex", ip, 0);
    w->u_rate = param_size_t("UpdateRate", ip, 1000);
    w->dwl_local = param_bool("DownloadLocal", ip, 0);
    w->dwl = param_size_t("Download", ip, 0);
    w->srf_dir = clone_string(absolute_path(ip, "", EXTENSION_PATH_SURFACE));
    temp = param_string("URL", EXTENSION_XPAND_VARIABLES, ip, NULL);

    if((pip = get_parent(temp, ip)) != NULL && w->parent == NULL)
    {
        w->parent = get_private_data(pip);

        if(linked_list_empty(&w->current))
            linked_list_add_last(&w->current, &w->parent->children);
    }
    else if(pip == NULL)
    {
        linked_list_remove(&w->current);
        list_entry_init(&w->current);
        w->address = clone_string(temp);
    }

    if(pip == NULL || (pip && w->dwl))
    {
        w->success_act = clone_string(param_string("SuccessAction", EXTENSION_XPAND_ALL, ip, NULL));
        w->parse_fail_act = clone_string(param_string("ParseFailAction", EXTENSION_XPAND_ALL, ip, NULL));
        w->dwl_fail = clone_string(param_string("DownloadFailAction", EXTENSION_XPAND_ALL, ip, NULL));
        w->connect_fail = clone_string(param_string("ConnetionFailAction", EXTENSION_XPAND_ALL, ip, NULL));
    }

    pthread_mutex_unlock(&w->mutex);
}




double webget_update(void *spv)
{
    webget *w = spv;

    if((safe_flag_get(w->work) == 0 && w->worker) || safe_flag_get(w->update))
    {
        if(w->worker)
        {
            pthread_join(w->worker, NULL);
            memset(&w->worker, 0, sizeof(pthread_t));
            w->c_rate = w->u_rate;
        }

        int ret = webget_post_worker_update(w); //do the update of the parent and its children

        if(w->ww)
        {
            ret == 0 ? send_command(w->ip, w->success_act) : 0;
            sfree((void**)&w->ww->buf);
            sfree((void**)&w->ww);
        }
    }

    if(w->worker == 0 && w->address && (w->c_rate == 0 || safe_flag_get(w->update)))
    {

        int status = 0;
        safe_flag_set(w->work, 1);
        status = pthread_create(&w->worker, NULL, webget_worker_thread, w);

        if(status)
        {
            safe_flag_set(w->work, 0);
            diag_crit("%s %d Failed to start webget_worker_thread. Status %x", __FUNCTION__, __LINE__, status);
        }
        else
        {
            if(w->c_rate)
                w->c_rate--;
        }
    }

    return(0.0);
}

unsigned char *webget_string(void *spv)
{
    webget *w = spv;
    return(w->result == NULL ? w->err_str : w->result);
}


void webget_destroy(void **spv)
{
    webget *w = *spv;
    webget *wc = NULL;

    if(safe_flag_get(w->work))
    {
        safe_flag_set(w->stop, 1);
        pthread_join(w->worker, NULL);
    }

    if(w->ww)
    {
        sfree((void**)&w->ww->buf);
        sfree((void**)&w->ww->fp);

        if(w->ww->f)
            fclose(w->ww->f);

        sfree((void**)&w->ww);
    }

    pthread_mutex_destroy(&w->mutex);

    sfree((void**)&w->err_str);
    sfree((void**)&w->address);
    sfree((void**)&w->temp);
    sfree((void**)&w->result);
    sfree((void**)&w->regexp);
    sfree((void**)&w->success_act);
    sfree((void**)&w->parse_fail_act);
    sfree((void**)&w->dwl_fail);
    sfree((void**)&w->connect_fail);
    sfree((void**)&w->srf_dir);
    safe_flag_destroy(&w->work);
    safe_flag_destroy(&w->update);
    safe_flag_destroy(&w->stop);

    if(linked_list_empty(&w->current) == 0)
        linked_list_remove(&w->current);

    list_enum_part(wc, &w->children, current) //make the children orphan
    {
        wc->parent = NULL;
    }
    sfree(spv);
}


static FILE *webget_create_file(unsigned char *fp)
{
    FILE *fd = NULL;

    if(fp == NULL)
        return(NULL);

#ifdef WIN32
    unsigned short *uc = utf8_to_ucs(fp);

    if(uc)
        fd = _wfopen(uc, L"wb");

    sfree((void**)&uc);
#elif __linux__
    fd = fopen(fp, "wb");
#endif
    return(fd);
}



static int webget_rename_file(unsigned char *on, unsigned char *nn)
{
    if(on == NULL || nn == NULL)
        return(-1);

#ifdef WIN32
    unsigned short *ucon = utf8_to_ucs(on);
    unsigned short *ucnn = utf8_to_ucs(nn);

    if(ucon && ucnn)
        _wrename(ucon, ucnn);

    sfree((void**)&ucon);
    sfree((void**)&ucnn);
#elif __linux__
    rename(on, nn);
#endif
    return(0);
}

static int webget_remove_file(unsigned char *fp)
{
    if(fp == NULL)
        return(-1);

#ifdef WIN32
    unsigned short *uc = utf8_to_ucs(fp);

    if(uc)
        _wremove(uc);

    sfree((void**)&uc);
#elif __linux__
    remove(fp);
#endif
    return(0);
}

static int webget_mkdir(unsigned char *fp)
{
    int ret = -1;

    if(fp == NULL)
        return(-1);

#ifdef WIN32
    unsigned short *uc = utf8_to_ucs(fp);

    if(uc)
        ret = _wmkdir(uc);

    sfree((void**)&uc);
#elif __linux__
    ret = mkdir(fp, 640);
#endif
    return(ret);
}

static webget_encoding webget_check_text_encoding(unsigned char *buf, size_t len)
{
    if(buf == NULL || len < 2)
        return(enc_utf8);
    else if(buf[0] == 0xFF && buf[1] == 0xFE)
        return (enc_ucs_le);
    else if(buf[0] == 0xFE && buf[1] == 0xFF)
        return (enc_ucs_be);
    else
        return(enc_utf8);
}



static int webget_post_worker_update(webget *w)
{
    int ret = 0;
    webget *c = NULL;
    pthread_mutex_lock(&w->mutex);
    webget_worker *ww = w->ww;
    pthread_mutex_unlock(&w->mutex);
    unsigned char *buf = NULL;
    size_t buf_len = 0;
    int ovector[OVECTOR_LIMIT] = {0};
    int match_count = 0;
    size_t ovec_sz = OVECTOR_LIMIT;
    size_t len = 0;

    if(ww)
    {
        buf = ww->buf;
        buf_len = ww->buf_pos;
    }
    else
    {
        buf = w->temp;
        buf_len = string_length(buf);
    }

    if(buf && ww) //we have a buffer that came from the worker thread
    {
        sfree((void**)&w->result);

        if(webget_apply_regexp(buf, buf_len, w->regexp, (int*)ovector, ovec_sz, &match_count) == 0) //apply regexp to get the offsets
        {
            //update the return value for the parent
            if(w->pr_index < match_count && w->pr_index > 0 && ww && ww->fp == NULL)
            {
                len = ovector[w->pr_index * 2 + 1] - ovector[w->pr_index * 2];
                w->result = zmalloc(len + 1);
                strncpy(w->result, buf + ovector[w->pr_index * 2], len);
            }
            else if(ww && ww->fp)
            {
                w->result = ww->fp;
            }
        }
        else
        {
            ret = 1;
            send_command(w->ip, w->parse_fail_act);
        }
    }
    else if(ww && ww->fp && (ww->dwl_mode & DOWNLOAD_TO_FILE)) //no buffer? let's see if we do have a file downloaded for us
    {
        sfree((void**)&w->result);
        w->result = ww->fp;
    }

    //let's update the children (we might only destroy their info)
    list_enum_part(c, &w->children, current)
    {

        int ovector2[OVECTOR_LIMIT] = {0};
        size_t ovec_sz_2 = OVECTOR_LIMIT;
        int match_count_2 = 0;
        pthread_mutex_lock(&c->mutex);

        safe_flag_set(c->update, 1);
        sfree((void**)&c->temp);
        sfree((void**)&c->result);
        sfree((void**)&c->address);

        if(c->pr_index < match_count && c->pr_index > 0)
        {
            len = ovector[c->pr_index * 2 + 1] - ovector[c->pr_index * 2];
            c->temp = zmalloc(len + 1);
            strncpy(c->temp, buf + ovector[c->pr_index * 2], len);
        }

        if(c->sec_index == 0)
        {
            if(c->dwl)
            {
                safe_flag_set(c->update, 2);
                c->address = c->temp;
                c->temp = NULL;
            }
            else
                c->result = c->temp;
        }
        else if(webget_apply_regexp(c->temp, len, c->regexp, (int*)ovector2, ovec_sz_2, &match_count_2) == 0)
        {
            if(c->sec_index < match_count_2)
            {
                len = ovector2[c->sec_index * 2 + 1] - ovector2[c->sec_index * 2];
                c->result = zmalloc(len + 1);
                strncpy(c->result, c->temp + ovector2[c->sec_index * 2], len);
            }

            if(c->dwl)
            {
                safe_flag_set(c->update, 2);
                c->address = c->result;
                c->result = NULL;
            }

            if(linked_list_empty(&c->children))
            {
                sfree((void**)&c->temp);
            }
        }
        else
        {
            ret = 1;
            send_command(c->ip, c->parse_fail_act);
        }

        pthread_mutex_unlock(&c->mutex);
    }


    if(w->result != w->temp) //our result is not the temporary value so we can safely free it
    {
        sfree((void**)&w->temp);
    }
    else //it looks that the temporary value is actually our result. we will not free it but instead we will mark it as NULL
    {
        w->temp = NULL;
    }

    if(safe_flag_get(w->update))
        safe_flag_set(w->update,safe_flag_get(w->update)-1);

    return(ret);
}



static int webget_apply_regexp(unsigned char *buffer, size_t buf_sz, unsigned char *regexp, int *ovector, size_t ovec_sz, int *match_count)
{
    if(regexp == NULL || buffer == NULL || ovector == NULL || ovec_sz < 1)
    {
        return(-1);
    }

    int err_off = 0;
    const char* err = NULL;

    pcre* pc = pcre_compile((char*)regexp, 0, &err, &err_off, NULL);

    if(pc == NULL)
    {
        return(-2);
    }

    //int buf_sz=string_length(buffer);
    *match_count = pcre_exec(pc, NULL, (char*)buffer, buf_sz, 0, 0, ovector, ovec_sz / sizeof(int)); /*good explanation at http://libs.wikia.com/wiki/Pcre_exec */
    pcre_free(pc);

    if(*match_count < 1)
    {
        *match_count = 0;
        return (-3);
    }

    return(0);
}

static size_t webget_curl_worker_callback(char *buf, size_t size, size_t nmemb, void *pv)
{
    webget_worker *ww = pv;
    unsigned char *temp = NULL;
    size_t nb = size * nmemb;
    unsigned char err = 0;

//------------------------------
    if(safe_flag_get(ww->stop) == 1 || ww->dwl_mode == 0)
        return(0);

    if(ww->dwl_mode & DOWNLOAD_TO_BUFFER)
    {
        if(ww->buf_sz < ww->buf_pos + nb)
        {
            size_t comp = (ww->buf_pos + nb) - ww->buf_sz;
            temp = realloc(ww->buf, ww->buf_sz + (comp * nb) + 2);
            ww->buf_sz = ww->buf_sz + (comp * nb);
        }
        else
            temp = ww->buf;

        if(temp)
        {
            ww->buf = temp;
            memcpy(ww->buf + ww->buf_pos, buf, size * nmemb);
            ww->buf_pos += nb;
            ww->buf[ww->buf_pos] = 0;
            ww->buf[ww->buf_pos + 1] = 0;
        }
        else
            err = 1;
    }

    if(ww->dwl_mode & DOWNLOAD_TO_FILE && err == 0)
    {
        if(ww->f == NULL || fwrite(buf, size, nmemb, ww->f) != nmemb * size)
            err = 1;
    }

    if(err)
    {
        ww->f ? fclose(ww->f) : 0;
        ww->f = NULL;
        sfree((void**)&ww->buf);
        ww->buf_pos = 0;
        return(0);
    }

    return(nb);

}

static int webget_curl_download(unsigned char *link, webget_worker *ww)
{
    unsigned char dwl_mode_temp = ww->dwl_mode;
    int ret = 1;

    unsigned char fn[256] = {0};
    unsigned char *fp = NULL;
    CURL *c_state = curl_easy_init();

    if(c_state == NULL)
    {
        return(0);
    }

    ret = curl_easy_setopt(c_state, CURLOPT_URL, link);
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_WRITEFUNCTION, webget_curl_worker_callback) : ret;
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_WRITEDATA, ww) : ret;
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_FOLLOWLOCATION, 1) : ret;
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_MAXREDIRS, 4096) : 1;
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_SSL_VERIFYPEER, 0) : ret;

    /*Query download  size*/
    ww->dwl_mode = DOWNLOAD_TO_BUFFER;
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_HEADER, 1) : ret;
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_NOBODY, 1) : ret;
    ret = (ret == 0) ? curl_easy_perform(c_state) : ret;
    ww->dwl_mode = dwl_mode_temp;

    if(ww->dwl_mode & DOWNLOAD_TO_FILE && ret == 0)
    {
        if(ww->buf)
        {
            unsigned char *fn_beg = strstr(ww->buf, "filename=");
            unsigned char *fn_end = NULL;

            if(fn_beg)
            {
                fn_end = strstr(fn_beg, "\r\n");
                fn_beg += sizeof("filename=");
            }

            if(fn_beg && fn_end)
            {
                size_t len = min(fn_end - fn_beg - 1, 255);
                strncpy(fn, fn_beg, len);
            }
            else
            {
                struct timespec ts = {0};
                clock_gettime(CLOCK_REALTIME, &ts);
                snprintf(fn, 256, "%llu-%lu", ts.tv_sec, ts.tv_nsec);
            }

            sfree((void**)&ww->buf);

        }

        ww->buf_pos = 0;
        ww->buf_sz = 0;
        unsigned char *tmp_path = ww->save_root_dir;

        if(tmp_path)
        {
            size_t len = string_length(tmp_path) + sizeof("Semper_Webget") + 1;
            unsigned char *dir_path = zmalloc(len + 1);
            snprintf(dir_path, len, "%s/Semper_Webget", tmp_path);

            uniform_slashes(dir_path);
            webget_mkdir(dir_path);

            len = string_length(dir_path) + string_length(fn) + 6;
            fp = zmalloc(len);
            snprintf(fp, len, "%s/%s.tmp", dir_path, fn);

            uniform_slashes(fp);
            ww->f = webget_create_file(fp);
            sfree((void**)&dir_path);
        }
    }

    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_HEADER, 0) : ret;
    ret = (ret == 0) ? curl_easy_setopt(c_state, CURLOPT_NOBODY, 0) : ret;
    ret = (ret == 0) ? curl_easy_perform(c_state) : ret;
    curl_easy_cleanup(c_state);

    if(ret)
    {
        sfree((void**)&ww->buf);
        ww->buf_pos = 0;
        ww->f != NULL ? fclose(ww->f) : 0;
        webget_remove_file(fp);
        ww->f = NULL;
        sfree((void**)&fp);
    }
    else
    {
        ww->f ? fclose(ww->f) : 0;
        ww->f = NULL;
        webget_encoding enc = webget_check_text_encoding(ww->buf, ww->buf_pos);

        if(enc != enc_utf8)
        {
            ww->buf_pos = 0;
            unsigned char *temp = ucs_to_utf8((unsigned short*)(ww->buf + 2), &ww->buf_pos, enc == enc_ucs_be);
            sfree((void**)&ww->buf);

            if(temp)
                ww->buf = temp;
            else
                ww->buf_pos = 0;
        }

        if(fp)
        {
            size_t tmp_len = string_length(fp);

            if(tmp_len < 4)
                tmp_len = 4;

            unsigned char *true_name = zmalloc(tmp_len - 3);
            strncpy(true_name, fp, tmp_len - 4);
            webget_remove_file(ww->old_fp);
            webget_rename_file(fp, true_name);
            sfree((void**)&fp);
            ww->fp = true_name;
        }
    }

    return(ret);
}

static void *webget_worker_thread(void *p)
{
    int ret = 1;
    webget *w = p;
    webget_worker *ww = zmalloc(sizeof(webget_worker));
    unsigned char *address = NULL;
    safe_flag_set(w->work, 2);

    pthread_mutex_lock(&w->mutex);
    ww->stop = w->stop;
    ww->dwl_mode = (w->dwl == 0 && w->parent == NULL) ? DOWNLOAD_TO_BUFFER : w->dwl;
    ww->old_fp = clone_string(w->result); //assume that the result has the old filename

    if(ww->dwl_mode & DOWNLOAD_TO_FILE)
        ww->save_root_dir = (w->dwl_local ? clone_string(w->srf_dir) : expand_env_var("%temp%"));

    address = w->address;
    w->address = NULL;
    pthread_mutex_unlock(&w->mutex);


    if(safe_flag_get(w->stop) == 0 && address)
    {
        ret = webget_curl_download(address, ww);
    }


    pthread_mutex_lock(&w->mutex);

    if(w->address == NULL)
        w->address = address;
    else
        sfree((void**)&address);


    sfree((void**)&ww->save_root_dir);
    sfree((void**)&ww->old_fp);

    ww->status = ret;

    if(ret == 0)
        w->ww = ww;
    else
        sfree((void**)&ww);

    switch(ret)
    {
        case CURLE_COULDNT_CONNECT:
        case CURLE_UNSUPPORTED_PROTOCOL:
            send_command(w->ip, w->connect_fail);
            break;

        case CURLE_URL_MALFORMAT:
            send_command(w->ip, w->dwl_fail);
            break;

    }

    pthread_mutex_unlock(&w->mutex);
    safe_flag_set(w->work, 0);
    return(NULL);

}
