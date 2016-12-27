#!/usr/bin/env python

import gtk
import cairo

class MainWindow(gtk.Window):
    __gsignals__ = {
        'expose-event': 'override'
        }

    def __init__(self):
        super(MainWindow, self).__init__()

        self.set_app_paintable(True)
        # no window border
        self.set_decorated(False)

        # see if we can do transparency
        screen = self.get_screen()
        
        alphamap = screen.get_rgba_colormap()
        rgbmap   = screen.get_rgb_colormap()

        if alphamap is not None:
            self.set_colormap(alphamap)
        else:
            self.set_colormap(rgbmap)
            print 'sorry, no alpha channel available :('
            
        self.set_size_request(300, 200)

    def do_expose_event(self, event):
        # we're going to draw on a temporary surface
        # then copy to the window surface
        # you can also draw directly on the window
        width, height = self.get_size()
        
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        ctx = cairo.Context(surface)

        # background is gray and transparent
        ctx.set_source_rgba(.7, .7, .7, 0.75)
        ctx.paint()

        # red border
        ctx.set_line_width(3.0)
        ctx.rectangle(0, 0, width, height)
        ctx.set_source_rgba(1.0, 0.0, 0.0, .75)
        ctx.stroke()

        # now copy to our window surface
        dest_ctx = self.window.cairo_create()
        # only update what needs to be drawn
        dest_ctx.rectangle(event.area.x, event.area.y, 
                           event.area.width, event.area.height)
        dest_ctx.clip()
        # source operator means replace, don't draw on top of
        dest_ctx.set_operator(cairo.OPERATOR_SOURCE)
        dest_ctx.set_source_surface(surface, 0, 0)
        dest_ctx.paint()

        
if __name__ == "__main__":
    w = MainWindow()
    w.show()
    gtk.main()
