#include "utils.h"
#include "animated-pointer.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void sync_window_pos_and_mouse(cairo_surface_t* surface, Point* mouse_pos_ret) {
		
		Display* display = cairo_xlib_surface_get_display(surface);
		Window window = cairo_xlib_surface_get_drawable(surface);
		Window root;
		int x, y;
		unsigned int width, height, border_width, depth;
		XGetGeometry(display, window, &root, &x, &y, &width, &height, &border_width, &depth); 

		Window child;
		int rx, ry, mx, my;
    unsigned int mask;
		XQueryPointer(display, window, &root, &child, &rx, &ry, &mx, &my, &mask);

		// Place the cursor at the center of the window.
		rx -= width / 2;
		ry -= height / 2;

		// Since this is a floating window and we move it around, the inner mouse cursor pos will always be the center of the window
		mouse_pos_ret->x = x + width / 2;
		mouse_pos_ret->y = y + height / 2;

		XMoveWindow(display, window, rx, ry);
		
}

cairo_surface_t* create_cairo_surface() {
		
		Display* display;

		if (( display = XOpenDisplay(NULL)) == NULL) {
				fprintf(stderr, "Error: Failed to open a display");
				exit(1);
		}
		
		Drawable drawable;
		cairo_surface_t *surface;
		Screen* screen = DefaultScreenOfDisplay(display);
		int screenNumber = XScreenNumberOfScreen(screen);
		Window root_window = DefaultRootWindow(display);

		XVisualInfo vinfo;
		XMatchVisualInfo(display, screenNumber, 32, TrueColor, &vinfo);

		XSetWindowAttributes attr;
		attr.colormap = XCreateColormap(display, root_window, vinfo.visual, AllocNone);
    attr.border_pixel = 0;
		attr.background_pixmap = ParentRelative;
    attr.background_pixel = 0x80808080;
		attr.save_under = true;
		attr.override_redirect = 1;
		
		drawable = XCreateWindow(display, root_window, 0, 0, 500, 500, 0, vinfo.depth, InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
		
		XSelectInput(display, drawable, PointerMotionMask | ButtonPressMask | KeyPressMask); 
		XMapWindow(display, drawable);

		surface = cairo_xlib_surface_create(display, drawable, vinfo.visual, 0, 0);

		cairo_xlib_surface_set_size(surface, 500, 500);
		
		XClassHint* class = XAllocClassHint();
		if (class != NULL) {
				class->res_name = "animated-pointer";
				class->res_class = "animated-pointer";
				XSetClassHint(display, drawable, class);
		}
		
		return surface;

}

void cleanup_surface(cairo_surface_t* surface) {
		Display* display;
		display = cairo_xlib_surface_get_display(surface);
		cairo_surface_destroy(surface);
		XCloseDisplay(display);
}


/**
 * Process the next XEvent.
 *
 * @param wait If true wait for next event
*/
int process_event_cairo(cairo_surface_t* surface, bool wait, Point* mouse_pos_ret) {

		char keybuf[8];
		XEvent event;
		KeySym key;

		XSync(cairo_xlib_surface_get_display(surface), false);

		for (;;) {
				if (wait || XPending(cairo_xlib_surface_get_display(surface))) {
						XNextEvent(cairo_xlib_surface_get_display(surface), &event);
				} else {
						return 0;
				}
				
				switch(event.type) {
						case ButtonPress:
								return -event.xbutton.button;
						case KeyPress:
								XLookupString(&event.xkey, keybuf, sizeof(keybuf), &key, NULL);
								return key;
						case MotionNotify:
								sync_window_pos_and_mouse(surface, mouse_pos_ret);
						default:
								return 0;
				}
				
		}
}

/**
 * Draw one frame of the pointer animation
 */
void draw_cairo(cairo_surface_t* surface, cairo_t* ctx, const int* counter, const Point* mouse_pos) {
		
		int scale = *counter % 10;
		
		cairo_save(ctx);
		cairo_set_operator(ctx, CAIRO_OPERATOR_CLEAR);
		cairo_paint(ctx); 
		cairo_restore(ctx);

		cairo_push_group(ctx);

		// center the matrix and set a transparent background
		cairo_save(ctx);
		cairo_translate(ctx, mouse_pos->x, mouse_pos->y);
		cairo_translate(ctx, -mouse_pos->x, -mouse_pos->y);
		cairo_restore(ctx);
		
		cairo_set_source_rgba(ctx, 0, 0, 0, 0);
		cairo_paint(ctx);
		
		// draw the mouse pointer
		cairo_set_line_join(ctx, CAIRO_LINE_JOIN_MITER);
		cairo_set_source_rgba(ctx, 0, 0, 0, 1);
		cairo_set_line_width(ctx, 2);
		cairo_move_to(ctx, (mouse_pos->x), (mouse_pos->y));

		cairo_rel_line_to(ctx, 15, 15);
		cairo_rel_line_to(ctx, -10, -2);
		cairo_rel_line_to(ctx, -5, 6);
		cairo_rel_line_to(ctx, 0, -19);

		cairo_stroke(ctx);

		cairo_set_source_rgba(ctx, 1, 0, 0, 1);

		cairo_fill(ctx);
		
		cairo_move_to(ctx, (mouse_pos->x) + 5, (mouse_pos->y) + 13);
		cairo_rel_line_to(ctx, 2, 2);
		
		cairo_stroke(ctx);
		
		cairo_pop_group_to_source(ctx);
		cairo_paint(ctx);
		cairo_surface_flush(surface);
		
}

int main(int argc, char **argv) {

		cairo_surface_t* surface;
		cairo_t* context;

		Point mouse_pos; // mouse position relative to the opened window

		struct timespec ts = {0, 5000000};
		
		bool is_running = true;
		int counter = 0;
		
		surface = create_cairo_surface();
		context = cairo_create(surface);
		sync_window_pos_and_mouse(surface, &mouse_pos); // initial sync

		while (is_running) {
				
				switch (process_event_cairo(surface, false, &mouse_pos)) {
						case 0xff1b:   // Esc
						case -1:       // left mouse button
								is_running = false;
								break;
				}

				draw_cairo(surface, context, &counter, &mouse_pos);

				counter++;
				nanosleep(&ts, NULL);

		}

		cleanup_surface(surface);

		return 0;

}
