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

static const float DIST_FROM_TOP = 0.37;
static const float DIST_FROM_BOTTOM = 0.06;
static const float DIST_FROM_LEFT = 0.04;
static const float DIST_FROM_RIGHT = 0.03;
static const float DIST_FROM_MIDDLE = 0.025;
static const float ATM_PRESS = 14.6959;
static const char* FONT_FACE = "Lato";
static const int COMP_SCALE_BOT = -5;
static const int COMP_SCALE_TOP = 15;
static const int COMP_STEP_SIZE = 1;
static const int COMP_STEP_LABEL = 5;
static const int SECT_SCALE_BOT = -15;
static const int SECT_SCALE_TOP = 5;
static const int SECT_STEP_LABEL = 5;
static const int SECT_STEP_SIZE = 1;
static const int COMP_PRES_NUMBER = 25;
static const int SECT_PRES_NUMBER = 39;

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
	char* window_title = "8x6 Manometer";

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

	return sfc;
}

// Create a random pressure between 14 & 15 PSI for testing

static float randompressure(void)
{
	float result;
	result = ((rand() % 100) / 100.00);
	return result;
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
	glob.image = cairo_image_surface_create_from_png("background.png");
	if (cairo_surface_status(glob.image) != CAIRO_STATUS_SUCCESS) {
		printf("Error loading ./background.png : %s \nUsing a black background. \n", cairo_status_to_string(cairo_surface_status(glob.image)));
		return 1;
	}
	return 0;
}

//Draw the static screen items

void draw_statics(cairo_t* ctx, int w, int h)
{

	int number_of_steps;
	double x_scaling = 0, y_scaling = 0;
	int i = 0;
	char axis_label[4];
	int error = 0;

	// Draw the Labels

	cairo_push_group(ctx);

	printf("Screen:  width = %d, height = %d \n", w, h);

	error = load_image();
	if (error == 0) {
		double image_width, image_height;
		image_width = cairo_image_surface_get_width(glob.image);
		image_height = cairo_image_surface_get_height(glob.image);
		x_scaling = w / image_width;
		y_scaling = h / image_height;

		printf("Image scaling:  x = %1.2f, y = %1.2f \n", x_scaling, y_scaling);
		cairo_scale(ctx, x_scaling, y_scaling);
		cairo_set_source_surface(ctx, glob.image, 0, 0);
		cairo_surface_destroy(glob.image);
	} else {
		cairo_set_source_rgb(ctx, 0, 0, 0);
	}
	cairo_paint(ctx);
	cairo_scale(ctx, 1 / x_scaling, 1 / y_scaling);

	// Draw the section labels

	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_text_extents_t te;
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(ctx, w * 0.025);

	cairo_text_extents(ctx, "Compressor Pressures", &te);
	cairo_move_to(ctx, w * (0.5) / 2 - te.width / 2 - te.x_bearing, h * (1 - DIST_FROM_BOTTOM) + h * 0.045);
	cairo_show_text(ctx, "Compressor Pressures");

	cairo_text_extents(ctx, "Test Section Pressures", &te);
	cairo_move_to(ctx, w * (0.5 + (0.5 - DIST_FROM_RIGHT) / 2) - te.width / 2 - te.x_bearing, h * (1 - DIST_FROM_BOTTOM) + h * 0.045);
	cairo_show_text(ctx, "Test Section Pressures");

	cairo_text_extents(ctx, "8x6 Manometer", &te);
	cairo_move_to(ctx, w * 0.5 - te.width / 2 - te.x_bearing, h * (0.01) + te.height);
	cairo_show_text(ctx, "8x6 Manometer");

	//Draw labels & ticks for the compressor

	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ctx, 0.018 * w);
	cairo_text_extents(ctx, "psi", &te);
	cairo_move_to(ctx, w * (DIST_FROM_LEFT / 2) - te.width / 2 - te.x_bearing, h * (1 - DIST_FROM_BOTTOM) + 1.5 * te.height);
	cairo_show_text(ctx, "psi");

	//Draw the ticks

	cairo_set_line_width(ctx, 2);

	number_of_steps = (COMP_SCALE_TOP - COMP_SCALE_BOT) / COMP_STEP_SIZE;

	for (i = 0; i <= number_of_steps; i++) {
		if (i % COMP_STEP_LABEL == 0) {
			cairo_move_to(ctx, w * (DIST_FROM_LEFT - 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (0.5 - DIST_FROM_MIDDLE + 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_stroke(ctx);
			snprintf(axis_label, 4, "%d", i + COMP_SCALE_BOT);
			cairo_text_extents(ctx, axis_label, &te);
			cairo_move_to(ctx, w * (DIST_FROM_LEFT / 2) - te.width / 2 - te.x_bearing, h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)) - te.height / 2 - te.y_bearing);
			cairo_show_text(ctx, axis_label);
		} else {
			cairo_move_to(ctx, w * (DIST_FROM_LEFT - 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * DIST_FROM_LEFT, h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_move_to(ctx, w * (0.5 - DIST_FROM_MIDDLE + 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (0.5 - DIST_FROM_MIDDLE), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_stroke(ctx);
		}
	}

	/*Draw the thin line at atmospheric pressures

	cairo_set_line_width(ctx, 1);
	cairo_move_to(ctx, w * (DIST_FROM_LEFT - 0.01), 0.5 + round(h * ((1.0 - DIST_FROM_BOTTOM) - (ATM_PRESS - COMP_SCALE_BOT) / (COMP_SCALE_TOP - COMP_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP))));
	cairo_line_to(ctx, w * (0.5 - DIST_FROM_MIDDLE + 0.01), 0.5 + round(h * ((1.0 - DIST_FROM_BOTTOM) - (ATM_PRESS - COMP_SCALE_BOT) / (COMP_SCALE_TOP - COMP_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP))));
	cairo_stroke(ctx);*/

	//Draw labels & ticks for the test section

	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ctx, 0.018 * w);
	cairo_text_extents(ctx, "psi", &te);
	cairo_move_to(ctx, w * (0.5) - te.width / 2 - te.x_bearing, h * (1 - DIST_FROM_BOTTOM) + 1.5 * te.height);
	cairo_show_text(ctx, "psi");

	cairo_set_line_width(ctx, 2);

	number_of_steps = floor((SECT_SCALE_TOP - SECT_SCALE_BOT) / SECT_STEP_SIZE);

	for (i = 0; i <= number_of_steps; i++) {

		if (i % SECT_STEP_LABEL == 0) {
			cairo_move_to(ctx, w * (0.5 + DIST_FROM_MIDDLE - 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (1 - DIST_FROM_RIGHT + 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_stroke(ctx);
			snprintf(axis_label, 4, "%d", i + SECT_SCALE_BOT);
			cairo_text_extents(ctx, axis_label, &te);
			cairo_move_to(ctx, w * ((0) + 0.5) - te.width / 2 - te.x_bearing, h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)) - te.height / 2 - te.y_bearing);
			cairo_show_text(ctx, axis_label);
		} else {
			cairo_move_to(ctx, w * (0.5 + DIST_FROM_MIDDLE - 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (0.5 + DIST_FROM_MIDDLE), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_move_to(ctx, w * (1 - DIST_FROM_RIGHT + 0.01), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_line_to(ctx, w * (1 - DIST_FROM_RIGHT), h * ((1.0 - DIST_FROM_BOTTOM) - i * ((1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP) / number_of_steps)));
			cairo_stroke(ctx);
		}
	}

	/*Draw the thin line at the atmospheric pressures

	cairo_set_line_width(ctx, 1);
	cairo_move_to(ctx, w * (0.5 + DIST_FROM_MIDDLE - 0.01), 0.5 + round(h * ((1.0 - DIST_FROM_BOTTOM) - (ATM_PRESS - SECT_SCALE_BOT) / (SECT_SCALE_TOP - SECT_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP))));
	cairo_line_to(ctx, w * (1 - DIST_FROM_RIGHT + 0.01), 0.5 + round(h * ((1.0 - DIST_FROM_BOTTOM) - (ATM_PRESS - SECT_SCALE_BOT) / (SECT_SCALE_TOP - SECT_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP))));
	cairo_stroke(ctx);*/

	cairo_pop_group_to_source(ctx);
	cairo_paint(ctx);
}

// Draw the bar charts, takes the screen widths only. Everything else is defined by constants within the function.

void draw_dynamics(cairo_t* ctx, int w, int h)
{

	int i = 0;
	float comp_pres[COMP_PRES_NUMBER]; //compressor section pressures.
	float sect_pres[SECT_PRES_NUMBER]; //test section pressures.
	cairo_text_extents_t te;
	char header[28];
	time_t current_time;

	cairo_push_group(ctx);
	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_select_font_face(ctx, FONT_FACE, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ctx, w * 0.02);
//	snprintf(header, 20, "Atm = %2.4f psi", ATM_PRESS);
//	cairo_text_extents(ctx, header, &te);
//	cairo_move_to(ctx, w * 0.99 - te.width, h * 0.01 + te.height);
//	cairo_show_text(ctx, header);

	time(&current_time);
	snprintf(header, 25, "%s", ctime(&current_time));
	cairo_text_extents(ctx, header, &te);
	cairo_move_to(ctx, w * 0.01, h * 0.01 + te.height);
	cairo_show_text(ctx, header);

	//Get pressures prior to displaying

	for (i = 0; i < COMP_PRES_NUMBER; i++) {
		comp_pres[i] = 5.0 + 5.0 * sin(-i / 7.0 - 3.14) + randompressure();
	}
	for (i = 0; i < SECT_PRES_NUMBER; i++) {
		sect_pres[i] = 5.0 * sin(-i / 14.0 - 5.5) + randompressure();
	}

	//Draw all of the compressor bar graphs

	const float C_BAR_WIDTH = (0.5 - DIST_FROM_LEFT - DIST_FROM_MIDDLE) / COMP_PRES_NUMBER; // Width of the compress bar graph bars

	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_set_line_width(ctx, 1);
	cairo_move_to(ctx, round(w * (DIST_FROM_LEFT + C_BAR_WIDTH * 6)), h * (1 - DIST_FROM_BOTTOM));
	cairo_line_to(ctx, round(w * (DIST_FROM_LEFT + C_BAR_WIDTH * 6)), h * 0.24);
	cairo_move_to(ctx, round(w * (DIST_FROM_LEFT + C_BAR_WIDTH * 22)), h * (1 - DIST_FROM_BOTTOM));
	cairo_line_to(ctx, round(w * (DIST_FROM_LEFT + C_BAR_WIDTH * 22)), h * 0.24);
	cairo_stroke(ctx);

	for (i = 0; i < COMP_PRES_NUMBER; i++) {
		cairo_set_line_width(ctx, -0.5 + round(w * (0.9 * C_BAR_WIDTH)));
		cairo_set_source_rgba(ctx, 0, 0.906, 1, 0.6); // Nice blueish color for the compressor
		cairo_move_to(ctx, w * (DIST_FROM_LEFT + C_BAR_WIDTH * (0.5 + i)), round(h * (1 - DIST_FROM_BOTTOM)));
		cairo_line_to(ctx, w * (DIST_FROM_LEFT + C_BAR_WIDTH * (0.5 + i)), round(h * ((1.0 - DIST_FROM_BOTTOM) - (comp_pres[i] - COMP_SCALE_BOT) / (COMP_SCALE_TOP - COMP_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP))));
		cairo_stroke(ctx);

		cairo_set_source_rgba(ctx, 0, 0.906, 1, 1);
		cairo_set_line_width(ctx, 2);
		cairo_rectangle(ctx, w * (DIST_FROM_LEFT + C_BAR_WIDTH * (i + 0.05)), round(h * (1 - DIST_FROM_BOTTOM)), w * 0.9 * C_BAR_WIDTH, round(-h * (comp_pres[i] - COMP_SCALE_BOT) / (COMP_SCALE_TOP - COMP_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP)));
		cairo_stroke(ctx);
	}

	//Draw all of the test section bar graphs

	const float S_BAR_WIDTH = (0.5 - DIST_FROM_MIDDLE - DIST_FROM_RIGHT) / SECT_PRES_NUMBER;

	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_set_line_width(ctx, 1);
	cairo_move_to(ctx, w * (0.5 + DIST_FROM_MIDDLE + S_BAR_WIDTH * 15), h * (1 - DIST_FROM_BOTTOM));
	cairo_line_to(ctx, w * (0.5 + DIST_FROM_MIDDLE + S_BAR_WIDTH * 15), h * 0.17);
	cairo_move_to(ctx, w * (0.5 + DIST_FROM_MIDDLE + S_BAR_WIDTH * 21), h * (1 - DIST_FROM_BOTTOM));
	cairo_line_to(ctx, w * (0.5 + DIST_FROM_MIDDLE + S_BAR_WIDTH * 21), h * 0.17);
	cairo_stroke(ctx);

	for (i = 0; i < SECT_PRES_NUMBER; i++) {
		cairo_set_line_width(ctx, -0.5 + round(w * S_BAR_WIDTH * 0.9));
		cairo_set_source_rgba(ctx, 1, 0.95, 0.4, 0.5); //Nice yellowish color or the test section
		cairo_move_to(ctx, w * (0.5 + DIST_FROM_MIDDLE + S_BAR_WIDTH * (0.5 + i)), h * (1 - DIST_FROM_BOTTOM));
		cairo_line_to(ctx, w * (0.5 + DIST_FROM_MIDDLE + S_BAR_WIDTH * (0.5 + i)), h * ((1.0 - DIST_FROM_BOTTOM) - (sect_pres[i] - SECT_SCALE_BOT) / (SECT_SCALE_TOP - SECT_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP)));
		cairo_stroke(ctx);
		cairo_set_source_rgba(ctx, 1, 0.95, 0.4, 1);
		cairo_set_line_width(ctx, 2);
		cairo_rectangle(ctx, w * (0.5 + DIST_FROM_MIDDLE + S_BAR_WIDTH * (i + 0.05)), round(h * (1 - DIST_FROM_BOTTOM)), w * 0.9 * S_BAR_WIDTH, round(-h * (sect_pres[i] - SECT_SCALE_BOT) / (SECT_SCALE_TOP - SECT_SCALE_BOT) * (1.0 - DIST_FROM_BOTTOM - DIST_FROM_TOP)));
		cairo_stroke(ctx);
	}
	cairo_pop_group_to_source(ctx);
	cairo_paint(ctx);
}
