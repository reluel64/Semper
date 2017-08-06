#pragma once
#include <stddef.h>

double surface_lister_update(void* spv);
void surface_lister_reset(void* spv, void* ip);
void surface_lister_init(void** spv, void* ip);
unsigned char* surface_lister_string(void* spv);
void surface_lister_command(void* spv, unsigned char* command);
void surface_lister_destroy(void** spv);
