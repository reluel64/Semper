/*Text input source
 * Part of Project 'Semper'
 * Writen by Alexandru-Daniel Mărgărit
 */


/*This is a very early implementation
 * It only supports text input from Windows (Linux will come soon) and even so, the functionality is very limited
 * Anyway, the aim of this source is to enable the user to input text
 * The reason that this is not added as a default option to the core but as a source is that it has to be bounded to an object to work*/
#include <mem.h>
#include <string_util.h>
#include <stdio.h>
#include <semper.h>
#include <surface.h>
#include <crosswin/crosswin.h>
#include <sources/source.h>
#include <sources/extension.h>
#include <ctype.h>
typedef struct
{
    unsigned int *buf;
    size_t  cbuf_pos;   /*current position in buffer*/
    size_t buf_lim;     /*buffer limit - read only*/
    size_t cbuf_len;
    size_t cursor_pos;  /*where is the cursor positioned*/
    size_t str_len;     /*The string length. Note: this is not the actual buffer size*/
    unsigned char digit_only;
    void *ip;
    unsigned char *ret_str; /*the string that should be returned or used for command processing*/
    window *w;
} input_text;

static int input_kbd_handler(unsigned int key_code,void *pv)
{
    input_text *it=pv;
#ifdef DEBUG
     printf("KeyCode %d\n",key_code);
#endif   
    if(key_code==(unsigned int)'\b')
    {
        it->buf[it->cbuf_pos]=0;
        
        if(it->cbuf_pos)
        {
            it->cbuf_pos--;
        }
    }
    else if(it->digit_only&&!isdigit(key_code))
    {
        return(-1);
    }
    else if(it->buf_lim==0||it->cbuf_pos<it->buf_lim)
    {
        /*Make the buffer comfy for the new character*/
        if(it->buf_lim==0&&it->cbuf_len<=it->cbuf_pos+1)
        {
            unsigned int *temp=zmalloc(sizeof(unsigned int)*(it->cbuf_len+2));
            memcpy(temp,it->buf,sizeof(unsigned int)*it->cbuf_len);
            sfree((void**)&it->buf);
            it->buf=temp;
            it->cbuf_len++;
        }
        it->buf[it->cbuf_pos++]=key_code;
    }
    extension_send_command(it->ip,"UpdateSurface()");
    return(0);
}

void input_init(void **spv,void *ip)
{
    input_text *it=zmalloc(sizeof(input_text));
    it->ip=ip;
    it->w=((surface_data*)((source*)ip)->sd)->sw;
    *spv=it;
}

void input_reset(void *spv,void *ip)
{
    input_text *it=spv;
    sfree((void**)&it->buf);
    it->buf_lim=extension_size_t("MaxTextSize",ip,0);
    it->digit_only=extension_bool("DigitOnly",ip,0);

    if(it->buf_lim)
    {
        it->buf=zmalloc((it->buf_lim+1)*sizeof(unsigned int));
    }
    else
    {
        it->buf=zmalloc(sizeof(unsigned int)*3);
        it->cbuf_len=2;
    }
}

double input_update(void *spv)
{
    input_text *it=spv;


    if(it->w->kb_data!=it)
    {
        it->buf[it->cbuf_pos]=' ';
    }
    else
    {
        if(it->cbuf_pos==0)
        {
            it->buf[it->cbuf_pos]= (it->buf[it->cbuf_pos]=='|'?' ':'|');
        }
        else
        {
            it->buf[it->cbuf_pos]= (it->buf[it->cbuf_pos]=='|'?0:'|');
        }
    }
    return(-1.0);
}

unsigned char *input_string(void *spv)
{
    input_text *it=spv;
    sfree((void**)&it->ret_str);
    it->ret_str=ucs32_to_utf8(it->buf,NULL,0);
    return(it->ret_str);
}

void input_command(void *spv,unsigned char *comm)
{
    input_text *it=spv;
    if(comm)
    {
        if(!strcasecmp(comm,"Read"))
        {
            crosswin_set_kbd_handler(it->w,input_kbd_handler,spv);
        }
        else if(!strcasecmp(comm,"StopRead") && it->w->kb_data==it)
        {
            crosswin_set_kbd_handler(it->w,NULL,NULL);
        }
    }
}

void input_destroy(void **spv)
{
    input_text *it=*spv;
    sfree((void**)&it->ret_str);
    sfree((void**)&it->buf);
    if(it->w->kb_data==it)
    {
        crosswin_set_kbd_handler(it->w,NULL,NULL);
    }
    sfree(spv);
}
