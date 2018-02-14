/*
 * Calculator source
 * Part of Project Semper
 * Written by Alexandru-Daniel Mărgărit
 */
#include <sources/calculator.h>
#include <semper_api.h>
#include <string_util.h>
#include <mem.h>
#include <sources/source.h>
#include <stddef.h>
#include <math_parser.h>

typedef struct
{
    unsigned char unique;
    unsigned char update;
    unsigned short seed;          // current random number
    unsigned short* vec;          // vector of random numbers
    unsigned short rnum;          // the random number
    unsigned short min_random;
    unsigned short max_random;
    size_t la;                    // last accessed
    size_t vec_count;             // elements in vector
    unsigned char* frm; 			// formula
    void* sd; 						// surface data

} calculator;

static inline size_t calculator_xorshift_generator(size_t seed)
{
    seed ^= seed >> 12; // a
    seed ^= seed << 25; // b
    seed ^= seed >> 27; // c
    return seed * (size_t)(2685821657736338717);
}

static void calculator_knuth_shuffle(unsigned short* v, size_t n)
{
    for(unsigned long i = 0; i < n; i++)
    {
        unsigned long cxchg = 0;
        unsigned short temp = 0;
        cxchg = calculator_xorshift_generator(n - i + 1) % n;
        temp = v[cxchg];
        v[cxchg] = v[n - i - 1];
        v[n - i - 1] = temp;
    }
}


static inline unsigned short calculator_random(calculator* c)
{
    if(c->update)
    {
        if(c->unique)
        {
            if(c->la == 0)
            {
                size_t tcount = (c->max_random - c->min_random) + 1;

                if(c->vec == NULL)
                {
                    c->vec = zmalloc(tcount * sizeof(unsigned short));
                }

                if(c->vec_count != tcount)
                {
                    sfree((void**)&c->vec);
                    c->vec = zmalloc(tcount * sizeof(unsigned short));
                }

                c->vec_count = tcount;
                c->la = tcount;

                for(size_t i = 0; i < tcount; i++)
                {
                    c->vec[i] = i + c->min_random;
                }

                calculator_knuth_shuffle(c->vec, tcount);
                c->rnum = c->vec[--c->la];
            }
            else
            {
                c->rnum = c->vec[--c->la];
            }
            return (c->rnum);
        }
        else
        {
            unsigned char retries = 128;
            unsigned short old_rnum = c->rnum;

            do
            {
                c->rnum = (unsigned short)calculator_xorshift_generator(time(NULL));
                c->seed = c->rnum;
            }
            while((c->rnum > c->max_random || c->rnum < c->min_random) && retries--);

            if(retries == 0)
            {
                c->rnum = old_rnum;
            }

            return (c->rnum);
        }
    }

    return (c->rnum);
}

static int calculator_math_parser(unsigned char *vn,size_t len,double *v,void *pv)
{
    calculator *c=pv;

    if(strncasecmp("Random",vn,len)==0)
    {

        *v=(double)calculator_random(c);
        return(0);
    }
    else if(strncasecmp("SurfaceCycle",vn,len)==0)
    {
        *v=(double)((surface_data*)c->sd)->cycle;
        return(0);
    }
    else
    {
        source *s=source_by_name(c->sd,vn,len);

        if(s)
        {
            *v=(double)s->d_info;
            return(0);
        }
    }

    return(-1);
}

void calculator_init(void** spv, void* ip)
{
    calculator* c = zmalloc(sizeof(calculator));
    c->sd = get_surface(ip);
    *spv = c;
}

void calculator_reset(void* spv, void* ip)
{

    calculator* c = spv;
    sfree((void**)&c->frm);
    c->frm = clone_string(param_string("Function", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, "0"));
    c->max_random = (unsigned short)param_double("MaxRandom", ip, 65535.0);
    c->min_random = (unsigned short)param_double("MinRandom", ip, 0.0);
    c->seed = (unsigned int)time(NULL);

    if(c->max_random == 0)
    {
        c->max_random = 100;
    }

    if(c->max_random == c->min_random)
    {
        c->max_random = c->min_random + 1;
    }

    c->update = 1; // set it, temporarly, to 1 in order to obtain a first and potentially the single random number
    calculator_random(c);
    c->update = param_bool("RefreshRandom", ip, 0);
    c->unique = param_bool("AlwaysRandom", ip, 0);

    if(c->unique && !c->update)
    {
        c->update = 1;
    }
}

double calculator_update(void* spv)
{
    calculator* c = spv;
    double v=0.0;

    if(math_parser(c->frm,&v,calculator_math_parser,c))
        return(0.0);

    return (v);
}

void calculator_destroy(void** spv)
{
    calculator* c = *spv;
    sfree((void**)&c->frm);
    sfree((void**)&c->vec);
    sfree(spv);
}
