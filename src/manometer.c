#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <drawing.h>
#include <math.h>
#include <modbus_server.h>
#include <pthread.h>
#include <scanivalve.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <gperftools/profiler.h>

/*! Check for Xlib Mouse/Keypress events. All other events are discarded. 
 * @param sfc Pointer to Xlib surface.
 * @param block If block is set to 0, this function always returns immediately
 * and does not block. if set to a non-zero value, the function will block
 * until the next event is received.
 * @return The function returns 0 if no event occured (and block is set). A
 * positive value indicates that a key was pressed and the X11 key symbol as
 * defined in <X11/keysymdef.h> is returned. A negative value indicates a mouse
 * button event. -1 is button 1 (left button), -2 is the middle button, and -3
 * the right button.
 */
int _fCloseThreads = 1;

int cairo_check_event(cairo_surface_t* sfc, int block)
{
	char keybuf[8];
	KeySym key;
	XEvent e;

	Atom wmDeleteMessage = XInternAtom(cairo_xlib_surface_get_display(sfc), "WM_DELETE_WINDOW", False);

	XSetWMProtocols(cairo_xlib_surface_get_display(sfc), cairo_xlib_surface_get_drawable(sfc), &wmDeleteMessage, 1);

	for (;;) {
		if (block || XPending(cairo_xlib_surface_get_display(sfc)))
			XNextEvent(cairo_xlib_surface_get_display(sfc), &e);
		else
			return 0;

		switch (e.type) {
		case ButtonPress:
			return -e.xbutton.button;
		case KeyPress:
			XLookupString(&e.xkey, keybuf, sizeof(keybuf), &key, NULL);
			return key;
		case ClientMessage:
			if (e.xclient.data.l[0] == wmDeleteMessage)
				return 1;
			break;
		default:;
		}
	}
}

int main(int argc, char** argv)
{ 
//	ProfilerStart("./manometer.prof");
	cairo_surface_t* sfc;	  //XLib surface that is actually displayed on the screen.
	cairo_surface_t* backgrnd; //Image surface that is used to generate the background
	cairo_surface_t* comp;

	cairo_t* screen_ctx; //Context to draw to the Xlib surface
	cairo_t* back_ctx;   //Context to draw the background
	cairo_t* comp_ctx;

	pthread_t scanivalve_thread;
	int running = 1;
	int x, y;
	x = 00;
	y = 00;
	long t = 0;

	// Begin the threads for the server

	pthread_create(&scanivalve_thread, NULL, scanivalve, (void*)t);
	//pthread_create(&modbus_server_thread, NULL, modbus_server, (void*)t);

	//Initialize the X11 surface used for displaying to screen
	sfc = cairo_create_x11_surface(&x, &y);
	screen_ctx = cairo_create(sfc);

	//Initialize the surface the static items are drawn to
	backgrnd = cairo_image_surface_create(CAIRO_FORMAT_RGB24, x, y);
	back_ctx = cairo_create(backgrnd);

	//Initialize the surface that is used to combine the static and dynamic items
	comp = cairo_image_surface_create(CAIRO_FORMAT_RGB24, x, y);
	comp_ctx = cairo_create(comp);

	draw_statics(back_ctx, x, y); //Draw the background to the background surface

	while (running && _fCloseThreads) {

		cairo_set_source_surface(comp_ctx, backgrnd, 0, 0); // Copy the background to the compositing surface
		cairo_paint(comp_ctx);

		draw_dynamics(comp_ctx, x, y); //Draw the dynamic items to the compositing surface.

		cairo_set_source_surface(screen_ctx, comp, 0, 0); // Copy the compositing surface to the screen surface.
		cairo_paint(screen_ctx);

		cairo_surface_flush(sfc);

		switch (cairo_check_event(sfc, 0)) {
			case 0xff1b: // ESC
			case 0x51: // Q
			case 0x71: // q
//			case -1:	 // left mouse button
//			case 1:		 // Right mouse button
				running = 0;
				break;
		}

		nanosleep((const struct timespec[]){ { 0, 200000000L } }, NULL);
	}

	//Close the server threads

	_fCloseThreads = 0;
	pthread_join(scanivalve_thread, NULL);
	//pthread_join(modbus_server_thread, NULL);

	//Clean up cairo contexts and surfaces
	
	cairo_destroy(screen_ctx);
	cairo_close_x11_surface(sfc);
	cairo_destroy(back_ctx);
	cairo_surface_destroy(backgrnd);
	cairo_destroy(comp_ctx);
	cairo_surface_destroy(comp);
	cairo_debug_reset_static_data();
//	ProfilerStop();

	return 0;
}
