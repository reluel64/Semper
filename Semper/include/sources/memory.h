#pragma once
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#endif

void memory_init(void** spv, void* ip);
void memory_reset(void* spv, void* ip);
double memory_update(void* spv);
void memory_destroy(void** spv);
