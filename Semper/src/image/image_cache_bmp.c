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
#pragma pack(push,1)
typedef struct
{
    unsigned short  bfType;
    unsigned int bfSize;
    unsigned short  bfReserved1;
    unsigned short  bfReserved2;
    unsigned int bfOffBits;
} bitmap_file_header;

typedef struct
{
    unsigned int biSize;
    long  biWidth;
    long  biHeight;
    unsigned short  biPlanes;
    unsigned short  biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    long  biXPelsPerMeter;
    long  biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} bitmap_info_header;
#pragma pack(pop)


/*This routine does the actual decoding
 * At the moment it only supports uncompressed bitmaps*/
int image_cache_decode_bmp(FILE *fh, image_cache_decoded* icd)
{

    bitmap_file_header bfh= {0};
    bitmap_info_header bih= {0};

    unsigned int *color_table=NULL;
    fread(&bfh,sizeof(bitmap_file_header),1,fh);
    fread(&bih,sizeof(bitmap_info_header),1,fh);

    if(bih.biCompression)
    {
        diag_error("%s Compressed bitmap is not supported. biCompression = %d",__FUNCTION__,bih.biCompression);
        return(-1);
    }

    switch(bih.biBitCount)
    {
    case 32:
    case 16:
    case 24:
    case 8:
    case 4:
    case 1:
        break;
    default:
        diag_error("%s Unsupported bit depth %d",__FUNCTION__,bih.biBitCount);
        return(-1);
    }
    bih.biWidth=labs(bih.biWidth);
    bih.biHeight=labs(bih.biHeight);
    icd->image_px = zmalloc(bih.biWidth * bih.biHeight * sizeof(unsigned int)); // the image can be alpha adjusted

    icd->width=bih.biWidth;
    icd->height=bih.biHeight;

    if(bih.biBitCount<=8)
    {
        size_t color_table_sz =(bfh.bfOffBits-bih.biSize);
        size_t color_table_start=sizeof(bitmap_file_header)+bih.biSize;

        fseek(fh,color_table_start,SEEK_SET);
        color_table=zmalloc(color_table_sz);
        fread(color_table,1,color_table_sz,fh);
    }

    fseek(fh,bfh.bfOffBits,SEEK_SET);


    //////////////////////////////////////////////////////////////////////////////////////////
    switch(bih.biBitCount)
    {
    case 32:
        fread(icd->image_px,sizeof(unsigned int),bih.biHeight *  bih.biWidth,fh);
        break;
    case 24:
        for(long h = 0; h < bih.biHeight; h++)
        {
            for(long w = 0; w < bih.biWidth; w++)
            {
                fread(((unsigned int*)icd->image_px + h * bih.biWidth+w),sizeof(unsigned char),3,fh);
                ((unsigned int*)icd->image_px) [(h * bih.biWidth) + w] |= 0xff000000;
            }
            fseek(fh,(bih.biWidth % 4),SEEK_CUR);
        }
        break;
    case 16:
        for(size_t h = 0; h < (size_t)bih.biHeight; h++)
        {
            for(size_t w = 0; w < (size_t)bih.biWidth; w++)
            {
                unsigned short px=0;
                fread(&px,sizeof(px),1,fh);
                *((unsigned int*)icd->image_px + h * bih.biWidth + w) = (px&0xf) | ((px&0xf0) << 4) | ((px & 0xf00) << 8) | ((px & 0xf000) << 12);
            }
            fseek(fh,(bih.biWidth % 2),SEEK_CUR);
        }
        break;
    case 8:
    {
        for(size_t h = 0; h < (size_t)bih.biHeight; h++)
        {
            for(size_t w = 0; w < (size_t)bih.biWidth; w++)
            {
                unsigned char px=0;
                fread(&px,sizeof(px),1,fh);
                *((unsigned int*)icd->image_px + h * bih.biWidth + w) =color_table[px]|0xff000000;
            }
            if((bih.biWidth % 4))
                fseek(fh,4 - (bih.biWidth % 4),SEEK_CUR);
        }
        break;
    }
    case 4:
        for(size_t h = 0; h < (size_t)bih.biHeight; h++)
        {

            for(size_t w = 0; w < (size_t)bih.biWidth; w+=2)
            {
                unsigned char px=0;
                fread(&px,sizeof(px),1,fh);
                *((unsigned int*)icd->image_px + h * bih.biWidth + w)   = color_table[px>>4] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+1) = color_table[px&0xf]| 0xff000000;
            }

            if((bih.biWidth%8==0)||(bih.biWidth%8>6))
                fseek(fh,0,SEEK_CUR);
            else if(bih.biWidth % 8<=2)
                fseek(fh,3,SEEK_CUR);
            else if( bih.biWidth % 8 <= 4)
                fseek(fh,2,SEEK_CUR);
            else
                fseek(fh,1,SEEK_CUR);

        }
        break;
    case 1:
        for(size_t h = 0; h < (size_t)bih.biHeight; h++)
        {

            for(size_t w = 0; w < (size_t)bih.biWidth; w+=8)
            {
                unsigned char px=0;
                fread(&px,sizeof(px),1,fh);
                *((unsigned int*)icd->image_px + h * bih.biWidth + w)   =  color_table[(px&0x80)>>7] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+1) =  color_table[(px&0x40)>>6] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+2) =  color_table[(px&0x20)>>5] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+3) =  color_table[(px&0x10)>>4] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+4) =  color_table[(px&0x08)>>3] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+5) =  color_table[(px&0x04)>>2] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+6) =  color_table[(px&0x02)>>1] | 0xff000000;
                *((unsigned int*)icd->image_px + h * bih.biWidth + w+7) =  color_table[(px&0x01)>>0] | 0xff000000;
            }

            if((bih.biWidth%32==0)||(bih.biWidth%32>24))
                fseek(fh,0,SEEK_CUR);
            else if(bih.biWidth % 32<=8)
                fseek(fh,3,SEEK_CUR);
            else if( bih.biWidth % 32 <= 16)
                fseek(fh,2,SEEK_CUR);
            else
                fseek(fh,1,SEEK_CUR);

        }
        break;
    }


    sfree((void**)&color_table);

    /*Flip the image vertically*/
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

    return (0);
}
