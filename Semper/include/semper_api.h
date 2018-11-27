#pragma once
#include <stddef.h>
#include <sys/types.h>
#ifndef unused_parameter
#define unused_parameter(p) ((p)=(p))
#endif

#define EXTENSION_XPAND_SOURCES   0x2
#define EXTENSION_XPAND_VARIABLES 0x1
#define EXTENSION_XPAND_ALL       0x3
#define EXTENSION_PATH_SEMPER     0x1
#define EXTENSION_PATH_SURFACES   0x2
#define EXTENSION_PATH_EXTENSIONS 0x3
#define EXTENSION_PATH_SURFACE    0x4

typedef struct
{
    size_t pos;                                                                 //current position in buffer returned by the string_tokenizer()
    unsigned char *buf;                                                         //starting address of the buffer - in case you haven't saved it :-)
    unsigned char reset;                                                        //this will signal the filtering function to reset its state (if it needs to)
} tokenize_string_status;

typedef struct
{
    unsigned char *buffer;                                                                //the buffer that has to be tokenized (input)
    size_t oveclen;                                                                       //offset vector length (in_out)
    size_t *ovecoff;                                                                      //offset vector (in_out)
    int (*tokenize_string_filter)(tokenize_string_status *sts, void* pv);     //filtering function (mandatory)
    void *filter_data;                                                                    //user data for the filtering function
} tokenize_string_info;


typedef int (*event_handler)(void*);
typedef void* (semper_thrd_start_t) (void*);
typedef void* semper_thrd_t;
typedef void* semper_mtx_t;
typedef void* semper_cnd_t;

void*          semper_safe_flag_init(void);
void*          get_surface(void* ip);
void*          get_extension_by_name(unsigned char* name, void* ip);
void*          get_private_data(void* ip);
void*          get_parent(unsigned char* str, void* ip);
void           tokenize_string_free(tokenize_string_info *tsi);
void           send_command(void* ir, unsigned char* command);
void           send_command_ex(void* ir, unsigned char* cmd, size_t timeout, char unique);
void           semper_safe_flag_set(void *sf, size_t flag);
void           semper_safe_flag_destroy(void **psf);
void           semper_free(void **p);
double         param_double(unsigned char* pn, void* ip, double def);
size_t         param_size_t(unsigned char* pn, void* ip, size_t def);
size_t         semper_safe_flag_get(void *sf);

int            has_parent(unsigned char* str);
int            tokenize_string(tokenize_string_info *tsi);
int            source_set_min(double val, void* ip, unsigned char force, unsigned char hold);
int            source_set_max(double val, void* ip, unsigned char force, unsigned char hold);
int            is_parent_candidate(void* pc, void* ip);
int            diag_log(unsigned char lvl, char *fmt, ...);
int            semper_event_remove(void *ip, event_handler eh, void* pv, unsigned char flags);
int            semper_event_push(void *ip, event_handler handler, void* pv, size_t timeout, unsigned char flags);
unsigned char *get_path(void *ip, unsigned char pth);
unsigned char *absolute_path(void *ip, unsigned char *rp, unsigned char pth);
unsigned char *param_string(unsigned char* pn, unsigned char flags, void* ip, unsigned char* def);
unsigned char *get_extension_name(void* ip);
unsigned char  param_bool(unsigned char* pn, void* ip, unsigned char def);
unsigned short *semper_utf8_to_ucs(unsigned char *s_in);
unsigned char *semper_ucs_to_utf8(unsigned short *s_in,size_t *len,unsigned char be);


int semper_thrd_create(semper_thrd_t *t, semper_thrd_start_t func, void *arg);
int semper_thrd_equal(semper_thrd_t t1, semper_thrd_t t2);
semper_thrd_t semper_thrd_current(void);
void semper_thrd_exit(void *res);
int semper_thrd_detach(semper_thrd_t t);
int semper_thrd_join(semper_thrd_t t, void **res);
int semper_mtx_init(semper_mtx_t *mtx,int type);
int semper_mtx_lock(semper_mtx_t *mtx);
int semper_mtx_unlock(semper_mtx_t *mtx);
void semper_mtx_destroy(semper_mtx_t *mtx);
int semper_cnd_init(semper_cnd_t *cnd);
int semper_cnd_signal(semper_cnd_t *cnd);
int semper_cnd_broadcast(semper_cnd_t *cnd);
int semper_cnd_wait(semper_cnd_t *cnd, semper_mtx_t *mtx);
int semper_cnd_timedwait(semper_cnd_t *cnd, semper_mtx_t *mtx, struct timespec *tm);
void semper_cnd_destroy(semper_cnd_t *cnd);




#ifndef diag_info
#define diag_info(x...)  diag_log(0x1,"[INFO] "x)
#endif

#ifndef diag_warn
#define diag_warn(x...)  diag_log(0x2,"[WARN] "x)
#endif

#ifndef diag_error
#define diag_error(x...) diag_log(0x4,"[ERROR] "x)
#endif

#ifndef diag_crit
#define diag_crit(x...)  diag_log(0x8,"[CRIT] "x)
#endif

#ifndef diag_verb
#define diag_verb(x...)  diag_log(0x10,"[VERB] "x)
#endif
