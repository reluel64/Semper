#pragma once
void spawner_init(void **spv,void *ip);
void spawner_reset(void *spv, void *ip);
double spawner_update(void *spv);
unsigned char *spawner_string(void *spv);
void spawner_command(void *spv, unsigned char *comm);
void spawner_destroy(void **spv);
