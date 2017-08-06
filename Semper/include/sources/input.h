#pragma once
void input_init(void **spv,void *ip);
void input_reset(void *spv,void *ip);
void input_command(void *spv,unsigned char *comm);
void input_destroy(void **spv);
unsigned char *input_string(void *spv);
double input_update(void *spv);
