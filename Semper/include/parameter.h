#pragma once
#include <xpander.h>
#include <skeleton.h>
#include <objects/object.h>
double parameter_double(void* req, unsigned char* npm, double def, unsigned char xpander_flags);
long long parameter_long_long(void* req, unsigned char* npm, long long def, unsigned char xpander_flags);
size_t parameter_size_t(void* req, unsigned char* npm, size_t def, unsigned char xpander_flags);
unsigned char parameter_byte(void* req, unsigned char* npm, unsigned char def, unsigned char xpander_flags);
unsigned char parameter_bool(void* req, unsigned char* npm, unsigned char def, unsigned char xpander_flags);
unsigned char* parameter_string(void* req, unsigned char* npm, unsigned char* def, unsigned char xpander_flags);
unsigned int parameter_color(void* req, unsigned char* npm, unsigned int def, unsigned char xpander_flags);
int parameter_image_tile(void* req, unsigned char* npm, image_tile* param, unsigned char xpander_flags);
int parameter_object_padding(void* req, unsigned char* npm, object_padding* param, unsigned char xpander_flags);
int parameter_image_crop(void* req, unsigned char* npm, image_crop* param, unsigned char xpander_flags);
unsigned int parameter_self_scaling(void* req, unsigned char* npm, unsigned int def, unsigned char xpander_flags);
int parameter_color_matrix(void* req, unsigned char* npm, double* cm, unsigned char xpander_flags);
