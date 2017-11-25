/*
*Image cache JPEG routines
*Part of Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/

#include <mem.h>
#include <stdio.h>
#include <jpeglib.h>
#include <image/image_cache.h>

int image_cache_decode_jpeg(FILE *fh, image_cache_decoded* icd)
{
    struct jpeg_decompress_struct decomps = { 0 };
    struct jpeg_error_mgr decomperr = { 0 };
    decomps.err = jpeg_std_error(&decomperr);
    jpeg_create_decompress(&decomps);
    jpeg_stdio_src(&decomps, fh);

    if(!jpeg_read_header(&decomps, 1))
    {
        jpeg_destroy_decompress(&decomps);
        return (-1);
    }

    if(!jpeg_start_decompress(&decomps))
    {
        jpeg_destroy_decompress(&decomps);
        return (-1);
    }

    icd->height = decomps.output_height;
    icd->width = decomps.output_width;
    unsigned char* tbuf = zmalloc(icd->height * icd->width * decomps.output_components);
    size_t pixel = decomps.output_components;

    while(decomps.output_scanline < decomps.output_height)
    {
        unsigned char* start_px = (tbuf + (decomps.output_width * pixel) * decomps.output_scanline);
        jpeg_read_scanlines(&decomps, &start_px, 1);
    }

    jpeg_finish_decompress(&decomps);
    jpeg_destroy_decompress(&decomps);

    icd->image_px = zmalloc(icd->height * icd->width * 4);

    /*Add alpha channel*/
    for(size_t i = 0; i < icd->height * icd->width; i++)
    {
        unsigned char* tb = tbuf + (i * pixel);
        ((unsigned int*)icd->image_px)[i] = 0xff000000 | ((unsigned long)tb[0] << 16) | ((unsigned long)tb[1] << 8) | ((unsigned long)tb[2]);
    }

    sfree((void**)&tbuf);
    return (0);
}
