/*
 * Disgnostic messages revealer
 * Part of Project 'Semper'
 * Writen by Alexandru-Daniel Mărgărit
 */


#include <mem.h>
#include <semper_api.h>
#include <string_util.h>
#include <surface.h>
#include <diag.h>

typedef struct
{
    size_t max_msg;
    unsigned char order;
    unsigned char *ret_str;
    size_t log_row;
    size_t logs_avail;
    size_t cpos;
} diag_show;

extern diag_status *diag_get_struct(void);

void diag_show_init(void **spv, void *ip)
{
    unused_parameter(ip);
    *spv = zmalloc(sizeof(diag_show));
}

void diag_show_reset(void *spv, void *ip)
{
    diag_show *ds = spv;
    ds->max_msg = param_size_t("LogCount", ip, 1);
    ds->order = (unsigned char)param_size_t("ReverseOrder", ip, 1);
}

double diag_show_update(void *spv)
{

    diag_status *dss = diag_get_struct();
    diag_show *ds = spv;
    size_t buf_sz = (ds->max_msg - 1);
    size_t c = ds->max_msg;
    size_t buf_pos = 0;
    size_t entry_pos = ds->cpos;
    ds->logs_avail = dss->mem_log_elem;

    pthread_mutex_lock(&dss->mutex); /*prevent nasty stuff from happening*/
    sfree((void**)&ds->ret_str);
    diag_mem_log *dml = NULL;

    if(ds->order)
    {
        list_enum_part_backward(dml, &dss->mem_log, current)
        {
            if(entry_pos)
            {
                entry_pos--;
                continue;
            }

            buf_sz += string_length(dml->log_buf) + 1;

            if(--c == 0)
            {
                break;
            }
        }
    }
    else
    {
        list_enum_part(dml, &dss->mem_log, current)
        {
            if(entry_pos)
            {
                entry_pos--;
                continue;
            }

            buf_sz += string_length(dml->log_buf) + 1;

            if(--c == 0)
            {
                break;
            }
        }
    }

    ds->ret_str = zmalloc(buf_sz + 1);
    c = ds->max_msg;
    entry_pos = ds->cpos;

    if(ds->order)
    {
        list_enum_part_backward(dml, &dss->mem_log, current)
        {
            if(entry_pos)
            {
                entry_pos--;
                continue;
            }

            strcpy(ds->ret_str + buf_pos, dml->log_buf);
            buf_pos += string_length(dml->log_buf);

            if(--c == 0)
            {
                break;
            }

            strcpy(ds->ret_str + buf_pos, "\n");
            buf_pos++;

        }
    }
    else
    {
        list_enum_part(dml, &dss->mem_log, current)
        {
            if(entry_pos)
            {
                entry_pos--;
                continue;
            }

            strcpy(ds->ret_str + buf_pos, dml->log_buf);
            buf_pos += string_length(dml->log_buf);

            if(--c == 0)
            {
                break;
            }

            strcpy(ds->ret_str + buf_pos, "\n");
            buf_pos++;

        }
    }

    ds->ret_str[buf_pos] = 0;
    pthread_mutex_unlock(&dss->mutex);

    return((double)ds->max_msg - (double)c);
}

unsigned char *diag_show_string(void *spv)
{
    diag_show *ds = spv;
    return(ds->ret_str);
}

void diag_show_command(void *spv, unsigned char *comm)
{
    diag_show *ds = spv;

    if(comm)
    {
        if(!strcasecmp("Up", comm))
        {
            if(ds->cpos)
            {
                ds->cpos--;
            }
        }
        else if(!strcasecmp("Down", comm))
        {
            if(ds->logs_avail > ds->cpos + 1 && ds->logs_avail - ds->cpos > ds->max_msg)
            {
                ds->cpos++;
            }
        }
    }
}

void diag_show_destroy(void **spv)
{
    diag_show *ds = *spv;
    sfree((void**)&ds->ret_str);
    sfree(spv);
}
