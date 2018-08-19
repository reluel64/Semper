/*
 * String handling routines
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Margarit
 */
#define PCRE_STATIC
#include <pcre.h>
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

typedef struct
{
    unsigned char *ps;  // pair string
    unsigned char *in;  // input string
    //To-be-replaced
    size_t tbr_start;
    size_t tbr_end;
    //To-replace-with
    size_t rl_start;
    size_t rl_end;

} replace_state;

typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} replacer_tokenizer_status;


#ifndef WIN32

char *strlwr(char *s)
{
    if(s == NULL)
    {
        return(NULL);
    }

    for(size_t i = 0; s[i]; i++)
    {
        s[i] = lowercase((uint32_t)s[i]);
    }

    return(s);
}

char *strupr(char *s)
{
    if(s == NULL)
    {
        return(NULL);
    }

    for(size_t i = 0; s[i]; i++)
    {
        s[i] = uppercase((uint32_t)s[i]);
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
            break;
        }
    }

    return (r);
}

size_t  remove_end_begin_quotes(unsigned char* s)
{
    size_t sz = string_length(s);

    if(sz < 2)
    {
        return (0);
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

    return (sz - 1);
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
    size_t bn = 0;

    for(size_t i = 0; us[i]; i++)
    {
        us[i] = lowercase(us[i]);
    }

    memset(s, 0, slen);

    unsigned char* utf = ucs_to_utf8(us, &bn, 0);

    strncpy(s, utf, min(slen, bn));
    sfree((void**)&utf);
    sfree((void**)&us);
    return (s);
}

unsigned char* string_upper(unsigned char* s)
{
    unsigned short* us = utf8_to_ucs(s);
    size_t slen = string_length(s);
    size_t bn = 0;

    for(size_t i = 0; us[i]; i++)
    {
        us[i] = uppercase(us[i]);
    }

    memset(s, 0, slen);

    unsigned char* utf = ucs_to_utf8(us, &bn, 0);

    strncpy(s, utf, min(slen, bn));
    sfree((void**)&utf);
    sfree((void**)&us);
    return (s);
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

size_t utf8_len(unsigned char *str, size_t b_len)
{
    size_t ret = 0;

    if(str == NULL)
        return(0);

    if(b_len == 0)
    {
        b_len = string_length(str);
    }

    for(size_t si = 0; si < b_len; si++)
    {
        if(str[si] <= 0x7F)
        {
            ret++;
        }
        else if(str[si] >= 0xE0 && str[si] <= 0xEF)
        {
            si += 2;
            ret++;
        }
        else if(str[si] >= 0xc0 && str[si] <= 0xDF)
        {
            si++;
            ret++;
        }
    }

    return(ret);
}


unsigned char* ucs_to_utf8(unsigned short* s_in, size_t* bn, unsigned char be)
{
    size_t nb = 0;
    size_t di = 0;
    unsigned char* out = NULL;

    if(s_in == NULL)
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

    if(s_in == NULL)
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


#ifdef __linux__
static int variable_tokenizer(string_tokenizer_status *sti, int  *fl)
{
    if(sti->reset)
    {
        *fl = 0;
        return(0);
    }

    if(sti->buf[sti->pos] == '$')
    {
        *fl = 1;
        return(1);
    }

    if(sti->buf[sti->pos] == '/' && *fl == 1)
    {
        *fl = 0;
        return(1);
    }


    return(0);
}

static char *string_util_xpand_linux_env(unsigned char *str)
{
    if(str == NULL)
    {
        return(NULL);
    }


    unsigned char *res = str;
    size_t len = 0;

    for(unsigned char stage = 0; stage < 255; stage++)
    {
        char more = 0;
        int p = 0;
        string_tokenizer_info sti = {0};
        sti.buffer = res;
        sti.string_tokenizer_filter = variable_tokenizer;
        sti.filter_data = &p;
        string_tokenizer(&sti);
        res = NULL;

        for(size_t i = 0; i < sti.oveclen / 2; i++)
        {
            char bbb[256] = {0};
            size_t start = sti.ovecoff[i * 2];
            size_t end = sti.ovecoff[i * 2 + 1];

            if(end == start)
                continue;

            strncpy(bbb, sti.buffer + start, end - start);


            if(sti.buffer[start] == '$')
            {
                unsigned char *vname = zmalloc((end - start) + 1);
                unsigned char *vval = NULL;
                size_t vlen = end - start;
                strncpy(vname, sti.buffer + start + 1, end - (start + 1));
                vval = getenv(vname);

                if(vval)
                {
                    vlen = string_length(vval);

                    if(vval[0] == '$')
                        more = 1;
                }
                else
                {
                    vval = sti.buffer + start;
                }

                unsigned char *tmp = realloc(res, vlen + len + 1);

                if(tmp)
                {
                    res = tmp;
                    strncpy(res + len, vval, vlen);
                    len += vlen;
                    res[len] = 0;
                }


                fflush(NULL);
                sfree((void**)&vname);
            }
            else
            {
                size_t vlen = end - start;
                unsigned char *tmp = realloc(res, vlen + len + 1);

                if(tmp)
                {
                    res = tmp;
                    strncpy(res + len, sti.buffer + start, vlen);
                    len += vlen;
                    res[len] = 0;
                }
            }
        }


        if(sti.buffer != str)
        {
            sfree((void**)&sti.buffer);
        }

        sfree((void**)&sti.ovecoff);

        if(more == 0)
            break;


    }

    return(res);
}
#endif


unsigned char* expand_env_var(unsigned char* path)
{

    unsigned char* ret = NULL;
#ifdef WIN32
    size_t mem_n = 0;
    wchar_t* unip = utf8_to_ucs(path);

    mem_n = ExpandEnvironmentStringsW(unip, NULL, 0);
    wchar_t* expanded_var = zmalloc((mem_n + 1) * 2);
    ExpandEnvironmentStringsW(unip, expanded_var, (unsigned long)mem_n + 1);
    ret = ucs_to_utf8(expanded_var, NULL, 0);
    sfree((void**)&expanded_var);
    sfree((void**)&unip);
#elif __linux__
    ret = string_util_xpand_linux_env(path);
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
        b[sl] = 0;
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

    if(math_parser(formula, &n, NULL, NULL) == 0)
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
    op->left   = op->left   > 0 ? op->left   : 0;
    op->right  = op->right  > 0 ? op->right  : 0;
    op->top    = op->top    > 0 ? op->top    : 0;
    op->bottom = op->bottom > 0 ? op->bottom : 0;

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


int uniform_slashes(unsigned char *str)
{
    if(str == NULL)
    {
        return (-1);
    }

    for(size_t i = 0; str[i]; i++)
    {
#ifdef WIN32

        if(str[i] == '/')
        {
            str[i] = '\\';
        }

#elif __linux__

        if(str[i] == '\\')
        {
            str[i] = '/';
        }

#endif

        if((str[i] == '/' || str[i] == '\\') && (str[i + 1] == '\\' || str[i + 1] == '/'))
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



int string_tokenizer(string_tokenizer_info *sti)
{
    string_tokenizer_status sts =
    {
        .pos = 0,
        .buf = sti->buffer
    };

    if(sti->ovecoff == NULL)
    {
        sti->oveclen = 0;
    }

    if(sti == NULL || sti->buffer == NULL || sti->oveclen % 2)
    {
        return(-1);
    }

    size_t buf_len = 0;


    buf_len = string_length(sti->buffer);

    for(unsigned char step = sti->oveclen > 0 ? 1 : 0; step < 2; step++)
    {
        size_t buf_pos = 0;
        size_t offsets = 0;
        size_t lpos = 0;

        while(buf_pos < buf_len)
        {

            while(buf_pos < buf_len && isspace(sti->buffer[buf_pos]))
            {
                buf_pos++;
            }

            while(buf_pos < buf_len)
            {
                sts.pos = buf_pos;

                if(sti->string_tokenizer_filter && sti->string_tokenizer_filter(&sts, sti->filter_data))
                {
                    if(step == 0)
                    {
                        offsets += 2;
                    }
                    else if(sti->oveclen >= offsets + 2)
                    {
                        sti->ovecoff[offsets++] = lpos;
                        sti->ovecoff[offsets++] = buf_pos;
                        sti->ovecoff[offsets] = buf_pos;
                    }

                    lpos = buf_pos++;
                    break;
                }

                buf_pos++;
            }
        }

        if(step == 0)
        {
            if(sti->ovecoff == NULL)
            {
                offsets += 2;
                sti->ovecoff = zmalloc(offsets * sizeof(size_t));
                sti->oveclen = offsets;
                sts.reset = 1;

                if(sti->string_tokenizer_filter)
                {
                    sti->string_tokenizer_filter(&sts, sti->filter_data);
                }

                sts.reset = 0;
            }
        }
        else if(sti->oveclen >= offsets + 2)
        {
            sti->ovecoff[offsets++] = lpos;
            sti->ovecoff[offsets++] = buf_pos;
            sti->oveclen = offsets;
        }
    }

    return(0);
}

int string_strip_space_offsets(unsigned char *buf, size_t *start, size_t *end)
{
    while((*start) < (*end))
    {
        unsigned char found = 0;

        if(isspace(buf[(*start)]))
        {
            (*start)++;
            found = 1;
        }

        if(isspace(buf[(*end) - 1]))
        {
            (*end)--;
            found = 1;
        }

        if(found == 0)
        {
            break;
        }
    }

    if((*start) > (*end))
    {
        *start = *end;
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
            if(s[i] == '0')
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


static unsigned char* string_util_do_replacements_plain(replace_state* rs)
{
    size_t nm = 0;
    size_t di = 0;
    size_t rl_len = rs->rl_end - rs->rl_start;
    size_t tbr_len = rs->tbr_end - rs->tbr_start;

    unsigned char* ret = NULL;

    if(tbr_len == 0)
    {
        return(NULL);
    }

    for(size_t i = 0; rs->in[i]; i++)
    {
        if(tbr_len && strncmp(rs->in + i, rs->ps + rs->tbr_start, tbr_len) == 0)
        {
            nm += rl_len;
            i += tbr_len - 1;
        }
        else
        {
            nm++;
        }
    }

    ret = zmalloc(nm + 1);

    for(size_t i = 0; rs->in[i]; i++)
    {
        if(tbr_len && strncmp(rs->in + i, rs->ps + rs->tbr_start, tbr_len) == 0)
        {
            strncpy(ret + di, rs->ps + rs->rl_start, rl_len);
            di += rl_len;
            i += tbr_len - 1;
        }
        else
        {
            ret[di++] = rs->in[i];
        }
    }

    return (ret);
}

static unsigned char* string_util_do_replacements_pcre(replace_state* rs)
{

    if(!rs || !rs->in || (rs->tbr_end - rs->tbr_start) == 0)
    {
        return (NULL);
    }

    size_t new_length = 0;          // total length of the new string
    size_t r_length = 0;            // replacement length
    size_t in_length = 0;           // length of the input string
    size_t tbr_len = rs->tbr_end - rs->tbr_start;
    size_t ni = 0;                  // new index
    int err_off = 0;
    const char* err = NULL;
    unsigned char* ns = NULL;       // new string
    int match_count = 0;
    int ovector[300] = { 0 };

    in_length = string_length(rs->in);
    unsigned char *expression = zmalloc(tbr_len + 1);
    strncpy(expression, rs->ps + rs->tbr_start, tbr_len);

    pcre* pc = pcre_compile((char*)expression, 0, &err, &err_off, NULL);

    if(pc == NULL)
    {
        sfree((void**)&expression);
        return (NULL);
    }

    /* ***Copied from http://libs.wikia.com/wiki/Pcre_exec***
     * The first pair of integers, ovector[0] and ovector[1],
     * identify the portion of the subject string matched by the entire pattern.
     * The next pair is used for the first capturing subpattern, and so on.
     * The value returned by pcre_exec() is one more than the highest numbered pair that has been set.
     * For example, if two substrings have been captured, the returned value is 3.
     * If there are no capturing subpatterns, the return value from a successful match is 1,
     * indicating that just the first pair of offsets has been set.
     */
    match_count = pcre_exec(pc, NULL, (char*)rs->in, in_length, 0, 0, ovector, sizeof(ovector) / sizeof(int));
    pcre_free(pc);
    sfree((void**)&expression);

    if(match_count < 1)
    {

        return (NULL);
    }

    /*
     * Obtain the length of the replacements
     * The string length is obtained using the following:
     * new_length=initial_length-length_matched_by_pattern
     */

    for(size_t i = rs->rl_start; i < rs->rl_end; i++)
    {
        int index = 0;

        if(rs->ps[i] == '$' && isdigit(rs->ps[i + 1]))
        {
            unsigned char* na = NULL;
            index = strtoul((char*)rs->ps + i + 1, (char**)&na, 10);

            if(index && index <= match_count)
            {
                r_length += (ovector[2 * (index) + 1] - ovector[2 * (index)]);
            }

            i = (na - rs->ps) - 1;
        }
        else
        {
            r_length++;
        }
    }

    /*Good, we do have the new length
     * Let's allocate some memory*/

    new_length = in_length - (ovector[1] - ovector[0]);
    new_length += r_length;
    ns = zmalloc(new_length + 1);

    if(ns == NULL)
    {
        return (NULL);
    }

    if(ovector[0] > 0)
    {
        strncpy(ns, rs->in, ovector[0]);
    }

    for(size_t i = rs->rl_start; i < rs->rl_end; i++)
    {
        int index = 0;

        if(rs->ps[i] == '$' && isdigit(rs->ps[i + 1])) /*Get the index*/
        {
            unsigned char* na = NULL;
            index = strtoul(rs->ps + i + 1, (char**)&na, 10);

            if(index && index <= match_count)
            {
                size_t str_off = ovector[2 * (index)];                           /*Get start of string*/
                size_t ov_len = ovector[2 * (index) + 1] - ovector[2 * (index)]; /*Get the length of the string*/
                strncpy(ns + ovector[0] + ni, rs->in + str_off, ov_len);
                ni += ov_len;
            }

            i = (na - rs->ps) - 1;
        }
        else
        {
            (ns + ovector[0])[ni++] = rs->ps[i];
        }
    }

    /*Let's complete the rest of the string*/

    if((size_t)ovector[1] < in_length)
    {
        strcpy(ns + r_length + ovector[0], rs->in + ovector[1]);
    }

    return (ns);
}

static int string_util_replace_tokenizer_filter(string_tokenizer_status *pi, void* pv)
{
    replacer_tokenizer_status* rts = pv;

    if(rts->quote_type == 0 && (pi->buf[pi->pos] == '"' || pi->buf[pi->pos] == '\''))
    {
        rts->quote_type = pi->buf[pi->pos];
    }

    if(pi->buf[pi->pos] == rts->quote_type)
    {
        rts->quotes++;
    }

    if(rts->quotes % 2)
    {
        return(0);
    }
    else
    {
        rts->quote_type = 0;
    }

    if(pi->buf[pi->pos] == '(')
    {
        if(++rts->op == 1)
        {
            return(1);
        }
    }

    if(rts->op && pi->buf[pi->pos] == ')')
    {
        if(--rts->op == 0)
        {
            return(0);
        }
    }

    if(pi->buf[pi->pos] == ';')
    {
        return (rts->op == 0);
    }

    if(rts->op % 2 && pi->buf[pi->pos] == ',')
    {
        return (1);
    }

    return (0);
}


unsigned char *replace(unsigned char* in, unsigned char* rep_pair, unsigned char regexp)
{
    unsigned char begin_pair = 0;
    unsigned char end_pair = 0;
    unsigned char step = 0;
    unsigned char *work = NULL;
    size_t tvec[256] = {0};
    replacer_tokenizer_status rts = { 0 };
    replace_state rs =
    {
        .ps = rep_pair,
        .in = in,
        .tbr_start = 0,
        .tbr_end = 0,
        .rl_start = 0,
        .rl_end = 0,
    };

    string_tokenizer_info    sti =
    {
        .buffer                  = rep_pair,
        .filter_data             = &rts,
        .string_tokenizer_filter = string_util_replace_tokenizer_filter,
        .ovecoff                 = NULL,
        .oveclen                 = 0
    };

    if(rep_pair == NULL || in == NULL)
    {
        return (NULL);
    }

    string_tokenizer(&sti);

    memcpy(tvec, sti.ovecoff, sti.oveclen * sizeof(size_t));

    for(size_t i = 0; i < sti.oveclen / 2; i++)
    {
        size_t start = sti.ovecoff[2 * i];
        size_t end   = sti.ovecoff[2 * i + 1];

        if(start == end)
        {
            continue;
        }

        if(string_strip_space_offsets(rep_pair, &start, &end) == 0)
        {

            if(rep_pair[start] == '(')
            {
                start++;
                begin_pair = 1;
                step = 0;
            }

            if(rep_pair[start] == ',')
            {
                start++;
            }

            if(rep_pair[start] == ';')
            {
                begin_pair = 0;
                end_pair = 0;
                start++;
            }

            if(rep_pair[end - 1] == ')')
            {
                end--;
                end_pair = 1;
            }
        }

        if(string_strip_space_offsets(rep_pair, &start, &end) == 0)
        {
            if(rep_pair[start] == '"' || rep_pair[start] == '\'')
                start++;

            if(rep_pair[end - 1] == '"' || rep_pair[end - 1] == '\'')
                end--;
        }

        if(step >= 2) //safety measure
        {
            continue;
        }

        if(begin_pair == 1 && end_pair == 0)
        {
            rs.tbr_start = start;
            rs.tbr_end = end;
        }
        else if(end_pair == 1)
        {
            rs.rl_start = start;
            rs.rl_end = end;

            if(!regexp || (work = string_util_do_replacements_pcre(&rs)) == NULL)
            {
                work = string_util_do_replacements_plain(&rs);
            }

            if(rs.in != in)
            {
                sfree((void**)&rs.in);
                rs.in = work;
            }
            else
            {
                rs.in = work;
            }
        }

        step++;
    }

    sfree((void**)&sti.ovecoff);
    return (rs.in == in ? NULL : rs.in);
}
