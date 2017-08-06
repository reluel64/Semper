/*
*String replacement routines
*Part of Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/
#define PCRE_STATIC
#include <mem.h>
#include <string_util.h>
#include <pcre.h>
#include <ctype.h>

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

static unsigned char* perform_replacements(replace_state* rs)
{
    size_t nm = 0;
    size_t di = 0;
    size_t rl_len=rs->rl_end-rs->rl_start;
    size_t tbr_len=rs->tbr_end-rs->tbr_start;

    unsigned char* ret = NULL;
    if(tbr_len==0)
    {
        return(NULL);
    }
    for(size_t i = 0; rs->in[i]; i++)
    {
        if(tbr_len && strncmp(rs->in + i, rs->ps+rs->tbr_start, tbr_len) == 0)
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
        if(tbr_len && strncmp(rs->in + i, rs->ps+rs->tbr_start, tbr_len) == 0)
        {
            strncpy(ret + di, rs->ps+rs->rl_start,rl_len);
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

static unsigned char* perform_replacements_pcre(replace_state* rs)
{

    if(!rs || !rs->in || (rs->tbr_end-rs->tbr_start)==0)
    {
        return (NULL);
    }

    size_t new_length = 0;          // total length of the new string
    size_t r_length = 0;            // replacement length
    size_t in_length = 0;           // length of the input string
    size_t tbr_len=rs->tbr_end-rs->tbr_start;
    size_t ni = 0;                  // new index
    int err_off = 0;
    const char* err = NULL;
    unsigned char* ns = NULL;       // new string
    int match_count = 0;
    int ovector[300] = { 0 };

    in_length = string_length(rs->in);
    unsigned char *expression=zmalloc(tbr_len+1);
    strncpy(expression,rs->ps+rs->tbr_start,tbr_len);

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

    for(size_t i = rs->rl_start; i<rs->rl_end; i++)
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

    for(size_t i = rs->rl_start; i<rs->rl_end; i++)
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

static int replacer_tokenizer_filter(string_tokenizer_status *pi, void* pv)
{
    replacer_tokenizer_status* rts = pv;

    if(rts->quote_type==0 && (pi->buf[pi->pos]=='"'||pi->buf[pi->pos]=='\''))
    {
        rts->quote_type=pi->buf[pi->pos];
    }

    if(pi->buf[pi->pos]==rts->quote_type)
    {
        rts->quotes++;
    }

    if(rts->quotes%2)
    {
        return(0);
    }
    else
    {
        rts->quote_type=0;
    }

    if(pi->buf[pi->pos] == '(')
    {
        if(++rts->op==1)
        {
            return(1);
        }
    }

    if(rts->op&&pi->buf[pi->pos] == ')')
    {
        if(--rts->op==0)
        {
            return(0);
        }
    }

    if(pi->buf[pi->pos] == ';')
    {
        return (rts->op==0);
    }

    if(rts->op%2&&pi->buf[pi->pos] == ',')
    {
        return (1);
    }
    return (0);
}


unsigned char *replace(unsigned char* in, unsigned char* rep_pair, unsigned char regexp)
{
    unsigned char begin_pair=0;
    unsigned char end_pair=0;
    unsigned char step=0;
    unsigned char *work=NULL;
    size_t tvec[256]= {0};
    replace_state rs=
    {
        .ps=rep_pair,
        .in=in,
        .tbr_start=0,
        .tbr_end=0,
        .rl_start=0,
        .rl_end=0,
    };
    replacer_tokenizer_status rts = { 0 };
    /*command_handler_status   chs=  { 0 };*/
    string_tokenizer_info    sti=
    {
        .buffer                  = rep_pair,
        .filter_data             = &rts,
        .string_tokenizer_filter = replacer_tokenizer_filter,
        .ovecoff                 = NULL,
        .oveclen                 = 0
    };

    if(rep_pair == NULL || in == NULL)
    {
        return (NULL);
    }

    string_tokenizer(&sti);

    memcpy(tvec,sti.ovecoff,sti.oveclen*sizeof(size_t));
    for(size_t i=0; i<sti.oveclen/2; i++)
    {
        size_t start = sti.ovecoff[2*i];
        size_t end   = sti.ovecoff[2*i+1];

        if(start==end)
        {
            continue;
        }

        if(string_strip_space_offsets(rep_pair,&start,&end)==0)
        {

            if(rep_pair[start]=='(')
            {
                start++;
                begin_pair=1;
                step=0;
            }
            if(rep_pair[start]==',')
            {
                start++;
            }
            if(rep_pair[start]==';')
            {
                begin_pair=0;
                end_pair=0;
                start++;
            }
            if(rep_pair[end-1]==')')
            {
                end--;
                end_pair=1;
            }
        }
        if(string_strip_space_offsets(rep_pair,&start,&end)==0)
        {
            if(rep_pair[start]=='"'||rep_pair[start]=='\'')
                start++;

            if(rep_pair[end-1]=='"'||rep_pair[end-1]=='\'')
                end--;
        }

        if(step>=2) //safety measure
        {
            continue;
        }

        if(begin_pair==1&&end_pair==0)
        {
            rs.tbr_start=start;
            rs.tbr_end=end;
        }
        else if(end_pair==1)
        {
            rs.rl_start=start;
            rs.rl_end=end;
            
            if(regexp)
            {
                if((work=perform_replacements_pcre(&rs))==NULL)
                {
                     work=perform_replacements(&rs);
                }
            }
            else
            {
                work=perform_replacements(&rs);
            }
            
            if(rs.in!=in)
            {
                sfree((void**)&rs.in);
                rs.in=work;
            }
            else
            {
                rs.in=work;
            }
        }
        step++;
    }
    sfree((void**)&sti.ovecoff);
    return (rs.in==in?NULL:rs.in);
}

