#pragma once
#include <stddef.h>
#ifndef unused_parameter
#define unused_parameter(p) ((p)=(p))
#endif

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

#define EXTENSION_XPAND_SOURCES   0x2
#define EXTENSION_XPAND_VARIABLES 0x1
#define EXTENSION_XPAND_ALL       0x3
#define EXTENSION_PATH_SEMPER     0x1
#define EXTENSION_PATH_SURFACES   0x2
#define EXTENSION_PATH_EXTENSIONS 0x3
#define EXTENSION_PATH_SURFACE    0x4



void*          get_surface(void* ip);
void*          get_extension_by_name(unsigned char* name, void* ip);
void*          get_private_data(void* ip);
void*          get_parent(unsigned char* str, void* ip);
void           tokenize_string_free(tokenize_string_info *tsi);
void           send_command(void* ir, unsigned char* command);
double         param_double(unsigned char* pn, void* ip, double def);
size_t         param_size_t(unsigned char* pn, void* ip, size_t def);

int            has_parent(unsigned char* str);
int            tokenize_string(tokenize_string_info *tsi);
int            source_set_min(double val, void* ip, unsigned char force, unsigned char hold);
int            source_set_max(double val, void* ip, unsigned char force, unsigned char hold);
int            is_parent_candidate(void* pc, void* ip);
unsigned char *get_path(void *ip,unsigned char pth);
unsigned char *absolute_path(void *ip,unsigned char *rp,unsigned char pth);
unsigned char *param_string(unsigned char* pn, unsigned char flags, void* ip, unsigned char* def);
unsigned char *get_extension_name(void* ip);
unsigned char  param_bool(unsigned char* pn, void* ip, unsigned char def);



