/*Image cache SVG decoding
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Margarit
 * */
#include <stdio.h>
#include <image/image_cache.h>
#include <mem.h>
#include <stdint.h>
#include <3rdparty/nanosvg.h>
#include <3rdparty/nanosvgrast.h>
#define USE_SCALE 1
#if USE_SCALE
/* After some testing it seems that this value (8) brings the perfect balance between performance and quality
 * Thank you Gabi for the suggestion.
 * */
#define SCALE_FACTOR 8

#else
#define SCALE_FACTOR 1
#endif


int image_cache_decode_svg(FILE *f,image_cache_decoded *icd)
{
    int ret=0;
    fseeko64(f,0,SEEK_END);
    size_t buf_sz=ftello64(f);
    fseeko64(f,0,SEEK_SET);

    unsigned char *buf=zmalloc(buf_sz+1);
    fread(buf,buf_sz,1,f);

    NSVGimage *img=nsvgParse(buf,"px",0);

    if(img)
    {
        NSVGrasterizer *rast=nsvgCreateRasterizer();

        if(rast)
        {
            if(icd->width==0||icd->height==0)
            {
                icd->width=img->width;
                icd->height=img->height;
            }

            size_t big_w=icd->width*SCALE_FACTOR;
            size_t big_h=icd->height*SCALE_FACTOR;


            float wratio=(float)big_w/img->width;
            float hratio=(float)big_h/img->height;
            float fratio=wratio>hratio?wratio:hratio;

            icd->image_px=zmalloc(big_w*big_h*4);

            nsvgRasterize(rast,img,0,0,fratio,icd->image_px,big_w,big_h,big_w*4);
            nsvgDeleteRasterizer(rast);

            /*swap blue and red channels*/
            unsigned int *px=(unsigned int*)icd->image_px;

            for(size_t i=0; i<big_w*big_h; i++)
            {
                unsigned char *clswp=(unsigned char*)(px+i);
                unsigned char temp=clswp[2];
                clswp[2]=clswp[0];
                clswp[0]=temp;

                /*premultiply*/
                clswp[2]=((size_t)clswp[2]*(size_t)clswp[3])>>8;
                clswp[1]=((size_t)clswp[1]*(size_t)clswp[3])>>8;
                clswp[0]=((size_t)clswp[0]*(size_t)clswp[3])>>8;
            }

#if USE_SCALE
            int stride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,big_w);


            cairo_surface_t *crs=cairo_image_surface_create_for_data(icd->image_px,CAIRO_FORMAT_ARGB32,big_w,big_h,stride);

            cairo_surface_t *dest=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,icd->width,icd->height);
            cairo_t *ctx=cairo_create(dest);
            cairo_scale(ctx,(double)icd->width/(double)big_w,(double)icd->height/(double)big_h);

            cairo_set_source_surface (ctx, crs, 0, 0);
            cairo_pattern_set_filter (cairo_get_source (ctx), CAIRO_FILTER_BEST);

            cairo_set_antialias(ctx,CAIRO_ANTIALIAS_NONE);
            cairo_paint(ctx);
            cairo_destroy(ctx);
            cairo_surface_destroy(crs);

            sfree((void**)&icd->image_px);
            icd->image_px=zmalloc(icd->width*icd->height*4);
            unsigned char *pxx=cairo_image_surface_get_data(dest);
            memcpy(icd->image_px,pxx,icd->width*icd->height*4);
            cairo_surface_destroy(dest);
#endif
            ret=0;
        }

        nsvgDelete(img);
    }

    sfree((void**)&buf);


    return(ret);
}
