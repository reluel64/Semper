#pragma once
void surface_info_init(void** spv, void* ip);
void surface_info_reset(void* spv, void* ip);
double surface_info_update(void* spv);
void surface_info_destroy(void** spv);

unsigned char* surface_info_string(void* spv);
