/*
*Image cache PNG routines
*Part of Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/
#include <image/image_cache.h>
#include <mem.h>
#include <cairo/cairo.h>

static cairo_status_t image_cache_png_read(void* pv, unsigned char* data, unsigned int length)
{
    return ((fread(data,1,length,pv)==length)?CAIRO_STATUS_SUCCESS:CAIRO_STATUS_READ_ERROR);
}

int image_cache_decode_png(FILE *fh, image_cache_decoded* icd)
{
    cairo_surface_t* surface = cairo_image_surface_create_from_png_stream(image_cache_png_read, (void*)fh);

    if(cairo_surface_status(surface)!=CAIRO_STATUS_SUCCESS)
    {
        cairo_surface_destroy(surface);
        return(-1);
    }

    unsigned char* buf = cairo_image_surface_get_data(surface);

    if(buf==NULL)
    {
        cairo_surface_destroy(surface);
        return(-1);
    }

    icd->height = cairo_image_surface_get_height(surface);
    icd->width = cairo_image_surface_get_width(surface);
    icd->image_px = zmalloc(icd->height * icd->width * sizeof(unsigned int));
    memcpy(icd->image_px, buf, icd->height * icd->width * sizeof(unsigned int));
    buf = NULL;
    cairo_surface_destroy(surface);

    return (0);
}
