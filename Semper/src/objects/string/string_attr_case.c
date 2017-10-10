#include <pango/pango.h>
#include <mem.h>
#include <sources/extension.h>
#include <objects/string/string_attr.h>
#include <string_util.h>
static PangoAttribute *string_attr_case_copy(const PangoAttribute *pa)
{
    PangoAttrInt *attr=zmalloc(sizeof(PangoAttrInt));
    PangoAttrInt *src=(PangoAttrInt*)pa;
    memcpy(attr,src,sizeof(PangoAttrInt));
    return(&attr->attr);
}

static void string_attr_case_destroy(PangoAttribute *pa1)
{
    PangoAttrInt *pac=(PangoAttrInt*)pa1;
    sfree((void**)&pac);
}

static int string_attr_case_compare(const PangoAttribute *pa1,const PangoAttribute *pa2)
{
    unused_parameter(pa1);
    unused_parameter(pa2);
    return(0);
}

PangoAttribute *string_attr_case(unsigned char case_type,size_t start,size_t end)
{
    static PangoAttrClass cls=
    {
        .type=STRING_ATTR_CASE,
        .copy=string_attr_case_copy,
        .destroy=string_attr_case_destroy,
        .equal=string_attr_case_compare
    };

    PangoAttrInt *attr=zmalloc(sizeof(PangoAttrInt));
    attr->value=case_type;
    attr->attr.klass=&cls;
    attr->attr.start_index=start;
    attr->attr.end_index=end;
    return(&attr->attr);
}

int string_attr_case_handler(PangoAttribute *pa,void *pv)
{
    string_object *so=pv;
    PangoAttrInt *pai=(PangoAttrInt*)pa;
    size_t end=0;
    unsigned char save_ch=0;

    if(pa->klass->type!=STRING_ATTR_CASE)
    {
        return(0);
    }

    if(pa->end_index!=-1)
    {
        end=pa->end_index;
    }
    else
    {
        end=string_length(so->bind_string);
    }

    if(so->bind_string==so->string)
    {
        so->bind_string=clone_string(so->string);
    }

    save_ch=so->bind_string[end];
    so->bind_string[end]=0;

    if(pai->value==STRING_CASE_LOWER)
    {
        string_lower(so->bind_string+pa->start_index);
    }
    else if(pai->value==STRING_CASE_UPPER)
    {
        string_upper(so->bind_string+pa->start_index);
    }

    so->bind_string[end]=save_ch;

    return(0);
}
