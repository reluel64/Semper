#pragma once
double processor_update(void* spv);
void processor_reset(void* spv, void* ip);
void processor_init(void** spv, void* ip);
void processor_destroy(void** spv);
