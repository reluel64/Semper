#pragma once
void network_destroy(void **spv);
double network_update(void *spv);
void network_reset(void *spv, void *ip);
void network_init(void**spv, void *ip);
