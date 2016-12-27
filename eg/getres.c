// -----------------------------------------------------------
// Purpose : simply retrieve current X screen resolution and 
//    the size of current root window of the default 
//    screen of curent default window
// -----------------------------------------------------------

#include <X11/Xlib.h>
#include <stdio.h>

int getRootWindowSize(int *w, int *h)
{
 Display* pdsp = NULL;
 Window wid = 0;
 XWindowAttributes xwAttr;

 pdsp = XOpenDisplay( NULL );
 if ( !pdsp ) {
  fprintf(stderr, "Failed to open default display.\n");
  return -1;
 }

 wid = DefaultRootWindow( pdsp );
 if ( 0 > wid ) {
  fprintf(stderr, "Failed to obtain the root windows Id "
      "of the default screen of given display.\n");
  return -2;
 }
 
 Status ret = XGetWindowAttributes( pdsp, wid, &xwAttr );
 *w = xwAttr.width;
 *h = xwAttr.height;

 XCloseDisplay( pdsp );
 return 0;
}

int getScreenSize(int *w, int*h)
{

 Display* pdsp = NULL;
 Screen* pscr = NULL;

 pdsp = XOpenDisplay( NULL );
 if ( !pdsp ) {
  fprintf(stderr, "Failed to open default display.\n");
  return -1;
 }

    pscr = DefaultScreenOfDisplay( pdsp );
 if ( !pscr ) {
  fprintf(stderr, "Failed to obtain the default screen of given display.\n");
  return -2;
 }

 *w = pscr->width;
 *h = pscr->height;

 XCloseDisplay( pdsp );
 return 0;
}

int main()
{
 int w, h;

 getScreenSize(&w, &h);
 printf (" Screen:  width = %d, height = %d \n", w, h);

 getRootWindowSize(&w, &h);
 printf (" Root Window:  width = %d, height = %d \n", w, h);
 
 return 1;
 
}


/* gcc -o $@ $< -lX11 */


/* set ts=4 sts=4 tw=100 sw=4 */
