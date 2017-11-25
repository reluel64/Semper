/*Text input source
 * Part of Project 'Semper'
 * Writen by Alexandru-Daniel Mărgărit
 */


/*This is a very early implementation
   Anyway, the aim of this source is to enable the user to input text
 * The reason that this is not added as a default option to the core but as a source is that it has to be binded to an object to work*/
#include <mem.h>
#include <string_util.h>
#include <stdio.h>
#include <semper.h>
#include <surface.h>
#include <crosswin/crosswin.h>
#include <sources/source.h>
#include <sources/extension.h>
#include <ctype.h>
#include <enumerator.h>


typedef struct
{
    unsigned char *command;

    size_t index;
    list_entry current;
} input_text_command;

typedef struct
{
    size_t  buf_pos;   /*current position in buffer*/
    size_t buf_lim;     /*buffer limit - read only*/
    size_t buf_sz;
    size_t cursor_pos;  /*where is the cursor positioned*/
    size_t ret_str_len;
    size_t ovec_pos;
    size_t wcommand;
    size_t start_command;
    size_t end_command;
    unsigned int *buf;
    unsigned char number_only;
    unsigned char dot_set;
    unsigned char active;
    unsigned char *current_command;
    unsigned char *ret_str; /*the string that should be returned or used for command processing*/
    void *ip;
    window *w;
    list_entry commands;
    extension_string_tokenizer_info esti;
} input_text;


typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} input_tokenizer_status;


static int input_parse_string_filter(extension_string_tokenizer_status *pi, void* pv)
{
    input_tokenizer_status* its = pv;

    if(pi->reset)
        memset(its,0,sizeof(input_tokenizer_status));


    if(its->quote_type==0 && (pi->buf[pi->pos]=='"'||pi->buf[pi->pos]=='\''))
        its->quote_type=pi->buf[pi->pos];

    if(pi->buf[pi->pos]==its->quote_type)
        its->quotes++;

    if(its->quotes%2)
        return(0);
    else
        its->quote_type=0;

    if(pi->buf[pi->pos] == ';')
        return (its->quote_type==0);

    if(its->op%2&&pi->buf[pi->pos] == ',')
        return (1);

    return (0);
}


/*This routine will search for a command that has $TextInput$
 and execute the ones without it*/
static int input_get_command(input_text *it)
{
    input_text_command *itc=NULL;
    int found=0;


    while(1)
    {

        if(it->ovec_pos>=it->esti.oveclen/2)
        {
            if(it->start_command<it->end_command)
            {
                it->wcommand= it->start_command++;
            }
            else if(it->start_command>it->end_command)
            {
                it->wcommand= it->start_command--;
            }
            else
            {
                it->wcommand=it->start_command;
            }
        }

        list_enum_part(itc,&it->commands,current)
        {
            if(it->ovec_pos>=it->esti.oveclen/2)
            {
                extension_tokenize_string_free(&it->esti);
            }

            if(itc->index== it->wcommand)
            {
                if(it->ovec_pos>=it->esti.oveclen/2)
                {
                    it->ovec_pos=0;
                    it->current_command=itc->command;

                    input_tokenizer_status its= {0};

                    it->esti.string_tokenizer_filter=input_parse_string_filter;
                    it->esti.filter_data=&its;
                    it->esti.buffer=itc->command;
                    extension_tokenize_string(&it->esti);
                    it->esti.string_tokenizer_filter=NULL;
                    it->esti.filter_data=NULL;
                }
                else
                {
                    it->ovec_pos++;
                }

                for(; it->ovec_pos<it->esti.oveclen/2; it->ovec_pos++)
                {
                    for(size_t i=it->esti.ovecoff[it->ovec_pos*2]; it->esti.ovecoff[it->ovec_pos*2+1]-i>=11; i++)
                    {
                        unsigned char *str=it->current_command+i;

                        if(strncasecmp(str,"$TextInput$",11)==0)
                        {
                            /*Found a string that has $TextInput$*/
                            found=1;
                            break;
                        }
                    }

                    if(found)
                    {
                        break;
                    }
                    else
                    {
                        unsigned char *comm=it->current_command+it->esti.ovecoff[it->ovec_pos*2];
                        unsigned char temp=it->current_command[it->esti.ovecoff[it->ovec_pos*2+1]];
                        it->current_command[it->esti.ovecoff[it->ovec_pos*2+1]]=0;
                        extension_send_command(it->ip,comm);
                        it->current_command[it->esti.ovecoff[it->ovec_pos*2+1]]=temp;
                    }
                }

                if(found)
                    break;
            }

        }

        if(found)
            break;

        if(it->start_command==it->end_command)
        {
            found=-1;
            break;
        }
    }

    return(found);
}


static void input_update_ret_buf(input_text *it)
{
    unsigned char *ret_tmp=NULL;
    sfree((void**)&it->ret_str);
    it->ret_str_len=0;
    ret_tmp=ucs32_to_utf8(it->buf,&it->ret_str_len,0);
    it->ret_str=zmalloc(it->ret_str_len+3);
    strncpy(it->ret_str,ret_tmp,it->ret_str_len);
    sfree((void**)&ret_tmp);
}

static void input_exec_handler(input_text *it)
{
    unsigned char *user_inp=ucs32_to_utf8(it->buf,NULL,0);
    memset(it->buf,0,it->buf_pos*sizeof(unsigned int));

    it->buf_pos=0;
    input_update_ret_buf(it);

    if(it->ovec_pos<it->esti.oveclen/2&&user_inp)
    {
        size_t start=it->esti.ovecoff[it->ovec_pos*2];
        size_t end=it->esti.ovecoff[it->ovec_pos*2+1];

        if(it->current_command[start]==';')
            start++;

        size_t len=end-start;
        unsigned char *temp=zmalloc(len+1);
        strncpy(temp,it->current_command+start,len);
        size_t flen=16+string_length(user_inp);
        unsigned char *pair=zmalloc(flen+1);
        snprintf(pair,flen,"($TextInput$,%s)",user_inp);

        unsigned char *final_command=replace(temp,pair,0);
        sfree((void**)&temp);
        sfree((void**)&pair);

        extension_send_command(it->ip,final_command);
        sfree((void**)&final_command);
    }

    sfree((void**)&user_inp);

    if(input_get_command(it)<0)
    {
        it->ovec_pos=0;
        it->active=0;
        it->esti.oveclen=0;
        it->ret_str[it->ret_str_len]=0;
        extension_tokenize_string_free(&it->esti);
        crosswin_set_kbd_handler(it->w,NULL,NULL);
    }
}

static void input_destroy_list(input_text *it)
{
    input_text_command *itc=NULL;
    input_text_command *titc=NULL;
    list_enum_part_safe(itc,titc,&it->commands,current)
    {
        sfree((void**)&itc->command);
        sfree((void**)&itc);
    }
}


static void input_populate_list(input_text *it)
{

    void* es = NULL; // enumerator internal state
    input_destroy_list(it);
    unsigned char* ev = enumerator_first_value(it->ip, ENUMERATOR_SOURCE, &es);

    do
    {
        if(ev==NULL)
            break;

        if(strncasecmp("Command",ev,7))
            continue;

        input_text_command *itc=zmalloc(sizeof(input_text_command));
        itc->command=clone_string(extension_string(ev,EXTENSION_XPAND_ALL,it->ip,NULL));
        //printf("%s\n",itc->command);
        sscanf(ev,"Command%llu",&itc->index);
        list_entry_init(&itc->current);
        linked_list_add_last(&itc->current,&it->commands);
    }
    while((ev=enumerator_next_value(es))!=NULL);

    enumerator_finish(&es);
}

static int input_kbd_handler(unsigned int key_code,void *pv)
{
    input_text *it=pv;
#ifdef DEBUG
    printf("KeyCode %d\n",key_code);
#endif

    if(key_code=='\r')
    {
        input_exec_handler(it);
    }
    else
    {
        if(key_code==(unsigned int)'\b')
        {
            if(it->buf_pos)
                it->buf_pos--;

            if(it->number_only&&it->buf[it->buf_pos]=='.')
                it->dot_set=0;

            it->buf[it->buf_pos]=0;
            input_update_ret_buf(it);
            extension_send_command(it->ip,"UpdateSurface()");
            return(0);
        }

        if(it->number_only)
        {
            if(((it->buf_pos==0&&key_code=='.')||
                    (it->dot_set&&key_code=='.'))||
                    (it->buf_pos&&key_code=='-') ||
                    (!isdigit(key_code)&&key_code!='-'&&key_code!='.'))
                return(-1);
            else if(it->buf_pos!=0&&key_code=='.'&&it->dot_set==0)
                it->dot_set=1;

        }

        if(it->buf_lim==0||it->buf_pos<it->buf_lim)
        {
            /*Make the buffer comfy for the new character*/

            if(it->buf_lim==0&&it->buf_sz<=it->buf_pos+1)
            {
                unsigned int *temp=zmalloc(sizeof(unsigned int)*(it->buf_sz+2+8192));
                memcpy(temp,it->buf,sizeof(unsigned int)*it->buf_sz);
                sfree((void**)&it->buf);
                it->buf=temp;
                it->buf_sz+=8192;
            }

            it->buf[it->buf_pos++]=key_code;
            input_update_ret_buf(it);

        }
    }

    extension_send_command(it->ip,"UpdateSurface()");
    return(0);
}

void input_init(void **spv,void *ip)
{
    input_text *it=zmalloc(sizeof(input_text));
    it->ip=ip;
    list_entry_init(&it->commands);
    it->w=((surface_data*)((source*)ip)->sd)->sw;
    *spv=it;
}

void input_reset(void *spv,void *ip)
{
    input_text *it=spv;
    sfree((void**)&it->buf);
    it->buf_lim=extension_size_t("MaxTextSize",ip,0);
    it->number_only=extension_bool("NumberOnly",ip,0);
    it->ret_str=zmalloc(2);
    it->current_command=NULL;
    input_populate_list(it);

    extension_tokenize_string_free(&it->esti);
    it->ovec_pos=0;

    if(it->buf_lim)
    {
        it->buf=zmalloc((it->buf_lim+1)*sizeof(unsigned int));
    }
    else
    {
        it->buf=zmalloc(sizeof(unsigned int)*3);
        it->buf_sz=2;
    }
}

double input_update(void *spv)
{
    input_text *it=spv;

    if(it->active)
    {
        if(it->w->kb_data!=it)
            it->ret_str[it->ret_str_len]=0;
        else
            it->ret_str[it->ret_str_len]= (it->ret_str[it->ret_str_len]=='|'?0:'|');
    }

    return(-1.0);
}

unsigned char *input_string(void *spv)
{
    input_text *it=spv;

    return(it->ret_str?it->ret_str:(unsigned char*)"");
}

void input_command(void *spv,unsigned char *comm)
{
    input_text *it=spv;

    if(comm)
    {
        if(strncasecmp(comm,"Read",4)==0)
        {
            if(it->active==0)
            {
                it->active=1;
                it->start_command=0;
                sscanf(comm,"Read %llu-%llu",&it->start_command,&it->end_command);

                if(strchr(comm,'-')==NULL)
                {
                    it->end_command=it->start_command+1;
                }

                if(input_get_command(it)>0)
                    crosswin_set_kbd_handler(it->w,input_kbd_handler,spv);
            }
            else if(it->active)
            {
                crosswin_set_kbd_handler(it->w,input_kbd_handler,spv);
            }
        }
        else if(it->active==1&&!strcasecmp(comm,"StopRead") && it->w->kb_data==it)
        {
            it->active=0;
            extension_tokenize_string_free(&it->esti);
            it->ovec_pos=0;
            it->esti.oveclen=0;
            crosswin_set_kbd_handler(it->w,NULL,NULL);
            it->ret_str[it->ret_str_len]=0;
        }
    }
}

void input_destroy(void **spv)
{
    input_text *it=*spv;
    sfree((void**)&it->ret_str);
    sfree((void**)&it->buf);
    extension_tokenize_string_free(&it->esti);
    input_destroy_list(it);

    if(it->w->kb_data==it)
    {
        crosswin_set_kbd_handler(it->w,NULL,NULL);
    }

    sfree(spv);
}
