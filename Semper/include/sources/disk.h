#pragma once
void disk_init(void** spv, void* ip);
void disk_reset(void* spv, void* ip);
double disk_update(void* spv);
unsigned char* disk_string(void* spv);
void disk_destroy(void** spv);
