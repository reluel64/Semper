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
} extension_string_tokenizer_status;

typedef struct
{
    unsigned char *buffer;                                                                //the buffer that has to be tokenized (input)
    size_t oveclen;                                                                       //offset vector length (in_out)
    size_t *ovecoff;                                                                      //offset vector (in_out)
    int (*string_tokenizer_filter)(extension_string_tokenizer_status *sts, void* pv);     //filtering function (mandatory)
    void *filter_data;                                                                    //user data for the filtering function
} extension_string_tokenizer_info;

#define EXTENSION_XPAND_SOURCES   0x2
#define EXTENSION_XPAND_VARIABLES 0x1
#define EXTENSION_XPAND_ALL       0x3
#define EXTENSION_PATH_SEMPER     0x1
#define EXTENSION_PATH_SURFACES   0x2
#define EXTENSION_PATH_EXTENSIONS 0x3
#define EXTENSION_PATH_SURFACE    0x4



void*          extension_surface(void* ip);
void*          extension_by_name(unsigned char* name, void* ip);
void*          extension_private(void* ip);
void*          extension_get_parent(unsigned char* str, void* ip);
void           extension_tokenize_string_free(extension_string_tokenizer_info *esti);
void           extension_send_command(void* ir, unsigned char* command);
double         extension_double(unsigned char* pn, void* ip, double def);
size_t         extension_size_t(unsigned char* pn, void* ip, size_t def);

int            extension_has_parent(unsigned char* str);
int            extension_tokenize_string(extension_string_tokenizer_info *esti);
int            extension_set_min(double val, void* ip, unsigned char force, unsigned char hold);
int            extension_set_max(double val, void* ip, unsigned char force, unsigned char hold);
int            extension_parent_candidate(void* pc, void* ip);

unsigned char *extension_get_path(void *ip,unsigned char pth);
unsigned char *extension_absolute_path(void *ip,unsigned char *rp,unsigned char pth);
unsigned char *extension_string(unsigned char* pn, unsigned char raw, void* ip, unsigned char* def);
unsigned char *extension_name(void* ip);
unsigned char  extension_bool(unsigned char* pn, void* ip, unsigned char def);
