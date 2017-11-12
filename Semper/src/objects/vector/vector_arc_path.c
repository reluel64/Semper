#include <cairo/cairo.h>
#include <objects/vector.h>


typedef enum
{
    Negative,
    Positive
} Sweep;



static void
rsvg_path_arc_segment (cairo_t *builder,
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

    cairo_curve_to (builder,
                    xc + cosf*x1 - sinf*y1,
                    yc + sinf*x1 + cosf*y1,
                    xc + cosf*x2 - sinf*y2,
                    yc + sinf*x2 + cosf*y2,
                    xc + cosf*x3 - sinf*y3,
                    yc + sinf*x3 + cosf*y3);
}

/**
 * rsvg_path_builder_arc:
 * @builder: Path builder.
 * @x1: Starting x coordinate
 * @y1: Starting y coordinate
 * @rx: Radius in x direction (before rotation).
 * @ry: Radius in y direction (before rotation).
 * @x_axis_rotation: Rotation angle for axes.
 * @large_arc_flag: 0 for arc length <= 180, 1 for arc >= 180.
 * @sweep_flag: 0 for "negative angle", 1 for "positive angle".
 * @x2: Ending x coordinate
 * @y2: Ending y coordinate
 *
 * Add an RSVG arc to the path context.
 **/
void
rsvg_path_builder_arc (cairo_t  *builder,
                       double x1, double y1,
                       double rx, double ry,
                       double x_axis_rotation,
                       unsigned char large_arc_flag, unsigned char sweep_flag,
                       double x2, double y2)
{

    /* See Appendix F.6 Elliptical arc implementation notes
       http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes */

    double f, sinf, cosf;
    double x1_, y1_;
    double cx_, cy_, cx, cy;
    double gamma;
    double theta1, delta_theta;
    double k1, k2, k3, k4, k5;

    int i, n_segs;



    /* X-axis */
    f = x_axis_rotation * M_PI / 180.0;
    sinf = sin (f);
    cosf = cos (f);

    rx = fabs (rx);
    ry = fabs (ry);

    /* Check the radius against floading point underflow.
       See http://bugs.debian.org/508443 */
    if ((rx < 0.01) || (ry < 0.01))
    {
        cairo_line_to (builder, x2, y2);
        return;
    }

    k1 = (x1 - x2) / 2;
    k2 = (y1 - y2) / 2;

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
    if (sweep_flag == large_arc_flag)
        k1 = -k1;

    cx_ = k1 * rx * y1_ / ry;
    cy_ = -k1 * ry * x1_ / rx;

    cx = cosf * cx_ - sinf * cy_ + (x1 + x2) / 2;
    cy = sinf * cx_ + cosf * cy_ + (y1 + y2) / 2;

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

    if (sweep_flag && delta_theta < 0)
        delta_theta += M_PI * 2;
    else if (!sweep_flag && delta_theta > 0)
        delta_theta -= M_PI * 2;

    /* Now draw the arc */

    n_segs = ceil (fabs (delta_theta / (M_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++)
        rsvg_path_arc_segment (builder, cx, cy,
                               theta1 + i * delta_theta / n_segs,
                               theta1 + (i + 1) * delta_theta / n_segs,
                               rx, ry, x_axis_rotation);
}


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

    rsvg_path_builder_arc(cr,va->sx,va->sy,va->rx,va->ry,va->angle,va->large,va->sweep,va->ex,va->ey);
    return(0);
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

    rx2=pow(va->rx,2);
    ry2=pow(va->ry,2);

    sinf=sin(va->angle);
    cosf=cos(va->angle);
    _x1=cosf*((va->sx-va->ex)/2.0)+sinf*((va->sy-va->ey)/2.0);
    _y1=-sinf*((va->sx-va->ex)/2.0)+cosf*((va->sy-va->ey)/2.0);
    _x2=pow(_x1,2);
    _y2=pow(_y1,2);
    sq=(sqrt((rx2*ry2-rx2*_y2-ry2*_x2)/(rx2*_y2+ry2*_x2)));

    _cx=(va->sweep==va->large?-sq:sq)*((va->rx*_y1)/va->ry);
    _cy=(va->sweep==va->large?-sq:sq)*-((va->ry*_x1)/va->rx);

    cx=cosf*_cx-sinf*_cy+(va->sx+va->ex)/2;
    cx=sinf*_cx+cosf*_cy+(va->sy+va->ey)/2;

    /*Caluclate theta*/

    vlen=sqrt(pow((_x1-_cx)/va->rx,2)+pow((_y1-_cy)/va->ry,2));

    if(vlen==0.0)
    {
        return(-1);
    }

    dp=(_x1-_cx)/va->rx;
    theta=acos(CLAMP(dp/vlen,-1,1));

    vlen=sqrt(
             (pow((_x1-_cx)/va->rx,2)+pow((_y1-_cy)/va->ry,2))  *
             (pow((-_x1-_cx)/va->rx,2)+pow((-_y1-_cy)/va->ry,2))
         );

    if(vlen==0.0)
    {
        return(-1);
    }

    dp=(((_x1-_cx)/va->rx)*((-_x1-_cx)/va->rx))+(((_y1-_cy)/va->ry)*((-_y1-_cy)/va->ry));

    dtheta=acos(CLAMP(dp/vlen,-1,1));




    return(0);
}
