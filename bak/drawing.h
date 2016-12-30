#ifndef __DRAWING_H__
#define __DRAWING_H__

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>


cairo_surface_t *cairo_create_x11_surface(int*, int*);
void cairo_close_x11_surface(cairo_surface_t*);
void draw_statics (cairo_t*, int, int);
void draw_dynamics (cairo_t*, int, int);

#endif
