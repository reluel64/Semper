/*
String object rise attribute
Part of Project "Semper"
Wrriten by Alexandru-Daniel Mărgărit
*/
#include <pango/pango.h>
#include <mem.h>
#include <sources/extension.h>

static PangoAttribute *string_attr_rise_copy(const PangoAttribute *pa)
{
    PangoAttrInt *attr=zmalloc(sizeof(PangoAttrInt));
    PangoAttrInt *src=(PangoAttrInt*)pa;
    memcpy(attr,src,sizeof(PangoAttrInt));
    return(&attr->attr);
}

static void string_attr_rise_destroy(PangoAttribute *pa1)
{
    PangoAttrInt *pac=(PangoAttrInt*)pa1;
    sfree((void**)&pac);
}

static int string_attr_rise_compare(const PangoAttribute *pa1,const PangoAttribute *pa2)
{
    unused_parameter(pa1);
    unused_parameter(pa2);
    return(0);
}

PangoAttribute *string_attr_rise(int rise,size_t start,size_t end)
{
    static PangoAttrClass cls=
    {
        .type=PANGO_ATTR_RISE,
        .copy=string_attr_rise_copy,
        .destroy=string_attr_rise_destroy,
        .equal=string_attr_rise_compare
    };

    PangoAttrInt *attr=zmalloc(sizeof(PangoAttrInt));
    attr->value=rise*PANGO_SCALE;
    attr->attr.klass=&cls;
    attr->attr.start_index=start;
    attr->attr.end_index=end;
    return(&attr->attr);
}
