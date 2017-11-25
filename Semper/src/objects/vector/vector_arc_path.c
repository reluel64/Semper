#include <cairo/cairo.h>
#include <objects/vector.h>

/*Adapted from librsvg*/
typedef enum
{
    Negative,
    Positive
} Sweep;


static void
vector_arc_path_segment (cairo_t *cr,
                         double xc, double yc,
                         double th0, double th1, double rx, double ry,
                         double x_axis_rotation)
{
    double x1, y1, x2, y2, x3, y3;
    double t;
    double th_half;
    double f, sinf, cosf;

    f = x_axis_rotation * M_PI / 180.0;
    sinf = sin(f);
    cosf = cos(f);

    th_half = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * sin (th_half * 0.5) * sin (th_half * 0.5) / sin (th_half);
    x1 = rx*(cos (th0) - t * sin (th0));
    y1 = ry*(sin (th0) + t * cos (th0));
    x3 = rx*cos (th1);
    y3 = ry*sin (th1);
    x2 = x3 + rx*(t * sin (th1));
    y2 = y3 + ry*(-t * cos (th1));

    cairo_curve_to (cr,
                    xc + cosf*x1 - sinf*y1,
                    yc + sinf*x1 + cosf*y1,
                    xc + cosf*x2 - sinf*y2,
                    yc + sinf*x2 + cosf*y2,
                    xc + cosf*x3 - sinf*y3,
                    yc + sinf*x3 + cosf*y3);
}


void
vector_arc_path (cairo_t *cr, vector_arc *va)
{

    double sinf=0.0;
    double cosf=0.0;
    double x1_=0.0;
    double y1_=0.0;
    double cx_=0.0;
    double cy_=0.0;
    double cx=0.0;
    double cy=0.0;
    double gamma=0.0;
    double theta1=0.0;
    double delta_theta=0.0;
    double k1=0.0;
    double k2=0.0;
    double k3=0.0;
    double k4=0.0;
    double k5=0.0;
    double rx=0.0;
    double ry=0.0;
    int i=0;
    int n_segs=0;

    if (va->sx == va->ex && va->sy == va->ey)
        return;

    /* X-axis */

    sinf = sin (va->angle);
    cosf = cos (va->angle);

    rx = fabs (va->rx);
    ry = fabs (va->ry);

    /* Check the radius against floading point underflow.
       See http://bugs.debian.org/508443 */
    if ((rx < 0.001) || (ry < 0.001))
    {
        cairo_move_to(cr,va->sx,va->sy);
        cairo_line_to (cr, va->ex, va->ey);
        return;
    }

    k1 = (va->sx - va->ex) / 2;
    k2 = (va->sy - va->ey) / 2;

    x1_ = cosf * k1 + sinf * k2;
    y1_ = -sinf * k1 + cosf * k2;

    gamma = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);

    if (gamma > 1)
    {
        rx *= sqrt (gamma);
        ry *= sqrt (gamma);
    }

    /* Compute the center */

    k1 = rx * rx * y1_ * y1_ + ry * ry * x1_ * x1_;

    if (k1 == 0)
        return;

    k1 = sqrt (fabs ((rx * rx * ry * ry) / k1 - 1));

    if (va->sweep ==va->large)
        k1 = -k1;

    cx_ = k1 * rx * y1_ / ry;
    cy_ = -k1 * ry * x1_ / rx;

    cx = cosf * cx_ - sinf * cy_ + (va->sx + va->ex) / 2;
    cy = sinf * cx_ + cosf * cy_ + (va->sy+ va->ey) / 2;

    /* Compute start angle */

    k1 = (x1_ - cx_) / rx;
    k2 = (y1_ - cy_) / ry;
    k3 = (-x1_ - cx_) / rx;
    k4 = (-y1_ - cy_) / ry;

    k5 = sqrt (fabs (k1 * k1 + k2 * k2));

    if (k5 == 0)
        return;

    k5 = k1 / k5;
    k5 = CLAMP (k5, -1, 1);
    theta1 = acos (k5);

    if (k2 < 0)
        theta1 = -theta1;

    /* Compute delta_theta */

    k5 = sqrt (fabs ((k1 * k1 + k2 * k2) * (k3 * k3 + k4 * k4)));

    if (k5 == 0)
        return;

    k5 = (k1 * k3 + k2 * k4) / k5;
    k5 = CLAMP (k5, -1, 1);
    delta_theta = acos (k5);

    if (k1 * k4 - k3 * k2 < 0)
        delta_theta = -delta_theta;

    if (va->sweep && delta_theta < 0)
        delta_theta += M_PI * 2;
    else if (!va->sweep && delta_theta > 0)
        delta_theta -= M_PI * 2;

    /* Now draw the arc */

    n_segs = ceil (fabs (delta_theta / (M_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++)
    {
        vector_arc_path_segment (cr, cx, cy,
                                 theta1 + i * delta_theta / n_segs,
                                 theta1 + (i + 1) * delta_theta / n_segs,
                                 rx, ry, va->angle);
    }
}

/*
int vector_arc_path(cairo_t *cr,vector_arc *va)
{
    double sinf=0.0;
    double cosf=0.0;
    double _x1=0.0;
    double _y1=0.0;
    double _x2=0.0;
    double _y2=0.0;
    double _cx=0.0;
    double _cy=0.0;
    double cx=0.0;
    double cy=0.0;
    double rx2=0.0;
    double ry2=0.0;
    double sq=0.0;
    double theta=0.0;
    double dtheta=0.0;
    double vlen=0.0;
    double dp=0.0;
    double gamma=0.0;
    double rx=va->rx;
    double ry=va->ry;

    if(va->sx==va->ex&&va->sy==va->ey)
    {
        return(0);
    }



    if(va->rx==0.0||va->ry==0.0)
    {
        cairo_move_to(cr,va->sx,va->sy);
        cairo_line_to(cr,va->ex,va->ey);
        return(0);
    }



    rx=fabs(va->rx);
    ry=fabs(va->ry);

    rx2=pow(rx,2);
    ry2=pow(ry,2);


    sinf=sin(va->angle);
    cosf=cos(va->angle);
    _x1=cosf*((va->sx-va->ex)/2.0)+sinf*((va->sy-va->ey)/2.0);
    _y1=-sinf*((va->sx-va->ex)/2.0)+cosf*((va->sy-va->ey)/2.0);

    gamma=(pow(_x1,2)/rx2)+(pow(_y1,2)/ry2);
    if(gamma>1.0)
    {
        rx=sqrt(gamma)*rx;
        ry=sqrt(gamma)*ry;
    }
    rx2=pow(rx,2);
    ry2=pow(ry,2);
    _x2=pow(_x1,2);
    _y2=pow(_y1,2);
    sq=(sqrt(fabs((rx2*ry2-rx2*_y2-ry2*_x2)/(rx2*_y2+ry2*_x2))));

    _cx=(va->sweep==va->large?-sq:sq)*((rx*_y1)/ry);
    _cy=(va->sweep==va->large?-sq:sq)*-((ry*_x1)/rx);

    cx=cosf*_cx-sinf*_cy+(va->sx+va->ex)/2;
    cy=sinf*_cx+cosf*_cy+(va->sy+va->ey)/2;



    vlen=sqrt(pow((_x1-_cx)/rx,2)+pow((_y1-_cy)/ry,2));

    if(vlen==0.0)
    {
        return(-1);
    }

    dp=(_x1-_cx)/rx;


    theta=(((_y1-_cy)/rx)>=0.0)?acos(CLAMP(dp/vlen,-1,1)):-acos(CLAMP(dp/vlen,-1,1));


    vlen=sqrt(
             (pow((_x1-_cx)/rx,2)+pow((_y1-_cy)/ry,2))  *
             (pow((-_x1-_cx)/rx,2)+pow((-_y1-_cy)/ry,2))
         );

    if(vlen==0.0)
    {
        return(-1);
    }

    dp=(((_x1-_cx)/rx)*((-_x1-_cx)/rx))+(((_y1-_cy)/ry)*((-_y1-_cy)/ry));

    dtheta=((((_x1-_cx)/va->rx)*((-_y1-_cy)/va->ry))-
            (((_y1-_cy)/va->ry)*((-_x1-_cx)/va->rx))) >=0.0 ?acos(CLAMP(dp/vlen,-1,1)):-acos(CLAMP(dp/vlen,-1,1));


    int n_segs = 2;
    cairo_move_to(cr,va->sx,va->sy);
    for (int i = 0; i < n_segs; i++)
        rsvg_path_arc_segment (cr, cx, cy,
                               theta + i * dtheta / n_segs,
                               theta + (i + 1) * dtheta / n_segs,
                               rx, ry, va->angle);

    return(0);
}
*/
