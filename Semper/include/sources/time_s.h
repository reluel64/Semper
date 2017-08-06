#pragma  once

void time_init(void **spv,void *ip);
void time_reset(void *spv,void *ip);
double time_update(void *spv);
unsigned char *time_string(void *spv);
void time_destroy(void **spv);
