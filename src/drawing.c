#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DIST_FROM_TOP 		0.37
#define DIST_FROM_BOTTOM 	0.06
#define DIST_FROM_LEFT 		0.04
#define DIST_FROM_RIGHT 	0.03
#define DIST_FROM_MIDDLE 	0.025
#define FONT_FACE 			"Lato"
#define COMP_SCALE_BOT 		-5
#define COMP_SCALE_TOP 		15
#define COMP_STEP_SIZE 		1
#define COMP_STEP_LABEL 	5
#define SECT_SCALE_BOT 		0
#define SECT_SCALE_TOP 		6
#define SECT_STEP_LABEL 	1
#define SECT_STEP_SIZE 		1
#define COMP_PRES_NUMBER 	4
#define SECT_PRES_NUMBER 	29
#define M_PI				3.141592653589793238

float comp_pres[4];  //compressor section pressures.
float sect_pres[29]; //test section pressures.
float ref_press;	 //reference pressure from HEISS DXD
float spacing[30] = { 48.0 / 724, 48.0 / 724, 48.0 / 724, 48.0 / 724, 48.0 / 724, 48.0 / 724,
	40.0 / 724, 24.0 / 724, 24.0 / 724, 24.0 / 724, 12.0 / 724, 12.0 / 724,
	12.0 / 724, 12.0 / 724, 12.0 / 724, 13.0 / 724, 17.0 / 724, 12.0 / 724,
	12.0 / 724, 18.0 / 724, 24.0 / 724, 24.0 / 724, 24.0 / 724, 24.0 / 724,
	24.0 / 724, 24.0 / 724, 24.0 / 724, 24.0 / 724, 24.0 / 724, 24.0 / 724 };

static struct {
	cairo_surface_t* image;
} glob;

static void fullscreen(Display* dpy, Window win)
{
	Atom atoms[2] = { XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False), None };
	XChangeProperty(dpy, win, XInternAtom(dpy, "_NET_WM_STATE", False),
		XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, 1);
}

/*! Open an X11 window and create a cairo surface based on that window. If x and
 * y are set to 0 the function opens a full screen window and stores to window
 * dimensions to x and y.
 * @param x Pointer to width of window.
 * @param y Pointer to height of window.
 * @return Returns a pointer to a valid Xlib cairo surface. The function does
 * not return on error (exit(3)).
 */

cairo_surface_t* cairo_create_x11_surface(int* x, int* y)
{
	Display* dsp;
	Drawable da; //Also the window name
	Screen* scr;
	int screen;
	cairo_surface_t* sfc;
	XTextProperty window_title_property;
	char* window_title = "10x10 Manometer";

	if ((dsp = XOpenDisplay(NULL)) == NULL)
		exit(1);

	screen = DefaultScreen(dsp);
	scr = DefaultScreenOfDisplay(dsp);

	if (!*x || !*y) {
		*x = WidthOfScreen(scr), *y = HeightOfScreen(scr);
		da = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0, *x, *y, 0, 0, 0);
		fullscreen(dsp, da);
	} else
		da = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0, *x, *y, 0, 0, 0);

	XSelectInput(dsp, da, ButtonPressMask | KeyPressMask);
	XMapWindow(dsp, da);

	sfc = cairo_xlib_surface_create(dsp, da, DefaultVisual(dsp, screen), *x, *y);
	cairo_xlib_surface_set_size(sfc, *x, *y);

	int rc = XStringListToTextProperty(&window_title, 1, &window_title_property);

	if (rc == 0) {
		fprintf(stderr, "XStringListToTextProperty - out of memory\n");
		exit(1);
	}
	/* assume that window_title_property is our XTextProperty, and is */
	/* defined to contain the desired window title.     */
	XSetWMName(dsp, da, &window_title_property);
	XRaiseWindow(dsp, da);

	return sfc;
}

/*! Destroy cairo Xlib surface and close X connection.
 */

void cairo_close_x11_surface(cairo_surface_t* sfc)
{
	Display* dsp = cairo_xlib_surface_get_display(sfc);

	cairo_surface_destroy(sfc);
	XCloseDisplay(dsp);
}

//Load a png to display as a background

int load_image(void)
{
	glob.image = cairo_image_surface_create_from_png("./background.png");
	if (cairo_surface_status(glob.image) != CAIRO_STATUS_SUCCESS) {
		printf("Error loading ./background.png : %s \nUsing a black background. \n", cairo_status_to_string(cairo_surface_status(glob.image)));
		return 1;
	}
	return 0;
}
//Draw the box for the bootup screen.
void draw_boot(cairo_t* ctx, int w, int h)
{
	cairo_push_group(ctx);
	cairo_set_source_rgb(ctx, 0, 0, 0);
	cairo_rectangle(ctx, 0.1*w, h*0.35, 0.8*w, 0.3*h);
	cairo_fill(ctx);
	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_rectangle(ctx, 0.1*w, h*0.35, 0.8*w, 0.3*h);
	cairo_stroke(ctx);

	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_text_extents_t te;
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(ctx, w * 0.03);
	cairo_text_extents(ctx, "Waiting for Scanivalve to boot", &te);
	cairo_move_to(ctx, w * 0.5 - te.width / 2 - te.x_bearing, h * (0.4) + te.height);
	cairo_show_text(ctx, "Waiting for Scanivalve to boot");

	cairo_set_font_size(ctx, w * 0.02);
	cairo_text_extents(ctx, "The Scanivalve pressure scanner takes up to 2 minutes to boot.", &te);
	cairo_move_to(ctx, w * 0.5 - te.width / 2 - te.x_bearing, h * (0.55) + te.height);
	cairo_show_text(ctx, "The Scanivalve pressure scanner takes up to 2 minutes to boot.");

	cairo_pop_group_to_source(ctx);
	cairo_paint(ctx);
}
	
//Draw the static screen items

void draw_statics(cairo_t* ctx, int w, int h)
{
	int number_of_steps;
	int i = 0;
	char axis_label[4];
	char tunnelstation[5];
	double x, y;

	// Draw the Labels
	//printf("Screen Resolution:  width = %d, height = %d \n", w, h);
	cairo_push_group(ctx);

	cairo_set_source_rgb(ctx, 0, 0, 0);
	cairo_paint(ctx);

	// Draw the section labels

	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_text_extents_t te;
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(ctx, w * 0.025);

	cairo_text_extents(ctx, "10x10 Manometer", &te);
	cairo_move_to(ctx, w * 0.5 - te.width / 2 - te.x_bearing, h * (0.01) + te.height);
	cairo_show_text(ctx, "10x10 Manometer");

	//Draw second throat diagram

	//Draw walls and center line
	cairo_move_to(ctx, (w * DIST_FROM_LEFT), h * 0.06);
	cairo_line_to(ctx, (w * (1 - DIST_FROM_RIGHT)), h * 0.06);
	cairo_set_line_width(ctx, 5);
	cairo_stroke(ctx);

	cairo_move_to(ctx, (w * DIST_FROM_LEFT), h * (0.06 + DIST_FROM_TOP - 0.01) / 2);
	cairo_line_to(ctx, (w * (1 - DIST_FROM_RIGHT)), h * (0.06 + DIST_FROM_TOP - 0.01) / 2);
	cairo_set_line_width(ctx, 1);
	cairo_stroke(ctx);

	cairo_move_to(ctx, (w * DIST_FROM_LEFT), h * (DIST_FROM_TOP - 0.01));
	cairo_line_to(ctx, (w * (1 - DIST_FROM_RIGHT)), h * (DIST_FROM_TOP - 0.01));
	cairo_set_line_width(ctx, 5);
	cairo_stroke(ctx);

	//Draw Second throat

	cairo_set_line_width(ctx, 2);
	cairo_move_to(ctx, (w * DIST_FROM_LEFT), h * 0.06);
	cairo_line_to(ctx, (w * (1 - .325)), h * 1 * (0.06 + DIST_FROM_TOP + 0.01) / 3);
	cairo_line_to(ctx, (w * (1 - DIST_FROM_RIGHT - .15)), h * 0.06);
	cairo_stroke(ctx);

	cairo_move_to(ctx, (w * DIST_FROM_LEFT), h * (DIST_FROM_TOP - 0.01));
	cairo_line_to(ctx, (w * (1 - .325)), h * 2 * (0.06 + DIST_FROM_TOP - 0.01) / 3);
	cairo_line_to(ctx, (w * (1 - DIST_FROM_RIGHT - .15)), h * (DIST_FROM_TOP - 0.01));
	cairo_stroke(ctx);

	cairo_set_source_rgba(ctx, 1, 1, 1, 0.2);

	cairo_move_to(ctx, (w * DIST_FROM_LEFT), h * 0.06);
	cairo_line_to(ctx, (w * (1 - .325)), h * 1 * (0.06 + DIST_FROM_TOP + 0.01) / 3);
	cairo_line_to(ctx, (w * (1 - DIST_FROM_RIGHT - .15)), h * 0.06);
	cairo_fill(ctx);

	cairo_move_to(ctx, (w * DIST_FROM_LEFT), h * (DIST_FROM_TOP - 0.01));
	cairo_line_to(ctx, (w * (1 - .325)), h * 2 * (0.06 + DIST_FROM_TOP - 0.01) / 3);
	cairo_line_to(ctx, (w * (1 - DIST_FROM_RIGHT - .15)), h * (DIST_FROM_TOP - 0.01));
	cairo_fill(ctx);

	cairo_set_source_rgb(ctx, 1, 1, 1);

	cairo_move_to(ctx, w * (0.004 + DIST_FROM_LEFT + spacing[0] / 2), h * (1 - DIST_FROM_BOTTOM) + 1.5 * te.height);

	cairo_save(ctx);
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ctx, w * 0.012);

	for (i = 0; i < SECT_PRES_NUMBER; i++) {
		snprintf(tunnelstation, 5, "%d", 579 - i);
		cairo_text_extents(ctx, tunnelstation, &te);
		cairo_get_current_point(ctx, &x, &y);
		cairo_rotate(ctx, -3.14159 / 2);
		cairo_show_text(ctx, tunnelstation);
		cairo_rotate(ctx, 3.14159 / 2);
		cairo_move_to(ctx, x, y);
		cairo_rel_move_to(ctx, w * ((spacing[i] + spacing[i + 1]) / 2 * (1.0 - DIST_FROM_LEFT - DIST_FROM_RIGHT - spacing[0] / 2)), 0);
	}
	cairo_restore(ctx);

	//Draw labels & ticks for the test section
	cairo_save(ctx);
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ctx, 0.018 * w);

	cairo_text_extents(ctx, "psi", &te);
	cairo_move_to(ctx, w * (DIST_FROM_LEFT / 2) - te.width / 2 - te.x_bearing, h * (1 - DIST_FROM_BOTTOM) + 1.5 * te.height);
	cairo_show_text(ctx, "psi");
	cairo_set_line_width(ctx, 2);
	number_of_steps = floor((SECT_SCALE_TOP - SECT_SCALE_BOT) / SECT_STEP_SIZE);
	cairo_restore(ctx);

	cairo_save(ctx);
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ctx, 0.008 * w);
	/*	
	cairo_text_extents(ctx, "Designed & Built at 8x6/9x15 SWT/LSWT", &te);
	cairo_move_to(ctx, w * (1-DIST_FROM_RIGHT / 2) - te.width / 1 - te.x_bearing, h * ( DIST_FROM_BOTTOM) + 1.5 * te.height);
	cairo_show_text(ctx, "Designed & Built at 8x6/9x15 SWT/LSWT");
	cairo_set_line_width(ctx, 2);
	number_of_steps = floor((SECT_SCALE_TOP - SECT_SCALE_BOT) / SECT_STEP_SIZE);
*/
	cairo_restore(ctx);

	//Draw lines

	for (i = 0; i <= number_of_steps; i++) {

		if (i % SECT_STEP_LABEL == 0) {
			cairo_move_to(ctx, w * (DIST_FROM_LEFT - 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (1 - DIST_FROM_RIGHT + 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_stroke(ctx);
			snprintf(axis_label, 4, "%d", i + SECT_SCALE_BOT);
			cairo_text_extents(ctx, axis_label, &te);
			cairo_move_to(ctx, w * ((0) + DIST_FROM_LEFT / 2) - te.width / 2 - te.x_bearing, h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)) - te.height / 2 - te.y_bearing);
			cairo_show_text(ctx, axis_label);
		} else {
			cairo_move_to(ctx, w * (DIST_FROM_LEFT - 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (DIST_FROM_LEFT), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_move_to(ctx, w * (1 - DIST_FROM_RIGHT + 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (1 - DIST_FROM_RIGHT), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_stroke(ctx);
		}
	}
	//Draw ticks

	for (i = 0; i / 4 < number_of_steps; i++) {
		cairo_move_to(ctx, w * (DIST_FROM_LEFT - 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps / 4)));
		cairo_line_to(ctx, w * (DIST_FROM_LEFT), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps / 4)));
		cairo_stroke(ctx);
	}

	//Draw the dots for each port location on the tunnel diagram.

	cairo_set_source_rgba(ctx, 1, 0.95, 0.4, 1); //Nice yellow color.

	x = w * (DIST_FROM_LEFT + spacing[0] / 2);
	y = h * (0.06 + DIST_FROM_TOP - 0.01) / 2;
	for (i = 0; i < SECT_PRES_NUMBER; i++) {
		double bar_space = floor(w * ((spacing[i] + spacing[i + 1]) / 2 * (1.0 - DIST_FROM_LEFT - DIST_FROM_RIGHT - spacing[0] / 2)));
		cairo_arc(ctx, x, y, 0.002*w, 0, 2*M_PI);
		cairo_fill(ctx);
		x=x+bar_space;
	}
	cairo_pop_group_to_source(ctx);
	cairo_paint(ctx);
}

// Draw the bar charts, takes the screen widths only. Everything else is defined by constants within the function.

void draw_dynamics(cairo_t* ctx, int w, int h)
{
	int i = 0;
	cairo_text_extents_t te;
	char header[28];
	time_t current_time;
	double x, y;

	cairo_push_group(ctx);
	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ctx, w * 0.02);

	time(&current_time);
	snprintf(header, 25, "%s", ctime(&current_time));
	cairo_text_extents(ctx, header, &te);
	cairo_move_to(ctx, w * 0.01, h * 0.04);
	cairo_show_text(ctx, header);

	snprintf(header, 25, "Atm press: %2.3f", ref_press);
	cairo_text_extents(ctx, header, &te);
	cairo_move_to(ctx, w * (1 - 0.2), h * 0.04);
	cairo_show_text(ctx, header);

	//Draw Main compressor pressures

	snprintf(header, 25, "Main Inlet: %2.2f", comp_pres[0]);
	cairo_text_extents(ctx, header, &te);
	cairo_move_to(ctx, w * DIST_FROM_LEFT, h * 0.09 + te.height);
	cairo_show_text(ctx, header);

	snprintf(header, 25, "Main Exit: %2.2f", comp_pres[1]);
	cairo_text_extents(ctx, header, &te);
	cairo_move_to(ctx, w * DIST_FROM_LEFT, h * 0.09 + 2.5 * te.height);
	cairo_show_text(ctx, header);

	snprintf(header, 25, "Main Ratio: %2.3f", comp_pres[1] / comp_pres[0]);
	cairo_text_extents(ctx, header, &te);
	cairo_move_to(ctx, w * DIST_FROM_LEFT, h * 0.09 + 4 * te.height);
	cairo_show_text(ctx, header);

	//Draw Secondary compressor pressures

	snprintf(header, 25, "Sec Inlet: %2.2f", comp_pres[2]);
	cairo_text_extents(ctx, header, &te);
	cairo_move_to(ctx, w * (1 - DIST_FROM_RIGHT - 0.15), h * 0.09 + te.height);
	cairo_show_text(ctx, header);

	snprintf(header, 25, "Sec Exit: %2.2f", comp_pres[3]);
	cairo_move_to(ctx, w * (1 - DIST_FROM_RIGHT - 0.15), h * 0.09 + 2.5 * te.height);
	cairo_show_text(ctx, header);

	snprintf(header, 25, "Sec Ratio: %2.3f", comp_pres[3] / comp_pres[2]);
	cairo_move_to(ctx, w * (1 - DIST_FROM_RIGHT - 0.15), h * 0.09 + 4 * te.height);
	cairo_show_text(ctx, header);

	//Draw all of the test section bar graphs
	cairo_move_to(ctx, w * (DIST_FROM_LEFT + spacing[0] / 2), h * (1 - DIST_FROM_BOTTOM)); //Beginning location of the bar graphs

	for (i = 0; i < SECT_PRES_NUMBER; i++) {
		double bar_height = h * ((((sect_pres[i] > 6) ? 6 : sect_pres[i]) - SECT_SCALE_BOT) / (SECT_SCALE_TOP - SECT_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP));
		double bar_width = floor((spacing[i] * w * (1 - DIST_FROM_LEFT - DIST_FROM_RIGHT - (spacing[0] + spacing[28]) / 2) - 3));
		double bar_space = floor(w * ((spacing[i] + spacing[i + 1]) / 2 * (1.0 - DIST_FROM_LEFT - DIST_FROM_RIGHT - spacing[0] / 2)));
		cairo_rel_line_to(ctx, 0, -bar_height);
		cairo_rel_move_to(ctx, 0, bar_height);
		cairo_get_current_point(ctx, &x, &y);
		cairo_set_source_rgba(ctx, 81 / 255.0, 248 / 255.0, 227 / 255.0, 0.6); //Nice transparent tealish color or the test section bar graphs
		cairo_set_line_width(ctx, bar_width);
		cairo_stroke(ctx);
		cairo_rectangle(ctx, x - bar_width / 2, y, bar_width, -bar_height);
		cairo_set_source_rgba(ctx, 81 / 255.0, 248 / 255.0, 227 / 255.0, 1); //Make the bar border opaque
		cairo_set_line_width(ctx, 2);
		cairo_stroke(ctx);
		cairo_move_to(ctx, x + bar_space, y);
	}

	cairo_set_source_rgba(ctx, 1, 0.95, 0.4, 1); //Nice yellowish color or the test section
	cairo_set_line_width(ctx, 2);
	cairo_fill(ctx);
	//Paint the dynamic stuff to the dynamic surface
	cairo_pop_group_to_source(ctx);
	cairo_paint(ctx);
}
