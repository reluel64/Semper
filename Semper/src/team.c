/*
 * Teaming support
 * Part of Project "Semper"
 * Written by Alexandru-Daniel Mărgărit
 */

#include <string_util.h>
#include <mem.h>

static int team_filter(string_tokenizer_status *sts, void* pv)
{
    size_t *quotes = pv;

    if(sts->buf[sts->pos] == '"' || sts->buf[sts->pos] == '\'')
    {
        (*quotes)++;
    }

    if((*quotes) % 2 && sts->buf[sts->pos] == ';')
    {
        return (0);
    }
    else if(sts->buf[sts->pos] == ';')
    {
        return (1);
    }
    return (0);
}

unsigned char team_member(unsigned char *members,unsigned char *rmember)
{
    int found=0;
    size_t quotes=0;
    string_tokenizer_info sti=
    {
        .buffer=members,
        .oveclen=0,
        .ovecoff=NULL,
        .string_tokenizer_filter=team_filter,
        .filter_data=&quotes
    };

    if(members==NULL||rmember==NULL)
    {
        return(0);
    }

    string_tokenizer(&sti);

    for(size_t i=0; i<sti.oveclen/2; i++)
    {
        size_t start = sti.ovecoff[2*i];
        size_t end   = sti.ovecoff[2*i+1];

        if(string_strip_space_offsets(members,&start,&end)==0)
        {
            if(members[start]==';')
            {
                start++;
            }
        }
        if(string_strip_space_offsets(members,&start,&end)==0)
        {
            if(members[start]=='"'||members[start]=='\'')
                start++;

            if(members[end-1]=='"'||members[end-1]=='\'')
                end--;
        }

        if(!strncasecmp(members+start,rmember,end-start))
        {
            found=1;
            break;
        }
    }

    sfree((void**)&sti.ovecoff);
    return(found);
}
