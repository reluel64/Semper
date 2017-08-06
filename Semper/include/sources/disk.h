#pragma once
typedef struct _disk
{
    unsigned char* name;
    unsigned char type;
    unsigned char label;
    void* free_bytes;
    void* total_bytes;
    unsigned char total;
    unsigned char* ret_str;
} disk;

void disk_create(void** spv, void* ip);
void disk_reset(void* spv, void* ip);
double disk_update(void* spv);
unsigned char* disk_string(void* spv);
void disk_destroy(void** spv);
