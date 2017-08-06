#pragma once
#include <stddef.h>
typedef void* section;
typedef void* key;

unsigned char* skeleton_get_section_name(section s);
key skeleton_next_key(key k, section s);
key skeleton_first_key(section s);
key skeleton_get_key(section s, unsigned char* kn);
key skeleton_get_key_n(section s, unsigned char* kn,size_t n);
section skeleton_next_section(section s, section shead);
section skeleton_first_section(section s);
section skeleton_get_section(section s, unsigned char* sn);
key skeleton_add_key(section s, unsigned char* kn, unsigned char* kv);
section skeleton_add_section(section s, unsigned char* sn);
unsigned char* skeleton_key_name(key k);
unsigned char* skeleton_key_value(key k);
void skeleton_key_remove(key* k);
int skeleton_destroy(section s);
void skeleton_remove_section(section* s);
