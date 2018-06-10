#pragma once
void disk_space_init(void** spv, void* ip);
void disk_space_reset(void* spv, void* ip);
double disk_space_update(void* spv);
unsigned char* disk_space_string(void* spv);
void disk_space_destroy(void** spv);
