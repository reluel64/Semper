#pragma once
#include <stdlib.h>
#include <string.h>
#include <linked_list.h>
void* zmalloc(size_t bytes);
void sfree(void** p);
int put_file_in_memory(unsigned char* file, void** out, size_t* sz);
int merge_sort(list_entry *head,int(*comp)(list_entry *l1,list_entry *l2,void *pv),void *pv);
