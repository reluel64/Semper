#pragma once
void script_reset(void *spv, void *ip);
void script_init(void **spv, void *ip);
double script_update(void *spv);
unsigned char *script_string(void *spv);
void script_command(void *spv, unsigned char *comm);
void script_destroy(void **spv);
