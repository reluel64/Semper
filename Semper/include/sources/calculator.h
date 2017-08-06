#pragma once
void calculator_init(void** spv, void* ip);
void calculator_reset(void* spv, void* ip);
double calculator_update(void* spv);
void calculator_destroy(void** spv);
