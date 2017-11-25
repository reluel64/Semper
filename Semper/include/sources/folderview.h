#pragma once
void folderview_init(void **spv, void *ip);
void folderview_reset(void *spv, void *ip);
double folderview_update(void *spv);
void folderview_command(void* spv, unsigned char* command);
unsigned char *folderview_string(void* spv);
void folderview_destroy(void **spv);
