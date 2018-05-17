/* Time source
 *Part of Project 'Semper'
 *Written by Alexandru-Daniel Mărgărit
 */

#include <semper_api.h>
#include <string_util.h>
#include <mem.h>
#include <sources/time_s.h>
#include <time.h>

typedef struct
{
    unsigned char *format;
    unsigned char buf[256]; /*this should be enough*/
    time_t _tm;
} time_state;

void time_init(void **spv, void *ip)
{
    unused_parameter(ip);
    *spv = zmalloc(sizeof(time_state));
}

void time_reset(void *spv, void *ip)
{
    time_state *ts = spv;
    sfree((void**)&ts->format);
    ts->format = clone_string(param_string("Format", 0x3, ip, NULL));
}

double time_update(void *spv)
{
    time_state *ts = spv;
    ts->_tm = time(NULL);
    return((double)ts->_tm);
}


unsigned char *time_string(void *spv)
{
    time_state *ts = spv;
    struct tm res = {0};
    struct tm *fmt_time = localtime_r(&ts->_tm, &res);
    strftime(ts->buf, 256, ts->format, fmt_time);
    return(ts->buf);
}

void time_destroy(void **spv)
{
    time_state *ts = *spv;
    sfree((void**)&ts->format);
    sfree(spv);
}
