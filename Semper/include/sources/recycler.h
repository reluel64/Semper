#pragma once
void recycler_init(void **spv, void *ip);
void recycler_reset(void *spv, void *ip);
double recycler_update(void *spv);
void recycler_command(void *spv, unsigned char *cmd);
void recycler_destroy(void **spv);
