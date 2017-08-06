#include <objects/object.h>

typedef struct _arc_obj
{
    unsigned char sraw;
    float start_angle;
    float step_angle;
    long radius;
    unsigned int color;
    unsigned char fill;
    unsigned char rounded;

    long width; // unimplemented
    float percents;
} arc_object;

void arc_init(object* o);
void arc_destroy(object* o);
void arc_reset(object* o);
int arc_update(object* o);
int arc_render(object* o, cairo_t* cr);
