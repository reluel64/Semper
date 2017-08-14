#pragma once
void webget_init(void **spv,void *ip);
void webget_reset(void *spv,void *ip);
double webget_update(void *spv);
unsigned char *webget_string(void *spv);
void webget_destroy(void**spv);
void webget2_reset(void *spv,void *ip);
void webget2_init(void **spv,void *ip);
double webget2_update(void *spv);
unsigned char *webget2_string(void *spv);
void webget2_destroy(void **spv);
