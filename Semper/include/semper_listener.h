#pragma once
#include <semper.h>
void *semper_listener_init(control_data *cd);
void semper_listener_destroy(void **p);
int semper_listener_writer(unsigned char *comm,size_t len);
