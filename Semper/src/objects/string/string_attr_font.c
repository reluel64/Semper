/*
String object font attribute
Part of Project "Semper"
Wrriten by Alexandru-Daniel Mărgărit
*/
#include <pango/pango.h>
#include <mem.h>
#include <semper_api.h>

static PangoAttribute *string_attr_font_copy(const PangoAttribute *pa)
{
    PangoAttrString *attr=zmalloc(sizeof(PangoAttrString));
    PangoAttrString *src=(PangoAttrString*)pa;
    memcpy(attr,src,sizeof(PangoAttrString));
    return(&attr->attr);
}

static void string_attr_font_destroy(PangoAttribute *pa1)
{
    PangoAttrString *pac=(PangoAttrString*)pa1;
    sfree((void**)&pac);
}

static int string_attr_font_compare(const PangoAttribute *pa1,const PangoAttribute *pa2)
{
    unused_parameter(pa1);
    unused_parameter(pa2);
    return(0);
}

PangoAttribute *string_attr_font(unsigned char *font_name,size_t start,size_t end)
{
    static PangoAttrClass cls=
    {
        .type=PANGO_ATTR_FAMILY,
        .copy=string_attr_font_copy,
        .destroy=string_attr_font_destroy,
        .equal=string_attr_font_compare
    };

    PangoAttrString *attr=zmalloc(sizeof(PangoAttrString));
    attr->value=(char*)font_name;
    attr->attr.klass=&cls;
    attr->attr.start_index=start;
    attr->attr.end_index=end;
    return(&attr->attr);
}
