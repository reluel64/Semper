/*
String object size attribute
Part of Project "Semper"
Wrriten by Alexandru-Daniel Mărgărit
*/
#include <pango/pango.h>
#include <mem.h>
#include <sources/extension.h>

static PangoAttribute *string_attr_size_copy(const PangoAttribute *pa)
{
    PangoAttrSize *attr=zmalloc(sizeof(PangoAttrSize));
    PangoAttrSize *src=(PangoAttrSize*)pa;
    memcpy(attr,src,sizeof(PangoAttrSize));
    return(&attr->attr);
}

static void string_attr_size_destroy(PangoAttribute *pa1)
{
    PangoAttrSize *pac=(PangoAttrSize*)pa1;
    sfree((void**)&pac);
}

static int string_attr_size_compare(const PangoAttribute *pa1,const PangoAttribute *pa2)
{
    unused_parameter(pa1);
    unused_parameter(pa2);
    return(0);
}

PangoAttribute *string_attr_size(double size,size_t start,size_t end)
{
    static PangoAttrClass cls=
    {
        .type=PANGO_ATTR_SIZE,
        .copy=string_attr_size_copy,
        .destroy=string_attr_size_destroy,
        .equal=string_attr_size_compare
    };

    PangoAttrSize *attr=zmalloc(sizeof(PangoAttrSize));
    attr->size=(unsigned int)(size*(double)PANGO_SCALE);
    attr->attr.klass=&cls;
    attr->attr.start_index=start;
    attr->attr.end_index=end;
    return(&attr->attr);
}
