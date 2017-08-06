/*
*Image cache BMP routines
*Part of Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/


#include <stdint.h>
#include <image/image_cache.h>
#include <string.h>
#include <mem.h>
#include <time.h>

static void image_cache_bmp_flip(image_cache_decoded* icd)
{
    unsigned int* w_temp_buf = (((unsigned int*)(icd->image_px)) + (icd->width * (icd->height - 1)));
    for(size_t h = 0; h < icd->height / 2; h++)
    {
        for(size_t w = 0; w < icd->width; w++)
        {
            unsigned int bpx = *(((unsigned int*)(icd->image_px)) + (h * icd->width) + w);
            unsigned int tpx = *(w_temp_buf - (h * icd->width) + w);

            *(w_temp_buf - (h * icd->width) + w) = bpx;
            *(((unsigned int*)(icd->image_px)) + (h * icd->width) + w) = tpx;
        }
    }
}

int image_cache_decode_bmp(FILE *fh, image_cache_decoded* icd)
{
    /*we are not using the bitmap specific structures due to structure padding issues*/

    unsigned int offset_to_data=0;
    int width = 0;
    int height = 0;
    unsigned short bit_count = 0;
    fseek(fh,10,SEEK_SET);
    fread(&offset_to_data,sizeof(unsigned int),1,fh);

    fseek(fh,18,SEEK_SET);
    fread(&width,sizeof(int),1,fh);
    fread(&height,sizeof(int),1,fh);
    fseek(fh,28,SEEK_SET);
    fread(&bit_count,sizeof(unsigned short),1,fh);


    icd->height = height;
    icd->width = width;

    icd->image_px = zmalloc(height * width * sizeof(unsigned int)); // the image can be alpha adjusted
    //fseek(fh,offset_to_data,SEEK_SET);


    //////////////////////////////////////////////////////////////////////////////////////////

    if(bit_count == 32)
    {
        fread(icd->image_px,sizeof(unsigned int),height * width,fh);
    }
    else if(bit_count == 24)
    {
        for(long h = 0; h < height; h++)
        {
            for(long w = 0; w < width; w++)
            {
                fread(((unsigned int*)icd->image_px + h * width+w),sizeof(unsigned char),3,fh);
                ((unsigned int*)icd->image_px) [(h * width) + w] |= 0xff000000;
            }
            fseek(fh,(width % sizeof(unsigned int)),SEEK_CUR);
        }
    }

    else if(bit_count == 16)
    {
        for(size_t h = 0; h < (size_t)height; h++)
        {
            for(size_t w = 0; w < (size_t)width; w++)
            {

                unsigned long px=0;

                fread(&px,sizeof(px),1,fh);

                unsigned int red = (px & 0xf00) >> 8;
                unsigned int green = (px & 0xf0) >> 4;
                unsigned int blue = (px & 0xf);
                unsigned int alpha = (px & 0xf000) >> 12;
                *((unsigned int*)((icd->image_px + h * width + w)) + 0) =
                    blue | (green << 8) | (red << 16) | (alpha << 24);
            }
        }
    }
    image_cache_bmp_flip(icd);
    return (0);
}
