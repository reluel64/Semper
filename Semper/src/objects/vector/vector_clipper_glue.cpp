/*Bind between Clipper library and Vector Object
 * Part of Project Semper
 */
#include <stdint.h>
#include <3rdparty/clipper.hpp>
#include <3rdparty/cairo_clipper.hpp>
using namespace ClipperLib;

extern "C"   Clipper *vector_init_clipper(void)
{
    try
    {
        return(new Clipper);
    }
    catch (...)
    {
        return(NULL);
    }
}

extern "C"    Paths *vector_init_paths(void)
{
    try
    {
        return(new Paths);
    }
    catch(...)
    {
        return(NULL);
    }
}

extern "C"    void vector_destroy_paths(Paths **p)
{
    if(p&&*p)
    {
        delete *p;
        *p=NULL;
    }
}
extern "C"    void vector_destroy_clipper(Clipper **c)
{
    if(c&&*c)
    {
        delete *c;
        *c=NULL;
    }
}

extern "C"  void vector_add_paths(Clipper *c,Paths *p,PolyType t,int closed)
{
    if(c&&p)
    {
        try
        {
            c->AddPaths(*p,t,closed?true:false);
        }
        catch(...)
        {
            printf("erreur\n");
        }
    }
}

extern "C"  void  vector_clip_paths(Clipper *c,Paths *p,ClipType t)
{
    try
    {
        c->Execute(t,*p,pftNonZero,pftNonZero);
    }
    catch(...)
    {
        ;
    }
}

extern "C"    void vector_cairo_to_clip(cairo_t *cr,Paths *p)
{
    try
    {
        cairo::cairo_to_clipper(cr,*p,5.0);
    }
    catch(...)
    {
        ;
    }
}

extern "C"   void vector_clip_to_cairo(cairo_t *cr,Paths *p)
{
    try
    {
        cairo::clipper_to_cairo(*p,cr,5.0);
    }
    catch(...)
    {
        ;
    }
}
