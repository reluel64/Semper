/*Image cache SVG decoding
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Margarit
 * */
#define NANOSVGRAST_IMPLEMENTATION
#define NANOSVG_IMPLEMENTATION
#include <stdio.h>
#include <image/image_cache.h>
#include <mem.h>
#include <stdint.h>
#include <3rdparty/nanosvg.h>
#include <3rdparty/nanosvgrast.h>
#include <cairo.h>
#define USE_SCALE 1
#if USE_SCALE
/* After some testing it seems that this value (8) brings the perfect balance between performance and quality
 * Thank you Gabi for the suggestion.
 * */
#define SCALE_FACTOR 2

#else
#define SCALE_FACTOR 1
#endif

#if 0
#define cairo_set_color(cr,color) \
    { \
        double alpha=((double)(((color)&0xff000000)>>24)) /255.0; \
        double red=  ((double)(((color)&0x00ff0000)>>16)) /255.0; \
        double green=((double)(((color)&0x0000ff00)>>8))  /255.0; \
        double blue= ((double)((color)&0x000000ff))       /255.0; \
        cairo_set_source_rgba((cr),red,green,blue,alpha); \
    }


static cairo_pattern_t *image_cache_decode_svg_pattern(NSVGpaint *paint, double opacity)
{
    cairo_pattern_t *pat = NULL;
    cairo_matrix_t mtx = {0};

    if(paint->type == NSVG_PAINT_RADIAL_GRADIENT)
    {
        pat = cairo_pattern_create_radial(0.0, 0.0, 0.0, paint->gradient->fx, paint->gradient->fy, 1.0);
    }
    else
    {
        pat = cairo_pattern_create_linear(0.0, 0.0, 1.0, 1.0);
    }

    switch(paint->gradient->spread)
    {
        case NSVG_SPREAD_PAD:
            cairo_pattern_set_extend(pat, CAIRO_EXTEND_PAD);
            break;

        case NSVG_SPREAD_REFLECT:
            cairo_pattern_set_extend(pat, CAIRO_EXTEND_REFLECT);
            break;

        case NSVG_SPREAD_REPEAT:
            cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
            break;
    }


    mtx.xx = paint->gradient->xform[0];
    mtx.yx = paint->gradient->xform[1];
    mtx.xy = paint->gradient->xform[2];
    mtx.yy = paint->gradient->xform[3];
    mtx.x0 = paint->gradient->xform[4];
    mtx.y0 = paint->gradient->xform[5];
    cairo_pattern_set_matrix(pat, &mtx);

    for(int stops = 0; stops < paint->gradient->nstops; stops++)
    {
        double offset = paint->gradient->stops[stops].offset;
        unsigned int fpx = paint->gradient->stops[stops].color;
        unsigned char tmp = 0;
        unsigned char *clswp = &fpx;
        tmp = clswp[0];
        clswp[0] = clswp[2];
        clswp[2] = tmp;
        clswp[3] = (double)clswp[3] * opacity;
        clswp[2] = ((size_t)clswp[2] * (size_t)clswp[3]) >> 8;
        clswp[1] = ((size_t)clswp[1] * (size_t)clswp[3]) >> 8;
        clswp[0] = ((size_t)clswp[0] * (size_t)clswp[3]) >> 8;
        cairo_pattern_add_color_stop_rgba(pat, offset, clswp[2] / 255.0, clswp[1] / 255.0, clswp[0] / 255.0, clswp[3] / 255.0);
    }

    return(pat);
}

int image_cache_decode_svg(FILE *f, image_cache_decoded *icd)
{
    int ret = 0;
    fseeko64(f, 0, SEEK_END);
    size_t buf_sz = ftello64(f);
    fseeko64(f, 0, SEEK_SET);

    unsigned char *buf = zmalloc(buf_sz + 1);
    fread(buf, buf_sz, 1, f);


    NSVGimage *img = nsvgParse(buf, "px", 96.0);

    if(img)
    {
        cairo_surface_t *surf = NULL;
        cairo_t *cr = NULL;
        double rw = 1.0;
        double rh = 1.0;
        unsigned char *px = NULL;

        if(icd->width == 0 || icd->height == 0)
        {
            icd->width = img->width;
            icd->height = img->height;
        }

        rw = ((double)icd->width / (double)img->width);
        rh = ((double)icd->height / (double)img->height);
        icd->width = (double)img->width * rw;
        icd->height = (double)img->height * rh;
        surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, icd->width, icd->height);
        cr = cairo_create(surf);

        // cairo_translate(cr,(double)img->width/2.0,(double)img->height/2.0);
        cairo_scale(cr, rw, rh);
        // cairo_translate(cr,(double)-img->width/2.0,(double)-img->height/2.0);

        for(NSVGshape *shape = img->shapes; shape != NULL; shape = shape->next)
        {
            cairo_save(cr);
            cairo_new_path(cr);
            NSVGpaint *fill = &shape->fill;
            NSVGpaint *stroke = &shape->stroke;
            double dashes[8] = {0};
            dashes[0] = shape->strokeDashArray[0];
            dashes[1] = shape->strokeDashArray[1];
            dashes[2] = shape->strokeDashArray[2];
            dashes[3] = shape->strokeDashArray[3];
            dashes[4] = shape->strokeDashArray[4];
            dashes[5] = shape->strokeDashArray[5];
            dashes[6] = shape->strokeDashArray[6];
            dashes[7] = shape->strokeDashArray[7];

            cairo_set_line_width(cr, shape->strokeWidth);
            cairo_set_dash(cr, dashes, shape->strokeDashCount, shape->strokeDashOffset);

            switch(shape->strokeLineJoin)
            {
                case NSVG_JOIN_MITER :
                    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
                    break;

                case NSVG_JOIN_ROUND:
                    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

                case NSVG_JOIN_BEVEL:
                    cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
                    break;
            }

            switch(shape->strokeLineCap)
            {
                case NSVG_CAP_BUTT :
                    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
                    break;

                case NSVG_CAP_ROUND:
                    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                    break;

                case NSVG_CAP_SQUARE:
                    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
                    break;
            }

            switch(shape->fillRule)
            {
                case NSVG_FILLRULE_EVENODD:
                    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
                    break;

                case NSVG_FILLRULE_NONZERO:
                    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
                    break;
            }

            cairo_set_miter_limit(cr, shape->miterLimit);

            if(!(shape->flags & NSVG_FLAGS_VISIBLE))
                continue;




            for(NSVGpath *path = shape->paths; path != NULL; path = path->next)
            {

                for(int i = 0; i < path->npts - 1; i += 3)
                {
                    float* p = &path->pts[i * 2];
                    cairo_move_to(cr, p[0], p[1]);
                    cairo_curve_to(cr, p[2], p[3], p[4], p[5], p[6], p[7]);

                    if(path->closed)
                    {
                        cairo_close_path(cr);
                    }
                }

            }

            unsigned int fpx = fill->color;
            unsigned int spx = stroke->color;
            unsigned char tmp = 0;

            if(fill->type == NSVG_PAINT_COLOR)
            {
                unsigned char *clswp = &fpx;
                tmp = clswp[0];
                clswp[0] = clswp[2];
                clswp[2] = tmp;
                clswp[3] = (double)clswp[3] * shape->opacity;
                clswp[2] = ((size_t)clswp[2] * (size_t)clswp[3]) >> 8;
                clswp[1] = ((size_t)clswp[1] * (size_t)clswp[3]) >> 8;
                clswp[0] = ((size_t)clswp[0] * (size_t)clswp[3]) >> 8;
                cairo_set_color(cr, fpx);
                cairo_fill_preserve(cr);
            }
            else if(fill->type >= NSVG_PAINT_LINEAR_GRADIENT)
            {

                cairo_pattern_t *pat = image_cache_decode_svg_pattern(fill, shape->opacity);
                cairo_set_source(cr, pat);
                cairo_fill_preserve(cr);
                cairo_pattern_destroy(pat);
            }
            else
            {
                printf("Fill type %d\n", fill->type);
            }

            if(stroke->type >= NSVG_PAINT_COLOR)
            {
                unsigned char *clswp = &spx;
                tmp = clswp[0];
                clswp[0] = clswp[2];
                clswp[2] = tmp;
                clswp[3] = (double)clswp[3] * shape->opacity;
                clswp[2] = ((size_t)clswp[2] * (size_t)clswp[3]) >> 8;
                clswp[1] = ((size_t)clswp[1] * (size_t)clswp[3]) >> 8;
                clswp[0] = ((size_t)clswp[0] * (size_t)clswp[3]) >> 8;

                cairo_set_color(cr, spx);
                cairo_stroke_preserve(cr);
            }
            else if(stroke->type >= NSVG_PAINT_LINEAR_GRADIENT)
            {
                cairo_pattern_t *pat = image_cache_decode_svg_pattern(stroke, shape->opacity);
                cairo_set_source(cr, pat);
                cairo_stroke_preserve(cr);
                cairo_pattern_destroy(pat);
            }

            cairo_restore(cr);
        }

        nsvgDelete(img);
        sfree((void**)&buf);

        cairo_destroy(cr);
        icd->image_px = zmalloc(icd->width * icd->height * sizeof(unsigned int));
        px = cairo_image_surface_get_data(surf);
        memcpy(icd->image_px, px, icd->width * icd->height * sizeof(unsigned int));
        cairo_surface_destroy(surf);
        return(ret);
    }
}
#endif
# if 1
int image_cache_decode_svg(FILE *f, image_cache_decoded *icd)
{
    int ret = 0;
    fseeko64(f, 0, SEEK_END);
    size_t buf_sz = ftello64(f);
    fseeko64(f, 0, SEEK_SET);

    unsigned char *buf = zmalloc(buf_sz + 1);
    fread(buf, buf_sz, 1, f);


    NSVGimage *img = nsvgParse(buf, "px", 0);

    if(img)
    {
        NSVGrasterizer *rast = nsvgCreateRasterizer();

        if(rast)
        {
            if(icd->width == 0 || icd->height == 0)
            {
                icd->width = img->width;
                icd->height = img->height;
            }

            size_t big_w = icd->width * SCALE_FACTOR;
            size_t big_h = icd->height * SCALE_FACTOR;


            float wratio = (float)big_w / img->width;
            float hratio = (float)big_h / img->height;
            float fratio = wratio > hratio ? wratio : hratio;

            icd->image_px = zmalloc(big_w * big_h * 4);

            nsvgRasterize(rast, img, 0, 0, fratio, icd->image_px, big_w, big_h, big_w * 4);
            nsvgDeleteRasterizer(rast);

            /*swap blue and red channels*/
            unsigned int *px = (unsigned int*)icd->image_px;

            for(size_t i = 0; i < big_w * big_h; i++)
            {
                unsigned char *clswp = (unsigned char*)(px + i);
                unsigned char temp = clswp[2];
                clswp[2] = clswp[0];
                clswp[0] = temp;

                /*premultiply*/
                clswp[2] = ((size_t)clswp[2] * (size_t)clswp[3]) >> 8;
                clswp[1] = ((size_t)clswp[1] * (size_t)clswp[3]) >> 8;
                clswp[0] = ((size_t)clswp[0] * (size_t)clswp[3]) >> 8;
            }

#if USE_SCALE
            int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, big_w);


            cairo_surface_t *crs = cairo_image_surface_create_for_data(icd->image_px, CAIRO_FORMAT_ARGB32, big_w, big_h, stride);

            cairo_surface_t *dest = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, icd->width, icd->height);
            cairo_t *ctx = cairo_create(dest);
            cairo_scale(ctx, (double)icd->width / (double)big_w, (double)icd->height / (double)big_h);

            cairo_set_source_surface(ctx, crs, 0, 0);
            cairo_pattern_set_filter(cairo_get_source(ctx), CAIRO_FILTER_BEST);

            cairo_set_antialias(ctx, CAIRO_ANTIALIAS_NONE);
            cairo_paint(ctx);
            cairo_destroy(ctx);
            cairo_surface_destroy(crs);

            sfree((void**)&icd->image_px);
            icd->image_px = zmalloc(icd->width * icd->height * 4);
            unsigned char *pxx = cairo_image_surface_get_data(dest);
            memcpy(icd->image_px, pxx, icd->width * icd->height * 4);
            cairo_surface_destroy(dest);
#endif
            ret = 0;
        }

        nsvgDelete(img);
    }

    sfree((void**)&buf);


    return(ret);
}
#endif
