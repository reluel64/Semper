/* Timed source for timed / deferred events
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

/*The purpose of this source is to allow the user push events in the main queue for further processing
 * With this type of behaviour, the user could have a lower
 * refresh rate of the surface and yet still have nice
 * animations or background tasks only when they're needed.
 * This allows the CPU to be used for other tasks or just save power
 * */

#include <stdlib.h>
#include <mem.h>
#include <stdio.h>
#include <xpander.h>
#include <string_util.h>
#include <sources/extension.h>
#include <sources/source.h>
#include <enumerator.h>
#include <string.h>
#include <linked_list.h>
#include <parameter.h>
#include <ctype.h>
#include <sys/time.h>
#ifndef WIN32
#include <unistd.h>
#endif


typedef enum
{
    none=0,
    repeat,
    wait,
} timed_type;

typedef struct
{
    pthread_mutex_t mtx; /*this mutex will allow us to play dirty by changing the command in the last moment before passing it to the core*/
    size_t index;
    size_t wait_timeout;
    size_t repeat_count;
    unsigned char *act;
    list_entry current;
} timed_action_list;

typedef struct
{
    void *ip;		/*our path to extension_send_command*/
    list_entry current;
    list_entry act_chain;
    size_t action_index;
    unsigned char disabled;
    unsigned char running;
    pthread_mutex_t mutex;	/*This will prevent nasty stuff to happen*/
    pthread_cond_t cond;    /*signal this when the timed action has to be killed*/
} timed_list;

typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} timed_action_tokenizer_status;

/*Structure used by the TimedList parsing routine*/

static timed_action_list *timed_action_list_entry(list_entry *act_head,size_t index)
{
    timed_action_list *tal=NULL;
    timed_action_list *ltal=NULL;
    list_enum_part(ltal,act_head,current)
    {
        if(index==ltal->index)
        {
            tal=ltal;
            break;
        }
    }

    if(tal==NULL)
    {
        tal=zmalloc(sizeof(timed_action_list));
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&tal->mtx,&mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
        list_entry_init(&tal->current);
        tal->index=index;
        linked_list_add_last(&tal->current,act_head);
    }
    return(tal);
}

static timed_list *timed_list_entry(list_entry *list,size_t index)
{
    timed_list *ltl=NULL;
    timed_list *tl=NULL;
    list_enum_part(ltl,list,current)
    {
        if(ltl->action_index==index)
        {
            tl=ltl;
            break;
        }
    }

    if(tl==NULL)
    {
        tl=zmalloc(sizeof(timed_list));
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&tl->mutex,&mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
        pthread_cond_init(&tl->cond,NULL);
        list_entry_init(&tl->current);
        list_entry_init(&tl->act_chain);
        tl->action_index=index;
        linked_list_add_last(&tl->current,list);
    }
    return(tl);
}

static void *timed_action_exec(void *pv)
{
    timed_list *tl=pv;
    timed_action_list *tal=NULL;
    tl->running=1;
    pthread_mutex_lock(&tl->mutex);

    list_enum_part(tal,&tl->act_chain,current)
    {
        for(size_t i=0; i<tal->repeat_count; i++)
        {
            struct timeval tv;
            struct timespec ts;
            pthread_mutex_t lmtx;
            int ret=0;
            pthread_mutex_lock(&tal->mtx);
            tal->act!=NULL?extension_send_command(tl->ip,tal->act):0;
            pthread_mutex_unlock(&tal->mtx);

            gettimeofday(&tv, NULL);
            ts.tv_sec = time(NULL) + tal->wait_timeout / 1000;
            ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (tal->wait_timeout % 1000);
            ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
            ts.tv_nsec %= (1000 * 1000 * 1000);

            pthread_mutex_init(&lmtx,NULL);
            pthread_mutex_lock(&lmtx);
            ret=pthread_cond_timedwait(&tl->cond, &lmtx, &ts);
            pthread_mutex_unlock(&lmtx);
            pthread_mutex_destroy(&lmtx);

            if(ret==0)
                break;
        }
    }
    pthread_mutex_unlock(&tl->mutex);
    tl->running=0;
    return(NULL);
}

static int timed_action_string_filter(string_tokenizer_status *pi, void* pv)
{
    timed_action_tokenizer_status* tats = pv;

    if(pi->reset)
    {
        memset(tats,0,sizeof(timed_action_tokenizer_status));
        return(0);
    }

    if(tats->quote_type==0 && (pi->buf[pi->pos]=='"'||pi->buf[pi->pos]=='\''))
    {
        tats->quote_type=pi->buf[pi->pos];
    }

    if(pi->buf[pi->pos]==tats->quote_type)
    {
        tats->quotes++;
    }

    if(tats->quotes%2)
    {
        return(0);
    }
    else
    {
        tats->quote_type=0;
    }

    if(pi->buf[pi->pos] == '(')
    {
        if(++tats->op==1)
        {
            return(1);
        }
    }

    if(tats->op&&pi->buf[pi->pos] == ')')
    {
        if(--tats->op==0)
        {
            return(0);
        }
    }

    if(pi->buf[pi->pos] == ';')
    {
        return (tats->op==0);
    }

    if(tats->op%2&&pi->buf[pi->pos] == ',')
    {
        return (1);
    }
    return (0);
}


size_t timed_action_fill_list(list_entry *head,string_tokenizer_info *sti,void *ip)
{
    unsigned char push_params=0;
    unsigned char finished=0;
    unsigned char step=0;
    size_t entries=0;

    timed_type type=none;
    timed_action_list *tal=NULL;

    for(size_t i=0; i<sti->oveclen/2; i++)
    {
        size_t start = sti->ovecoff[2*i];
        size_t end   = sti->ovecoff[2*i+1];

        if(start==end)
        {
            continue;
        }

        /*Clean spaces*/
        if(string_strip_space_offsets(sti->buffer,&start,&end)==0)
        {
            if(sti->buffer[start]=='(')
            {
                start++;
                push_params=1;
            }
            else if(sti->buffer[start]==',')
            {
                start++;
            }
            else if(sti->buffer[start]==';')
            {
                push_params=0;
                step=0;
                finished=0;
                start++;

                type=none;
            }
            if(sti->buffer[end-1]==')')
            {
                end--;
                finished=1;
                step=0;
            }
        }

        if(string_strip_space_offsets(sti->buffer,&start,&end)==0)
        {
            if(sti->buffer[start]=='"'||sti->buffer[start]=='\'')
                start++;

            if(sti->buffer[end-1]=='"'||sti->buffer[end-1]=='\'')
                end--;
        }
        /********************************************************************/
        if(push_params&&finished==0) //receiving parameters
        {
            if(type==repeat)
            {
                if(step==0)
                {
                    unsigned char *kn=zmalloc((end-start)+1);
                    strncpy(kn,sti->buffer+start,end-start);
                    tal->act=clone_string(extension_string(kn,EXTENSION_XPAND_ALL,ip,NULL));
                    sfree((void**)&kn);
                    step++;
                }
                else if(step==1)
                {
                    tal->repeat_count=strtoull(sti->buffer+start,NULL,10)+1;
                    step++;
                }
            }
        }
        else if(push_params==0&&finished==0) //we've receiving the Wait/Repeat or just a simple action
        {
            tal=timed_action_list_entry(head,entries++);
            pthread_mutex_lock(&tal->mtx);
            sfree((void**)&tal->act);
            tal->wait_timeout=0;
            tal->repeat_count=1;

            if(strncasecmp("Repeat",sti->buffer+start,end-start)==0)
            {
                type=repeat;
            }
            else if(strncasecmp("Wait",sti->buffer+start,end-start)==0)
            {
                type=wait;
            }
            else
            {
                unsigned char *kn=zmalloc((end-start)+1);
                strncpy(kn,sti->buffer+start,end-start);
                tal->act=clone_string(extension_string(kn,EXTENSION_XPAND_ALL,ip,NULL));
                sfree((void**)&kn);
            }
        }
        else
        {
            tal->wait_timeout=((type==repeat||type==wait)?strtoull(sti->buffer+start,NULL,10):0);
            pthread_mutex_unlock(&tal->mtx);
        }
    }
    return (entries);
}

static void timed_action_destroy_list(list_entry *head)
{
    timed_list *tl=NULL;
    timed_list *ttl=NULL;
    list_enum_part_safe(tl,ttl,head,current)
    {
        timed_action_list *tal=NULL;
        timed_action_list *ttal=NULL;
        pthread_cond_signal(&tl->cond);                         /*signal the thread to end*/

        pthread_mutex_lock(&tl->mutex);                         /*wait for thread to complete*/
        linked_list_remove(&tl->current);                       /*remove the timed list from the chain*/
        pthread_mutex_unlock(&tl->mutex);                       /*unlock the mutex before destroying it*/
        pthread_mutex_destroy(&tl->mutex);                      /*destroy the mutex*/
        pthread_cond_destroy(&tl->cond);                        /*destroy the condition variable*/

        list_enum_part_safe(tal,ttal,&tl->act_chain,current)
        {
            linked_list_remove(&tal->current);                  /*remove the action*/
            pthread_mutex_lock(&tal->mtx);                      /*make sure we own the mutex*/
            sfree((void**)&tal->act);                           /*free the action string*/
            pthread_mutex_unlock(&tal->mtx);                    /*unlock the mutex*/
            pthread_mutex_destroy(&tal->mtx);                   /*destroy the mutex*/
            sfree((void**)&tal);                                /*free the slot*/
        }
        sfree((void**)&tl);                                     /*remove the timed entry*/
    }
}


/*Generic routines*/

void timed_action_init(void **spv, void *ip)
{
    list_entry *ta=zmalloc(sizeof(list_entry));
    list_entry_init(ta);
    *spv=ta;
}

void timed_action_reset(void *spv,void *ip)
{
    void *en=NULL;
    list_entry *ta=spv;
    unsigned char *kn=enumerator_first_value(ip,ENUMERATOR_SOURCE,&en);
    source *s=ip;

    do
    {

        key k=NULL;
        size_t act_index=0;
        if(kn==NULL)
        {
            break;
        }
        if(strncasecmp("TimedList",kn,9))
        {
            continue;
        }
        k=skeleton_get_key(s->cs,kn);

        if(k==NULL)
        {
            continue;
        }

        sscanf(kn,"TimedList%llu",&act_index);
        timed_action_tokenizer_status tats = {0};

        string_tokenizer_info sti=
        {
            .buffer= clone_string(extension_string(kn,EXTENSION_XPAND_ALL,ip,NULL)),
            .oveclen=0,
            .ovecoff=NULL,
            .filter_data=&tats,
            .string_tokenizer_filter=timed_action_string_filter
        };

        string_tokenizer(&sti);

        timed_list *tl=timed_list_entry(ta,act_index);
        tl->ip=ip;
        timed_action_fill_list(&tl->act_chain,&sti,ip);
        sfree((void**)&sti.ovecoff);
        sfree((void**)&sti.buffer);
    }
    while((kn=enumerator_next_value(en))!=NULL);

    enumerator_finish(&en);

}

void timed_action_command(void *spv,unsigned char *command)
{
    list_entry *ta=spv;
    if(command)
    {
        if(!strncasecmp("Start ",command,6))
        {
            unsigned char *end=NULL;
            size_t comm_index=strtoull((char*)command+6,(char**)&end,10);

            if(end!=command+6)
            {
                timed_list *tl=NULL;
                list_enum_part(tl,ta,current)
                {
                    if(tl->action_index==comm_index)
                    {
                        if(tl->running)
                        {
                            break;
                        }
                        pthread_attr_t th_att= {0};
                        pthread_t dtid;
                        pthread_attr_init(&th_att);
                        pthread_attr_setdetachstate(&th_att,PTHREAD_CREATE_DETACHED);
                        pthread_create(&dtid,&th_att,timed_action_exec,tl);
                        pthread_attr_destroy(&th_att);
                        break;
                    }
                }
            }
        }

        if(!strncasecmp("Stop ",command,5))
        {
            unsigned char *end=NULL;
            size_t comm_index=strtoull((char*)command+5,(char**)&end,10);

            if(end!=command+5)
            {
                timed_list *tl=NULL;
                list_enum_part(tl,ta,current)
                {
                    if(tl->action_index==comm_index)
                    {
                        pthread_cond_signal(&tl->cond);
                        pthread_mutex_lock(&tl->mutex);
                        pthread_mutex_unlock(&tl->mutex);
                        break;
                    }
                }
            }
        }
    }
}

void timed_action_destroy(void **spv)
{
    list_entry *ta=*spv;
    timed_action_destroy_list(ta);
    sfree(spv);
}
