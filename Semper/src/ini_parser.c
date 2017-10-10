/* Callback based ini parser
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#include <mem.h>
#include <ini_parser.h>
#include <stdio.h>
#include <wchar.h>
#include <string_util.h>
#include <ctype.h>
typedef enum _encoding { ucs2 = 1, ucs2_be } encoding;

typedef struct _ini_ucs2
{
    FILE* fh;
    encoding enc;
} ini_ucs2;

static unsigned char* remove_trailing_spaces(unsigned char* s)
{
    size_t sz = string_length(s);
    unsigned char* p = s + sz;
    while(p > s && isspace(*--p))
    {
        *p = 0;
    }
    return (s);
}

static unsigned char* remove_heading_spaces(unsigned char* s)
{
    while(*s && isspace(*s))
    {
        s++;
    }
    return (s);
}

static unsigned char* ini_find_chars(unsigned char* s, unsigned char* cs)
{
    int space = 0;
    unsigned long i = 0;
    for(; s[i]; i++)
    {
        if((cs == NULL || !strchr(cs, s[i])) && !(space && strchr("#;", s[i])))
        {
            space = isspace(s[i]);
        }
        else
        {
            break;
        }
    }
    return (s + i);
}

static inline char* ucs2_line_get(unsigned char* buf, size_t buf_sz, ini_ucs2* iu2)
{
    unsigned short* wline = zmalloc((INI_PARSER_MAX_LINE + 1) * 2);

    if(fgetws((wchar_t*)wline, INI_PARSER_MAX_LINE, iu2->fh))
    {
        unsigned char* str = ucs_to_utf8(wline, NULL, iu2->enc == ucs2_be);
        sfree((void**)&wline);
        strncpy(buf, str, buf_sz);
        sfree((void**)&str);
        return (buf);
    }
    sfree((void**)&wline);
    return (NULL);
}

static inline encoding ini_detect_encoding(FILE* fh)
{
    unsigned char bom[2] = { 0 };

    fread(bom, 1, 2, fh);

    fseeko64(fh, 0, SEEK_SET);

    if(bom[0] == 0xFF && bom[1] == 0xFE)
    {
        return (ucs2);
    }
    else if(bom[0] == 0xFE && bom[1] == 0xFF)
    {
        return (ucs2_be);
    }
    else
    {
        return (0);
    }
}

int ini_parser_parse_stream(ini_reader ir, void* data, ini_handler ih, void* pv)
{
    unsigned char* buf = zmalloc(INI_PARSER_MAX_LINE + 1);
    unsigned char* sn = zmalloc(INI_PARSER_MAX_LINE + 1);
    // unsigned char *com=zmalloc(INI_PARSER_MAX_LINE+1);
    int ret = 0;
    if(buf == NULL || sn == NULL)
    {
        sfree((void**)&buf);
        sfree((void**)&sn);
        return (-1);
    }

    size_t cline = 0;
    while(ir(buf, INI_PARSER_MAX_LINE, data))
    {
        size_t offset = 0;
        cline++;
        unsigned char* line = NULL;

        if(cline == 1 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) /*BOM signature*/
        {
            offset += 3;
        }

        line = offset + buf; // set the beginning of the line
        remove_character(line, '\n'); // remove those nasty characters
        remove_character(line, '\r');

        /*we do not want spaces, don't we?*/
        remove_trailing_spaces(line);
        line = remove_heading_spaces(line);

        if(*(line) == ';' ||  *(line) == '#') /*this is a comment started at the beginning of the line so everything else is wiped out*/
        {
            // memset(sn,0,INI_PARSER_MAX_LINE);

            if(ih)
            {
                ret = ih(sn, NULL, NULL, line, pv);
            }
        }
        else if(*line == '[')
        {
            unsigned char* se = ini_find_chars(line, "]"); // get the section end

            if(*se != ']') // if there is no closing ']' then the section is considered invalid
            {
                continue;
            }
            memset(sn, 0, INI_PARSER_MAX_LINE);
            strncpy(sn, line + 1, (se - line) - 1);
            line = remove_heading_spaces(se + 1);
            remove_trailing_spaces(line);

            if(ih)
            {
                ret = ih(sn, NULL, NULL, line, pv);
            }
        }
        else if(*line)
        {
            unsigned char* delim = ini_find_chars(line, "=:");
            if(*delim == '=' || *delim == ':')
            {
                *delim = 0;

                unsigned char* key_name = remove_trailing_spaces(line);
                unsigned char* key_value = remove_heading_spaces(delim + 1);
                unsigned char* in_comm = ini_find_chars(key_value, NULL);
                //*in_comm=0;
                if(isspace(*(in_comm - 1)))
                {
                    *(in_comm - 1) = 0;
                }
                remove_trailing_spaces(key_value);
                remove_trailing_spaces(in_comm);

                remove_end_begin_quotes(key_value);

                if(ih)
                {
                    ret = ih(sn, key_name, key_value, in_comm, pv);
                }
            }
            else
            {
                unsigned char* key_value = remove_heading_spaces(line);
                unsigned char* in_comm = ini_find_chars(key_value, NULL);
                if(isspace(*(in_comm - 1)))
                {
                    *(in_comm - 1) = 0;
                }
                remove_trailing_spaces(key_value);
                remove_end_begin_quotes(key_value);
                remove_trailing_spaces(in_comm);

                if(ih)
                {
                    ret = ih(NULL, NULL, key_value, in_comm, pv); // the continue the key value on an new line
                }
            }
        }
        else
        {
            if(ih)
            {
                ret = ih(NULL, NULL, NULL, NULL, pv); // this will signal that there is a newline
            }
        }

        if(ret)
        {
            break;
        }
    }
    sfree((void**)&buf);
    sfree((void**)&sn);
    return (ret);
}

int ini_parser_parse_file(unsigned char* fn, ini_handler ih, void* pv)
{
    FILE* fh = NULL;
    int ret = -1;

#ifdef WIN32
    unsigned short* ufn = utf8_to_ucs(fn);
    fh = _wfopen(ufn, L"rb");
    fwide(fh, -1);
    sfree((void**)&ufn);
#else
    fh = fopen(fn, "rb");
#endif

    if(fh == NULL)
    {
        return (-1);
    }
    encoding enc = ini_detect_encoding(fh);

    if(enc != ucs2 && enc != ucs2_be)
    {
        ret = ini_parser_parse_stream((ini_reader)fgets, fh, ih, pv);
    }
    else
    {
        fseek(fh, 2, SEEK_SET);
        ini_ucs2 iu2 = { 0 };
        iu2.enc = enc;
        iu2.fh = fh;
        ret = ini_parser_parse_stream((ini_reader)ucs2_line_get, &iu2, ih, pv);
    }

    fclose(fh);
    return (ret);
}
