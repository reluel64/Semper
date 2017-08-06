#pragma once
#include <stdio.h>
#include <semper.h>
#include <cairo/cairo.h>
#include <linked_list.h>
/*New caching mechanism*/
#define COLOR_MATRIX_SIZE 25

typedef struct
{
    long x;
    long y;
    long w;
    long h;
    unsigned char origin;
} image_crop;

typedef struct
{
    long w;
    long h;
} image_tile;

typedef struct
{
    unsigned char* px;
    long w;
    long h;
    void* ie;
} image_info;

typedef struct
{
    /*Cache requests*/
    unsigned char ref;
    size_t width;
    size_t height;
    unsigned char* path;
    unsigned char reserved; /*it is mandatory to have this here to simplify cache hit*/
    size_t rw;
    size_t rh;
    unsigned char tile; // request
    unsigned char opacity;
    unsigned char flip;
    unsigned char rgb_bgr;
    unsigned char inv;
    unsigned char grayscale;
    unsigned char keep_ratio;
    unsigned int tint_color;
    double cm[COLOR_MATRIX_SIZE];
    image_crop cpm; // request
    image_tile tpm; // request

} image_attributes;

typedef struct _image_entry
{
    unsigned char* image_px;
    semper_timestamp st;
    image_attributes attrib;
    list_entry current; // this entry
    size_t refs;

} image_entry;

typedef struct _image_cache
{
    size_t flush_interval;
    size_t validity;
    list_entry images;
    void *cd;
} image_cache;

typedef struct _image_cache_decoded
{
    unsigned char* image_px;
    size_t width;
    size_t height;

} image_cache_decoded;

int image_cache_init(control_data *cd);
int image_cache_cleanup(void* ic);
void image_cache_destroy(void** ic);
void image_cache_image_parameters(void* r, image_attributes* ia, unsigned char flags, char* pre);
int image_cache_query_image(void *ic,image_attributes *ia,unsigned char **px,long width,long height);
void image_cache_unref_image(void *ic,image_attributes *ia,unsigned char riz);
