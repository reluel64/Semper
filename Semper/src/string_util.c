/*
 * String handling routines
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Margarit
 */
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <semper.h>
#include <string_util.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <math_parser.h>
extern uint32_t uppercase(uint32_t point);
extern uint32_t lowercase(uint32_t point);


#ifndef WIN32
char *strlwr(char *s)
{
    if(s==NULL)
    {
        return(NULL);
    }
    for(size_t i=0; s[i]; i++)
    {
        s[i]=lowercase((uint32_t)s[i]);
    }
    return(s);
}

char *strupr(char *s)
{
    if(s==NULL)
    {
        return(NULL);
    }
    for(size_t i=0; s[i]; i++)
    {
        s[i]=uppercase((uint32_t)s[i]);
    }
    return(s);
}
#endif
unsigned char is_file_type(unsigned char* file, unsigned char* ext)
{
    size_t length = string_length(file);
    size_t elength = string_length(ext);
    unsigned char r = 0;
    for(size_t i = length - 1; i; i--)
    {
        if(file[i] == '.')
        {
            r = strncasecmp(file + i + 1, ext, min(length, elength)) == 0;
        }
    }
    return (r);
}

int remove_end_begin_quotes(unsigned char* s)
{
    size_t sz = string_length(s);
    if(sz < 2)
    {
        return (-1);
    }

    if(s[sz - 1] == '\"')
    {
        s[sz - 1] = '\0';
    }

    if(s[0] == '\"')
    {
        for(size_t i = 0; s[i]; i++)
        {
            s[i] = s[i + 1];
        }
    }

    return (0);
}

int remove_character(unsigned char* str, unsigned char ch)
{
    if(str == NULL)
    {
        return (-1);
    }
    for(size_t i = 0; str[i]; i++)
    {
        if(str[i] == ch)
        {
            for(size_t cpos = i; str[cpos]; cpos++)
            {
                str[cpos] = str[cpos + 1];
            }
            if(i != 0)
            {
                i--;
            }
            continue;
        }
    }
    return (0);
}

void remove_be_space(unsigned char* s)
{
    size_t len = string_length(s);
    for(size_t i = len - 1; i; i--)
    {
        if(s[i] == ' ')
        {
            s[i] = 0;
        }
        else
        {
            break;
        }
    }
    if(s[0] == ' ')
    {
        for(size_t i = 0; i < len; i++)
        {
            if(s[i] != ' ')
            {
                memcpy(s, s + i, len);
                break;
            }
        }
    }
}

unsigned char* string_lower(unsigned char* s)
{
    unsigned short* us = utf8_to_ucs(s);
    size_t slen = string_length(s);
    for(size_t i = 0; us[i]; i++)
    {
        us[i] = lowercase(us[i]);
    }

    memset(s, 0, slen);

    unsigned char* utf = ucs_to_utf8(us, NULL, 0);

    strcpy(s, utf);
    sfree((void**)&utf);
    sfree((void**)&us);
    return (s);
}

unsigned char* string_upper(unsigned char* s)
{
    unsigned short* us = utf8_to_ucs(s);
    size_t slen = string_length(s);
    for(size_t i = 0; us[i]; i++)
    {
        us[i] = uppercase(us[i]);
    }

    memset(s, 0, slen);

    unsigned char* utf = ucs_to_utf8(us, NULL, 0);

    strcpy(s, utf);
    sfree((void**)&utf);
    sfree((void**)&us);
    return (s);
}


int uniform_slashes(unsigned char *str)
{
    if(str == NULL)
    {
        return (-1);
    }
    for(size_t i = 0; str[i]; i++)
    {
        if((str[i] == '/'||str[i] == '\\')&&(str[i+1]=='\\'||str[i+1]=='/'))
        {
            for(size_t cpos = i; str[cpos]; cpos++)
            {
                str[cpos] = str[cpos + 1];
            }
            if(i != 0)
            {
                i--;
            }
            continue;
        }
    }
    return (0);
}

unsigned short* utf8_to_ucs(unsigned char* str)
{
    size_t len = string_length(str);
    size_t di = 0;
    if(len == 0)
    {
        return (NULL);
    }

    unsigned short* dest = zmalloc((len + 1) * 2);

    for(size_t si = 0; si < len; si++)
    {
        if(str[si] <= 0x7F)
        {
            dest[di] = str[si];
        }
        else if(str[si] >= 0xE0 && str[si] <= 0xEF)
        {
            dest[di] = (((str[si++]) & 0xF) << 12);
            dest[di] = dest[di] | (((unsigned short)(str[si++]) & 0x103F) << 6);
            dest[di] = dest[di] | (((unsigned short)(str[si]) & 0x103F));
        }
        else if(str[si] >= 0xc0 && str[si] <= 0xDF)
        {
            dest[di] = ((((unsigned short)str[si++]) & 0x1F) << 6);
            dest[di] = dest[di] | (((unsigned short)str[si]) & 0x103F);
        }
        di++;
    }
    return (dest);
}



unsigned char* ucs_to_utf8(unsigned short* s_in, size_t* bn, unsigned char be)
{
    size_t nb = 0;
    size_t di = 0;
    unsigned char* out = NULL;

    if(s_in==NULL)
    {
        return(NULL);
    }
    /*Query the needed bytes to be allocated*/
    for(size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if(be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if(ch < 0x80)
        {
            nb++;
        }
        else if(ch >= 0x80 && ch < 0x800)
        {
            nb += 2;
        }
        else if(ch >= 0x800 && ch < 0xFFFF)
        {
            nb += 3;
        }
        else if(ch >= 0x10000 && ch < 0x10FFFF)
        {
            nb += 4;
        }
    }
    if(nb == 0)
    {
        return (NULL);
    }
    out = zmalloc(nb + 1);
    /*Let's encode Unicode to UTF-8*/

    for(size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if(be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if(ch < 0x80)
        {
            out[di++] = (unsigned char)s_in[i];
        }
        else if((size_t)ch >= 0x80 && (size_t)ch < 0x800)
        {
            out[di++] = (s_in[i] >> 6) | 0xC0;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if(ch >= 0x800 && ch < 0xFFFF)
        {
            out[di++] = (s_in[i] >> 12) | 0xE0;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if(ch >= 0x10000 && ch < 0x10FFFF)
        {
            out[di++] = ((unsigned long)s_in[i] >> 18) | 0xF0;
            out[di++] = ((s_in[i] >> 12) & 0x3F) | 0x80;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
    }

    if(bn)
    {
        *bn = nb;
    }

    return (out);
}



unsigned char* ucs32_to_utf8(unsigned int* s_in, size_t* bn, unsigned char be)
{
    size_t nb = 0;
    size_t di = 0;
    unsigned char* out = NULL;

    if(s_in==NULL)
    {
        return(NULL);
    }

    /*Query the needed bytes to be allocated*/
    for(size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if(be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if(ch < 0x80)
        {
            nb++;
        }
        else if(ch >= 0x80 && ch < 0x800)
        {
            nb += 2;
        }
        else if(ch >= 0x800 && ch < 0xFFFF)
        {
            nb += 3;
        }
        else if(ch >= 0x10000 && ch < 0x10FFFF)
        {
            nb += 4;
        }
    }

    if(nb == 0)
    {
        return (NULL);
    }

    out = zmalloc(nb + 1);

    /*Let's encode Unicode to UTF-8*/
    for(size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if(be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if(ch < 0x80)
        {
            out[di++] = (unsigned char)s_in[i];
        }
        else if((size_t)ch >= 0x80 && (size_t)ch < 0x800)
        {
            out[di++] = (s_in[i] >> 6) | 0xC0;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if(ch >= 0x800 && ch < 0xFFFF)
        {
            out[di++] = (s_in[i] >> 12) | 0xE0;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if(ch >= 0x10000 && ch < 0x10FFFF)
        {
            out[di++] = ((unsigned long)s_in[i] >> 18) | 0xF0;
            out[di++] = ((s_in[i] >> 12) & 0x3F) | 0x80;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
    }
    if(bn)
    {
        *bn = nb;
    }
    return (out);
}

unsigned char* expand_env_var(unsigned char* path)
{
    size_t mem_n = 0;
    unsigned char* ret = NULL;
#ifdef WIN32
    wchar_t* unip = utf8_to_ucs(path);

    mem_n = ExpandEnvironmentStringsW(unip, NULL, 0);
    wchar_t* expanded_var = zmalloc((mem_n + 1) * 2);
    ExpandEnvironmentStringsW(unip, expanded_var, (unsigned long)mem_n + 1);
    ret = ucs_to_utf8(expanded_var, NULL, 0);
    sfree((void**)&expanded_var);
    sfree((void**)&unip);
#elif __linux__
    ret = clone_string(path);
#endif
    return (ret);
}

unsigned char* clone_string(unsigned char* s)
{
    size_t sl = string_length(s);
    unsigned char* b = NULL;
    if(!sl)
    {
        return (NULL);
    }
    b = malloc(sl + 1);

    if(b)
    {
        b[sl]=0;
        strcpy(b, s);
    }

    return (b);
}

unsigned int string_to_color(unsigned char* str)
{
    if(str == NULL)
    {
        return (0x00000000);
    }
    unsigned int color = 0xff000000;
    remove_character(str, ' ');
    strlwr(str);
    if(sscanf(str, "0x%x", &color) == 0)
    {
        unsigned int alpha = 255;
        unsigned int red = 0;
        unsigned int blue = 0;
        unsigned int green = 0;

        if(sscanf(str, "%u", &color) <= sscanf(str, "%u;%u;%u;%u", &red, &green, &blue, &alpha))
        {
            color = blue | green << 8 | red << 16 | alpha << 24;
        }
    }
    return (color);
}

double compute_formula(unsigned char* formula)
{
    if(formula == NULL)
    {
        return (0.0);
    }
    double n = 0.0;
    if(math_parser(formula,&n,NULL,NULL)==0)
        return(n);
    else
        return(0.0);
}

int string_to_image_crop(unsigned char* str, image_crop* inf)
{
    if(str == NULL || inf == NULL)
    {
        return (-1);
    }
    memset(inf, 0, sizeof(image_crop));
    remove_character(str, ' ');
    sscanf(str, "%ld;%ld;%ld;%ld;%hhu", &inf->x, &inf->y, &inf->w, &inf->h, &inf->origin);

    return (0);
}

int string_to_image_tile(unsigned char* str, image_tile* inf)
{
    if(str == NULL || inf == NULL)
    {
        return (-1);
    }
    memset(inf, 0, sizeof(image_tile));
    remove_character(str, ' ');
    sscanf(str, "%ld;%ld", &inf->w, &inf->h);
    return (0);
}

int string_to_padding(unsigned char* str, object_padding* op)
{
    if(str == NULL || op == NULL)
    {
        return (-1);
    }
    memset(op, 0, sizeof(object_padding));
    remove_character(str, ' ');
    sscanf(str, "%ld;%ld;%ld;%ld", &op->left, &op->top, &op->right, &op->bottom);

    /*Make sure that the padding is strictly positive*/
    op->left   = op->left   > 0 ? op->left   :0;
    op->right  = op->right  > 0 ? op->right  :0;
    op->top    = op->top    > 0 ? op->top    :0;
    op->bottom = op->bottom > 0 ? op->bottom :0;

    return (0);
}

int string_to_color_matrix(unsigned char* str, double* cm)
{
    if(str == NULL || cm == NULL)
    {
        return (-1);
    }
    memset(cm, 0, sizeof(double) * 5);
    remove_character(str, ' ');
    sscanf(str, "%lf;%lf;%lf;%lf;%lf", cm, cm + 1, cm + 2, cm + 3, cm + 4);
    return (0);
}

int unix_slashes(unsigned char* s)
{
    if(s == NULL)
    {
        return (-1);
    }
    for(size_t i = 0; s[i]; i++)
    {
        if(s[i] == '\\')
        {
            s[i] = '/';
        }
    }
    return (0);
}


int windows_slahses(unsigned char* s)
{
    if(s == NULL)
    {
        return (-1);
    }

    for(size_t i = 0; s[i]; i++)
    {
        if(s[i] == '/')
        {
            s[i] = '\\';
        }
    }

    return (0);
}

typedef struct
{
    size_t op;
    size_t quotes;
} command_handler_status;



int string_tokenizer(string_tokenizer_info *sti)
{
    string_tokenizer_status sts=
    {
        .pos=0,
        .buf=sti->buffer
    };

    if(sti->ovecoff==NULL)
    {
        sti->oveclen=0;
    }

    if(sti==NULL||sti->buffer==NULL||sti->oveclen%2)
    {
        return(-1);
    }

    size_t buf_len = string_length(sti->buffer);

    for(unsigned char step=sti->oveclen>0?1:0; step<2; step++)
    {
        size_t buf_pos = 0;
        size_t offsets=0;
        size_t lpos=0;
        while(buf_pos < buf_len)
        {

            while(buf_pos < buf_len && isspace(sti->buffer[buf_pos]))
            {
                buf_pos++;
            }

            while(buf_pos<buf_len)
            {
                sts.pos=buf_pos;

                if(sti->string_tokenizer_filter&&sti->string_tokenizer_filter(&sts,sti->filter_data))
                {
                    if(step==0)
                    {
                        offsets+=2;
                    }
                    else if(sti->oveclen>=offsets+2)
                    {
                        sti->ovecoff[offsets++]=lpos;
                        sti->ovecoff[offsets++]=buf_pos;
                        sti->ovecoff[offsets]=buf_pos;
                    }
                    lpos=buf_pos++;
                    break;
                }
                buf_pos++;
            }
        }
        if(step==0)
        {
            if(sti->ovecoff==NULL)
            {
                offsets+=2;
                sti->ovecoff=zmalloc(offsets*sizeof(size_t));
                sti->oveclen=offsets;
                sts.reset=1;
                if(sti->string_tokenizer_filter)
                {
                    sti->string_tokenizer_filter(&sts,sti->filter_data);
                }
                sts.reset=0;
            }
        }
        else if(sti->oveclen>=offsets+2)
        {
            sti->ovecoff[offsets++]=lpos;
            sti->ovecoff[offsets++]=buf_pos;
            sti->oveclen=offsets;
        }
    }
    return(0);
}

int string_strip_space_offsets(unsigned char *buf,size_t *start,size_t *end)
{
    while((*start)<(*end))
    {
        unsigned char found=0;
        if(isspace(buf[(*start)]))
        {
            (*start)++;
            found=1;
        }
        if(isspace(buf[(*end)-1]))
        {
            (*end)--;
            found=1;
        }
        if(found==0)
        {
            break;
        }
    }
    if((*start)>(*end))
    {
        *start=*end;
        return(-1);
    }
    return(0);
}

int remove_trailing_zeros(unsigned char* s)
{
    int dot = 0;
    for(size_t i = 0; s[i]; i++)
    {
        if(s[i] == '.')
        {
            dot++;
            break;
        }
    }
    if(dot)
    {
        size_t sl = string_length(s);

        for(size_t i = sl - 1; dot && s[i] && i; i--)
        {
            if(s[i] == '0' && s[i] != '.')
            {
                s[i] = '\0';
            }
            else if(s[i] == '.' && s[i + 1] == '\0')
            {
                s[i] = '\0';
                dot = 0;
            }
            else if(s[i] != '0')
            {
                break;
            }
        }
    }
    return (0);
}
