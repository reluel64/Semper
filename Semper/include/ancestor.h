#pragma once
#include <stddef.h>
#include <linked_list.h>




/*New*/

typedef struct
{
    list_entry current;
    section s;
} ancestor_queue;
void *ancestor_fusion(void *r, unsigned char *npm, unsigned char xpander_flags, unsigned char gq);
#define ancestor(r,npm,xpander_flags)         ((unsigned char*)ancestor_fusion((r),(npm), (xpander_flags),0))
#define ancestor_build_queue(r,xpander_flags) ((void*)         ancestor_fusion((r),(NULL),(xpander_flags),1))


int ancestor_destroy_queue(void **qh);
