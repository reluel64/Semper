/*
 * Memroy related routines
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#define ALLOC_FACTOR 1
#include <mem.h>
#include <stdio.h>
#include <string_util.h>

void* zmalloc(size_t bytes)
{
    if(bytes == 0)
    {
        return (NULL);
    }
    return (calloc(bytes, ALLOC_FACTOR));
}

void sfree(void** p)
{
    if(p)
    {
        free(*p);
        *p = NULL;
    }
}

int put_file_in_memory(unsigned char* file, void** out, size_t* sz)
{
    FILE* f = NULL;
#ifdef WIN32
    wchar_t* fp = utf8_to_ucs(file);
    f = _wfopen(fp, L"rb");
    sfree((void**)&fp);
#elif __linux__
    f = fopen(file, "rb");
#endif

    if(f)
    {
        fseek(f, 0, SEEK_END);
#ifdef WIN32
        *sz = _ftelli64(f);
#else
        *sz = ftello(f);
#endif
        fseek(f, 0, SEEK_SET);
        *out = zmalloc(*sz);
        fread(*out, 1, *sz, f);
        fclose(f);
        return (0);
    }

    return (-1);
}

int merge_sort(list_entry *head,int(*comp)(list_entry *l1,list_entry *l2,void *pv),void *pv)
{
    size_t group_size=1;

    if(!comp||!head||linked_list_empty(head)||linked_list_single(head)) //check if we have a head, if the list is empty or contains only one element
        return(-1);

    while(1)
    {
        list_entry thead= {0};
        list_entry *list_1=head->next; //get the first entry
        size_t merges=0;
        list_entry_init(&thead);

        while(list_1!=head)
        {
            list_entry *list_2=list_1;
            size_t h1_sz=0;
            size_t h2_sz=0;
            //get the second half of the list

            for(size_t i=0; i<group_size; i++)
            {
                h1_sz++;
                list_2=list_2->next;

                if(list_2==head) //reached the end
                    break;
            }

            h2_sz=group_size; //asssume the lists are equal
            merges++;

            while(h1_sz||(list_2!=head&&h2_sz))
            {
                list_entry *entry=NULL;

                if(h1_sz==0)
                {
                    entry=list_2;                       //the element comes from the second half
                    list_2=list_2->next;                //go to the next entry

                    if(h2_sz)
                        h2_sz--;
                }
                else if(h2_sz==0||list_2==head)
                {
                    entry=list_1;                       //the element comes from the first half
                    list_1=list_1->next;                //go to the next element

                    if(h1_sz)
                        h1_sz--;
                }
                else if(comp(list_1,list_2,pv)<=0)
                {
                    entry=list_1;                       //the item from the first list is smaller than the element from the second
                    list_1=list_1->next;                //advance

                    if(h1_sz)
                        h1_sz--;
                }
                else
                {
                    entry=list_2;                       //the element from the second list is smaller than the element from the first
                    list_2=list_2->next;                //advance

                    if(h2_sz)
                        h2_sz--;
                }

                linked_list_remove(entry);              //remove the entry from the list
                list_entry_init(entry);                 //reinitialize its "current" structure
                linked_list_add_last(entry,&thead);     //add it to the temporary head
            }

            list_1=list_2;                              //update the position
        }

        linked_list_replace(&thead,head);               //update the head

        if(merges<=1)
            break;
        else
            group_size*=2; //incerase the group number
    }

    return(0);
}
