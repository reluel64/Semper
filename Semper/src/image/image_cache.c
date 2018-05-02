/*
* Image caching
* Part of Project 'Semper'
* Written by Alexandru-Daniel Mărgărit
*/
#ifdef WIN32
#include <windows.h>
#endif
#include <image/image_cache.h>

#include <stdio.h>
#include <mem.h>
#include <string_util.h>
#include <surface.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <parameter.h>
#include <semper.h>
extern int image_cache_decode_bmp(FILE *fh, image_cache_decoded* icd);
extern int image_cache_decode_png(FILE *fh, image_cache_decoded* icd);
extern int image_cache_decode_jpeg(FILE *fh, image_cache_decoded* icd);
extern int image_cache_decode_svg(FILE *f,image_cache_decoded *icd);
static void image_cache_remove_entry(image_entry** ie);

typedef enum
{
    none,
    bitmap,
    jpeg,
    png

} image_types;


static int image_cache_adjust_color_matrix(image_cache_decoded* icd, image_attributes *ia)
{

    /*Alter the color matrix*/
    static double matrix[5][5] = { 0 };
    unsigned char* tint_color_buf = (unsigned char*)&ia->tint_color;
    memcpy(matrix, &ia->cm, sizeof(matrix));

    matrix[0][0] *= (tint_color_buf[2] / 255.0);
    matrix[1][1] *= (tint_color_buf[1] / 255.0);
    matrix[2][2] *= (tint_color_buf[0] / 255.0);
    matrix[3][3] *= (tint_color_buf[3] / 255.0);

    /*Do grayscale and/or color inversion*/
    if(ia->grayscale||ia->inv)
    {
        for(unsigned char i = 0; i < 3; i++)
        {
            for(unsigned char j = 0; j < 3; j++)
            {
                if(ia->grayscale)
                {
                    matrix[i][j] *= 0.33;

                    if(matrix[i][j] == 0.0)
                    {
                        matrix[i][j] = 0.33;
                    }

                    if(ia->inv)
                    {
                        matrix[i][j] *= -1.0;
                    }
                }
            }
        }

        if(ia->inv)
        {
            matrix[4][0] = 1.0;
            matrix[4][1] = 1.0;
            matrix[4][2] = 1.0;
        }
    }

    /*Swap Red and Blue*/
    if(ia->rgb_bgr)
    {
        double temp = matrix[0][0];
        matrix[0][0] = matrix[2][2];
        matrix[2][2] = temp;
    }

    /*Apply the color matrix*/

    for(size_t h = 0; h < icd->height; h++)
    {
        for(size_t w = 0; w < icd->width; w++)
        {
            unsigned int px = ((unsigned int*)icd->image_px)[icd->width * h + w];
            double a = (double)((px & 0xff000000) >> 24);
            double r = (double)((px & 0x00ff0000) >> 16);
            double g = (double)((px & 0x0000ff00) >> 8);
            double b = (double)(unsigned char)px;

            unsigned int channels[4] = { 0 };

            for(unsigned char i = 0; i < 4; i++)
            {
                double res =   matrix[0][i] * r +
                               matrix[1][i] * g +
                               matrix[2][i] * b +
                               matrix[3][i] * a +
                               matrix[4][i] * 255.0;

                if(res>255.0)
                    channels[i]=255.0;

                else if(res<0.0)
                    channels[i]=0.0;

                else
                    channels[i]=res;
            }

            ((unsigned char*)&px)[0] = (channels[2] * channels[3]) >> 8;
            ((unsigned char*)&px)[1] = (channels[1] * channels[3]) >> 8;
            ((unsigned char*)&px)[2] = (channels[0] * channels[3]) >> 8;
            ((unsigned char*)&px)[3] = (channels[3]);                       //just to make things tidy
            ((unsigned int*)icd->image_px)[icd->width * h + w] = px;
        }
    }

    return (1);
}

static void image_cache_flip(image_cache_decoded* icd, int flip)
{
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, icd->width);
    cairo_surface_t* image =  cairo_image_surface_create_for_data(icd->image_px, CAIRO_FORMAT_ARGB32, icd->width, icd->height, stride);
    cairo_surface_t* timage = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, icd->width, icd->height);
    cairo_t* cr = cairo_create(timage);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    switch(flip)
    {
    case 1:
        cairo_translate(cr, icd->width, 0.0);
        cairo_scale(cr, -1.0, 1.0);
        break;

    case 2:
        cairo_translate(cr, 0.0, icd->height);
        cairo_scale(cr, 1.0, -1.0);
        break;

    case 3:
        cairo_translate(cr, (double)icd->width, (double)icd->height);
        cairo_scale(cr, -1.0, -1.0);
        break;
    }

    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
    sfree((void**)&icd->image_px);
    icd->image_px = zmalloc((size_t)labs(icd->width) * (size_t)labs(icd->height) * 4);
    unsigned char* img_buf = cairo_image_surface_get_data(timage);
    memcpy(icd->image_px, img_buf, (size_t)labs(icd->width) * (size_t)labs(icd->height) * 4);
    cairo_surface_destroy(timage);
    cairo_surface_destroy(image);
    cairo_destroy(cr);
}

static int image_cache_adjust_opacity(image_cache_decoded* icd, unsigned char opacity)
{
    if(icd == NULL || icd->width == 0 || icd->height == 0 || opacity == 255)
    {
        return (-1);
    }

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, icd->width);

    cairo_surface_t* image = cairo_image_surface_create_for_data(icd->image_px, CAIRO_FORMAT_ARGB32, icd->width, icd->height, stride);

    cairo_surface_t* timage = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, icd->width, icd->height);

    cairo_t* cr = cairo_create(timage);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, image, 0.0, 0.0);
    cairo_paint_with_alpha(cr, (double)opacity / 255.0);

    cairo_destroy(cr);
    cairo_surface_destroy(image);
    sfree((void**)&icd->image_px);
    icd->image_px = zmalloc((size_t)labs(icd->width) * (size_t)labs(icd->height) * 4);
    unsigned char* img_buf = cairo_image_surface_get_data(timage);
    memcpy(icd->image_px, img_buf, (size_t)labs(icd->width) * (size_t)labs(icd->height) * 4);
    cairo_surface_destroy(timage);
    return (0);
}

static int image_cache_tile(image_cache_decoded* icd, long tw, long th)
{
    if(icd == NULL || icd->width == 0 || icd->height == 0)
    {
        return (-1);
    }

    float nh = fmax((float)th, (float)icd->height);
    float nw = fmax((float)tw, (float)icd->width);
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, icd->width);

    cairo_surface_t* image = cairo_image_surface_create_for_data(icd->image_px, CAIRO_FORMAT_ARGB32, icd->width, icd->height, stride);
    cairo_surface_t* timage = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, nw, nh);
    cairo_pattern_t* pattern_tile = cairo_pattern_create_for_surface(image);
    cairo_t* cr = cairo_create(timage);
    cairo_matrix_t matrix = { 0 };

    if(cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
    {
        cairo_surface_destroy(image);
        return (-1);
    }

    cairo_pattern_set_extend(pattern_tile, CAIRO_EXTEND_REPEAT);
    cairo_matrix_init_scale(&matrix, 1, 1);
    cairo_pattern_set_matrix(pattern_tile, &matrix);

    cairo_set_source(cr, pattern_tile);
    cairo_paint(cr);
    cairo_pattern_destroy(pattern_tile);
    cairo_destroy(cr);
    cairo_surface_destroy(image);

    icd->width = nw;
    icd->height = nh;
    sfree((void**)&icd->image_px);
    icd->image_px = zmalloc((size_t)nw * (size_t)nh * 4);
    unsigned char* img_buf = cairo_image_surface_get_data(timage);
    memcpy(icd->image_px, img_buf, (size_t)nw * (size_t)nh * 4);
    cairo_surface_destroy(timage);
    return (0);
}

static int image_cache_scale(image_cache_decoded* icd, long nw, long nh,unsigned char keep_ratio)
{

    if(icd == NULL || icd->width == 0 || icd->height == 0     ||
            (icd->width == (size_t)nw && icd->height == (size_t)nh) ||
            (nw <= 0&&keep_ratio==0) || (keep_ratio==0&&nh <= 0))
    {
        return (-1);
    }

    if(keep_ratio)
    {
        nh=nh>0?nh:(long)icd->height;
        nw=nw>0?nw:(long)icd->width;
        double ratio=(double)icd->width/(double)icd->height;
        double rs_ratio=(double)nw/(double)nh;

        if(ratio<rs_ratio)
        {
            nh=nw*(double)icd->height/(double)icd->width;
        }
        else
        {
            nw=nh*(double)icd->width/(double)icd->height;
        }
    }

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, icd->width);

    cairo_surface_t* image = cairo_image_surface_create_for_data(icd->image_px, CAIRO_FORMAT_ARGB32, icd->width, icd->height, stride); /*original image*/

    cairo_surface_t* simage =  cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (double)abs(nw), (double)abs(nh)); /*Scaled image*/

    cairo_t* cr = cairo_create(simage);

    cairo_scale(cr, nw / (double)icd->width, nh / (double)icd->height);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(image);

    icd->width = nw;
    icd->height = nh;

    sfree((void**)&icd->image_px);
    icd->image_px = zmalloc((size_t)labs(nw) * (size_t)labs(nh) * 4);
    unsigned char* img_buf = cairo_image_surface_get_data(simage);
    memcpy(icd->image_px, img_buf, (size_t)labs(nw) * (size_t)labs(nh) * 4);
    cairo_surface_destroy(simage);

    return (0);
}

static int image_cache_crop(image_cache_decoded* icd, image_crop* itcd)
{
    static image_crop null_itcd = { 0 };

    if(icd == NULL || icd->width == 0 || icd->height == 0 || itcd == NULL ||!memcmp(&null_itcd, itcd, sizeof(image_crop)))
    {
        return (-1);
    }

    long cropx = 0;
    long cropy = 0;
    long ch = itcd->h;
    long cw = itcd->w;
    long cx = itcd->x;
    long cy = itcd->y;

    switch(itcd->origin)
    {
    case 1:
        cropx = cx;
        cropy = cy;
        break;

    case 2:
        cropx = (icd->width) - cx - cw;
        cropy = cy;
        break;

    case 3:
        cropx = cx;
        cropy = (icd->height) - cy - ch;
        break;

    case 4:
        cropy = (icd->height) - cy - ch;
        cropx = (icd->width) - cx - cw;
        break;

    case 5:
        cropx = (icd->width / 2) + cx;
        cropy = (icd->height / 2) + cy;
        break;

    default:
        return (-1);
    }

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, icd->width);
    cairo_surface_t* image = cairo_image_surface_create_for_data(icd->image_px, CAIRO_FORMAT_ARGB32, icd->width, icd->height, stride); /*original image*/
    cairo_surface_t* cimage = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (double)itcd->w, (double)itcd->h); /*cropped image*/
    cairo_t* cr = cairo_create(cimage);
    cairo_translate(cr, -cropx, -cropy);
    cairo_set_source_surface(cr, image, 0, 0);

    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(image);

    icd->width = itcd->w;
    icd->height = itcd->h;

    sfree((void**)&icd->image_px);
    icd->image_px = zmalloc((size_t)labs(itcd->w * (size_t)itcd->h * 4));
    unsigned char* img_buf = cairo_image_surface_get_data(cimage);
    memcpy(icd->image_px, img_buf, (size_t)labs(itcd->w * (size_t)itcd->h * 4));
    cairo_surface_destroy(cimage);
    return (0);
}

static inline image_types image_cache_detect_format(FILE *fh)
{
    unsigned char buffer[8]= {0};
    size_t br=fread(buffer,1,sizeof(buffer),fh);
    fseek(fh,0,SEEK_SET);

    if(br >= 2 &&
            buffer[0] == 0x42 &&
            buffer[1] == 0x4D)
    {
        return (bitmap);
    }
    else if(br >= 3 &&
            buffer[0] == 0xFF &&
            buffer[1] == 0xD8 &&
            buffer[2] == 0xFF)
    {
        return (jpeg);
    }

    else if(br >= 8 &&
            buffer[0] == 0x89 &&
            buffer[1] == 0x50 &&
            buffer[2] == 0x4e &&
            buffer[3] == 0x47 &&
            buffer[4] == 0x0d &&
            buffer[5] == 0x0a &&
            buffer[6] == 0x1a &&
            buffer[7] == 0x0a)
    {
        return (png);
    }

    return (none);
}

static inline FILE *image_cache_open_image(unsigned char *f)
{
    FILE* fh = NULL;
    unsigned char *gf=clone_string(f);
    uniform_slashes(gf);
#ifdef WIN32
    wchar_t* fp = utf8_to_ucs(gf);
    fh = _wfopen(fp, L"rb");
    sfree((void**)&fp);
#elif __linux__
    fh = fopen(gf, "rb");
#endif
    sfree((void**)&gf);
    return(fh);
}

static inline int image_cache_decode(unsigned char* path, image_cache_decoded* icd)
{
    int ret = -1;


    FILE *fh=image_cache_open_image(path);

    if(fh==NULL)
    {
        return (-1);
    }

    switch(image_cache_detect_format(fh))
    {
    case jpeg:
        ret = image_cache_decode_jpeg(fh, icd);
        break;

    case png:
        ret = image_cache_decode_png(fh, icd);
        break;

    case bitmap:
        ret = image_cache_decode_bmp(fh, icd);
        break;

    case none:
        ret=image_cache_decode_svg(fh,icd);
        break;
    }

    fseek(fh,0,SEEK_SET);
    fclose(fh);
    return (ret);
}

int image_cache_init(control_data *cd)
{
    if(cd==NULL||cd->ich !=NULL)
    {
        return (-1);
    }

    if(cd->ich == NULL)
    {
        cd->ich = zmalloc(sizeof(image_cache));
        list_entry_init(&((image_cache*)cd->ich)->images);
    }

    return (0);
}

void image_cache_destroy(void** ic)
{
    image_cache* lic = *ic;

    if(lic)
    {
        image_entry* tie = NULL;
        image_entry* ie = NULL;
        list_enum_part_safe(ie, tie, &lic->images, current)
        {
            image_cache_remove_entry(&ie);
        }
        sfree((void**)ic);
    }
}

static void image_cache_remove_entry(image_entry** ie)
{
    image_entry* tie = *ie;
    linked_list_remove(&tie->current);

    sfree((void**)&tie->attrib.path);
    sfree((void**)&tie->image_px);
    sfree((void**)ie);
}

/*This function will load the image and do the requested tweaks on it*/
static int image_cache_load_image(image_cache_decoded *icd,image_attributes *ia)
{
    icd->height=ia->rh;
    icd->width=ia->rw;

    if(image_cache_decode(ia->path, icd))
    {
        return (-1);
    }

    image_cache_crop(icd, &ia->cpm);

    switch(ia->tile)
    {
    case 1:
        image_cache_tile(icd, ia->rw, ia->rh);
        break;

    case 2:
        image_cache_scale(icd, ia->tpm.w, ia->tpm.h,ia->keep_ratio);
        image_cache_tile(icd, ia->rw, ia->rh);
        break;

    default:
        image_cache_scale(icd, ia->rw, ia->rh,ia->keep_ratio);
        break;
    }

    if(ia->opacity != 255)
    {
        image_cache_adjust_opacity(icd, ia->opacity);
    }

    if(ia->flip)
    {
        image_cache_flip(icd, ia->flip);
    }

    image_cache_adjust_color_matrix(icd, ia);
    return(0);
}

static image_entry* image_cache_add(image_cache* ic, image_attributes* ia)
{
    image_cache_decoded icd = { 0 };

    if(image_cache_load_image(&icd,ia))
    {
        return(NULL);
    }

    image_entry* lie = zmalloc(sizeof(image_entry));

    /*set the the right addresses*/
    void *iac=(void*)(((unsigned char*)ia)+offsetof(image_attributes,reserved));
    void *ieac=(void*)(((unsigned char*)&lie->attrib)+offsetof(image_attributes,reserved));
    size_t bytes_to_copy=sizeof(image_attributes)-offsetof(image_attributes,reserved);

    /*Clone the information*/
    memcpy(ieac,iac,bytes_to_copy);

    /*Set the proper information*/
    semper_get_file_timestamp(ia->path,&lie->st); //gather the timestamp
    list_entry_init(&lie->current);
    linked_list_add_last(&lie->current, &ic->images);

    lie->attrib.path = clone_string(ia->path);
    lie->attrib.width=icd.width;
    lie->attrib.height=icd.height;
    lie->image_px=icd.image_px;

    return (lie);
}

/*If the image is in the cache, it will return it. If it's not it will return NULL*/
static image_entry *image_cache_look_through(image_cache *ic,image_attributes *ia)
{
    if(ia==NULL||ic==NULL)
    {
        return(NULL);
    }

    image_entry* sie = NULL;
    image_entry *tsie=NULL;
    semper_timestamp st2= {0};

    semper_get_file_timestamp(ia->path,&st2);

    list_enum_part_safe(sie,tsie, &ic->images, current)
    {
        /*Try to hit the cache (as hard as we can)*/
        if(sie->attrib.path && ia->path && strcasecmp(sie->attrib.path, ia->path) == 0)
        {
            void *p1=(void*)(((unsigned char*)ia)+offsetof(image_attributes,reserved));
            void *p2=(void*)(((unsigned char*)&sie->attrib)+offsetof(image_attributes,reserved));
            size_t btc=sizeof(image_attributes)-offsetof(image_attributes,reserved);

            if(memcmp(p1,p2,btc)==0)
            {
                //attributes are corresponding but let's check the timestamp
                if(memcmp(&sie->st,&st2,sizeof(semper_timestamp)))
                {
                    //timestamp is not right so let's remove the entry
                    image_cache_remove_entry(&sie);
                }
                else
                {
                    return(sie);
                }
            }
        }
    }
    return(NULL);
}


static image_entry* image_cache_request(void* ich, image_attributes* ia)
{
    image_entry* ret =NULL;

    if(ich == NULL || ia == NULL||ia->path==NULL)
    {
        return (NULL);
    }

    ret=image_cache_look_through(ich,ia);

    /*Do a search through the cache*/

    if(ret == NULL)  /*Not found - try and load it*/
    {
        ret = image_cache_add(ich, ia);

        if(ret)
            diag_verb("Added \"%s\" to the cache",ret->attrib.path);

        else
            diag_error("Failed to add \"%s\" to the cache",ia->path);
    }

    if(ret == NULL) /*no image here mate*/
    {
        return (NULL);
    }

    return (ret);
}

void image_cache_image_parameters(void* r, image_attributes *ia, unsigned char flags, char* pre)
{
    unsigned char buf[256] = { 0 };

    if(pre == NULL)
        pre = "";

    double* vcm = (double*)ia->cm;
    snprintf(buf, sizeof(buf), "%sOpacity", pre);
    ia->opacity = parameter_byte(r, buf, 255, flags);

    snprintf(buf, sizeof(buf), "%sTile", pre);
    ia->tile = parameter_byte(r, buf, 0, flags);

    snprintf(buf, sizeof(buf), "%sInvertRGB", pre);
    ia->rgb_bgr = parameter_bool(r, buf, 0, flags);

    snprintf(buf, sizeof(buf), "%sInvertColors", pre);
    ia->inv = parameter_bool(r, buf, 0, flags);

    snprintf(buf, sizeof(buf), "%sGrayscale", pre);
    ia->grayscale = parameter_bool(r, buf, 0, flags);

    snprintf(buf, sizeof(buf), "%sTint", pre);
    ia->tint_color = parameter_color(r, buf, -1, flags);

    snprintf(buf, sizeof(buf), "%sFlip", pre);
    unsigned char* temp = parameter_string(r, buf, NULL, flags);

    snprintf(buf, sizeof(buf), "%sKeepRatio", pre);
    ia->keep_ratio = parameter_byte(r, buf, 0, flags);

    if(temp)
    {
        strupr(temp);

        if(strstr(temp, "V"))
            ia->flip = 2;

        else if(strstr(temp, "H"))
            ia->flip = 1;

        else if(strstr(temp, "B"))
            ia->flip = 3;

        else
            ia->flip=0;

        sfree((void**)&temp);
    }
    else
    {
        ia->flip=0;
    }

    snprintf(buf, sizeof(buf), "%sImageTile", pre);
    parameter_image_tile(r, buf, &ia->tpm, flags);

    snprintf(buf, sizeof(buf), "%sImageCrop", pre);
    parameter_image_crop(r, buf, &ia->cpm, flags);

    snprintf(buf, sizeof(buf), "%sColorMatrix", pre);
    if(parameter_color_matrix(r, buf, vcm, flags))
        ia->cm[0] = 1.0;

    snprintf(buf, sizeof(buf), "%sColorMatrix1", pre);
    if(parameter_color_matrix(r, buf, vcm + 5, flags))
        ia->cm[6] = 1.0;

    snprintf(buf, sizeof(buf), "%sColorMatrix2", pre);
    if(parameter_color_matrix(r, buf, vcm + 10, flags))
        ia->cm[12] = 1.0;

    snprintf(buf, sizeof(buf), "%sColorMatrix3", pre);
    if(parameter_color_matrix(r, buf, vcm + 15, flags))
        ia->cm[18] = 1.0;

    snprintf(buf, sizeof(buf), "%sColorMatrix4", pre);
    if(parameter_color_matrix(r, buf, vcm + 20, flags))
        ia->cm[24] = 1.0;
}

void image_cache_unref_image(void *ic,image_attributes *ia,unsigned char riz)
{
    if(ic&&ia)
    {
        image_entry *ie=image_cache_look_through(ic,ia);

        if(ie&&ie->refs)
        {
            ie->refs--;
        }

        if(riz&&ie&&ie->refs==0)
        {
            image_cache_remove_entry(&ie);
        }
    }
}

int image_cache_query_image(void *ic,image_attributes *ia,unsigned char **px,long width,long height)
{
    if(px)
    {
        *px=NULL;
    }

    if(ic == NULL || ia == NULL)
    {
        return (-1);
    }

    ia->rw=(width<0?0:width);
    ia->rh=(height<0?0:height);

    image_entry* ie = image_cache_request(ic, ia);

    if(ie == NULL)
    {
        ia->width=0;
        ia->height=0;
        return (-1);
    }
    else
    {
        if(px)
        {
            *px=ie->image_px;
        }

        ia->width=ie->attrib.width;
        ia->height=ie->attrib.height;
        return (0);
    }
}
