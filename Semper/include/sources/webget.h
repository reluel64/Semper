#pragma once
void           webget_reset  (void *spv,void *ip);
void           webget_init   (void **spv,void *ip);
double         webget_update (void *spv);
unsigned char *webget_string (void *spv);
void           webget_destroy(void **spv);
