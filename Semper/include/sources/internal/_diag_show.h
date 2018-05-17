#pragma once
void diag_show_init(void **spv, void *ip);
void diag_show_reset(void *spv, void *ip);
double diag_show_update(void *spv);
unsigned char *diag_show_string(void *spv);
void diag_show_destroy(void **spv);
void diag_show_command(void *spv, unsigned char *comm);
