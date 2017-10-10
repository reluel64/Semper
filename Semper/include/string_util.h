#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <image/image_cache.h>
#include <objects/object.h>
#include <string.h>
#define string_length(s) (((s) == NULL ? 0 : strlen((s))))
/**********************************/

typedef struct
{
    size_t pos;                                                                 //current position in buffer returned by the string_tokenizer()
    unsigned char *buf;                                                         //starting address of the buffer - in case you haven't saved it :-)
    unsigned char reset;                                                        //this will signal the filtering function to reset its state (if it needs to)
} string_tokenizer_status;

typedef struct
{
    unsigned char *buffer;                                                      //the buffer that has to be tokenized (input)
    size_t oveclen;                                                             //offset vector length (in_out)
    size_t *ovecoff;                                                            //offset vector (in_out)
    int (*string_tokenizer_filter)(string_tokenizer_status *sts, void* pv);     //filtering function (mandatory)
    void *filter_data;                                                          //user data for the filtering function
    size_t buf_len;
} string_tokenizer_info;


void remove_be_space(unsigned char* s);
int string_strip_space_offsets(unsigned char *buf,size_t *start,size_t *end);
double compute_formula(unsigned char* formula);
int remove_end_begin_quotes(unsigned char* s);
int remove_character(unsigned char* str, unsigned char ch);
int string_to_image_crop(unsigned char* str, image_crop* inf);
int string_to_image_tile(unsigned char* str, image_tile* inf);
int string_to_padding(unsigned char* str, object_padding* op);
int string_to_color_matrix(unsigned char* str, double* cm);
int windows_slahses(unsigned char* s);
int uniform_slashes(unsigned char *str);
int unix_slashes(unsigned char* s);
int remove_trailing_zeros(unsigned char* s);
int string_tokenizer(string_tokenizer_info *sti);
unsigned int string_to_color(unsigned char* str);
unsigned char* ucs_to_utf8(unsigned short* s_in, size_t* bn, unsigned char be);
unsigned char* clone_string(unsigned char* s);
unsigned char* expand_env_var(unsigned char* path);
unsigned char* ucs32_to_utf8(unsigned int* s_in, size_t* bn, unsigned char be);
unsigned char* string_upper(unsigned char* s);
unsigned char* string_lower(unsigned char* s);
unsigned char is_file_type(unsigned char* file, unsigned char* ext);
unsigned char* replace(unsigned char* in, unsigned char* rep_pair, unsigned char regexp);
unsigned short* utf8_to_ucs(unsigned char* str);
size_t utf8_len(unsigned char *str,size_t b_len);
#ifndef WIN32
char *strlwr(char *s);
char *strupr(char *s);
#endif
