/*
 * xgbsplay is an X11 frontend for gbsplay, the Gameboy sound player
 *
 * 2003-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <signal.h>

#include <X11/keysymdef.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <cairo/cairo-xcb.h>

#include "player.h"

#define XCB_STRING_FORMAT 8 /* 8, 16 or 32 (bits per character?) */
#define XCB_EVENT_SYNTHETIC 0x80
#define XCB_EVENT_MASK (~(XCB_EVENT_SYNTHETIC))

/*
 * Virtual LCD has double the real Gameboy pixel count for usability
 * With an 8x16 font this gives us 
 */
#define VLCD_WIDTH 320
#define VLCD_HEIGHT 288

/* Outer padding size */
#define VLCD_OUT_HPAD 14
#define VLCD_OUT_VPAD 13

/* Inner padding size */
#define VLCD_IN_HPAD 36
#define VLCD_IN_VPAD 35

/* LCD decoration (from outer) */
#define VLCD_DECO_HPAD 14
#define VLCD_DECO_VPAD 13

/* Top decoration (from edge) */
#define TOP_DECO_PAD 14
#define TOP_DECO_SIZE 4

#define MIN_WIDTH (VLCD_WIDTH + 2 * (VLCD_OUT_HPAD + VLCD_IN_HPAD))
#define MIN_HEIGHT (120 + TOP_DECO_PAD + TOP_DECO_SIZE + VLCD_HEIGHT + 2 * (VLCD_OUT_VPAD + VLCD_IN_VPAD))

#define STATUSTEXT_LENGTH 256

/* global variables */
static char statustext[STATUSTEXT_LENGTH];

static long quit = 0;
static long screen_dirty = 0;
static long screen_modified = 0;

static uint32_t window_width = MIN_WIDTH;
static uint32_t window_height = MIN_HEIGHT;

static xcb_get_keyboard_mapping_reply_t *keymap;
static xcb_connection_t *conn;
static xcb_screen_t *screen;
static xcb_window_t window;
static cairo_surface_t *surface;
static cairo_t *cr;
static cairo_matrix_t default_matrix;

static xcb_atom_t atomWmDeleteWindow = XCB_ATOM_NONE;
static xcb_atom_t atomWmProtocols = XCB_ATOM_NONE;
static xcb_atom_t atomWmName = XCB_ATOM_NONE;

static const struct gbs_metadata *metadata;

static const struct gbs_status *status;
static struct displaytime displaytime;
static long last_seconds = -1;
static long last_subsong = -1;

static long has_status_changed(struct gbs *gbs)
{
	status = gbs_get_status(gbs);
	update_displaytime(&displaytime, status);

	if (displaytime.played_sec != last_seconds || status->subsong != last_subsong) {
		last_seconds = displaytime.played_sec;
		last_subsong = status->subsong;
		return 1;
	}

	return 0;
}

static void update_title()
{
	int len;

	len = snprintf(statustext, STATUSTEXT_LENGTH, /* or use sizeof(statustext) ?? */
		 "xgbsplay %s %d/%d "
		 "%02ld:%02ld/%02ld:%02ld",
		 filename, status->subsong+1, status->songs,
		 displaytime.played_min, displaytime.played_sec, displaytime.total_min, displaytime.total_sec);

	xcb_icccm_set_wm_name(conn, window, XCB_ATOM_STRING, XCB_STRING_FORMAT, len, statustext);
	xcb_flush(conn);
}

void exit_handler(int signum)
{
	printf(_("\nCaught signal %d, exiting...\n"), signum);
	exit(1);
}

static void draw_top(double pad, double height)
{
	double bottom = pad + height;
	double left = 0;
	double right = MIN_WIDTH;
	double top = 0;

	cairo_set_line_width(cr, 1.0);

	// #6a4f41
	cairo_set_source_rgb(cr, 0x6a/255.0, 0x4f/255.0, 0x41/255.0);
	cairo_move_to(cr, left, top + pad);
	cairo_line_to(cr, left + pad, top + pad);
	cairo_move_to(cr, left + bottom, top);
	cairo_line_to(cr, left + bottom, top + pad);
	cairo_line_to(cr, right - bottom, top + pad);
	cairo_move_to(cr, right - pad, top);
	cairo_line_to(cr, right - pad, top + pad);
	cairo_line_to(cr, right, top + pad);
	cairo_stroke(cr);

	// #dac1ad
	cairo_set_source_rgb(cr, 0xda/255.0, 0xc1/255.0, 0xad/255.0);
	cairo_move_to(cr, left + pad, top);
	cairo_line_to(cr, left + pad, top + pad);
	cairo_move_to(cr, right - bottom, top);
	cairo_line_to(cr, right - bottom, top + pad);
	cairo_move_to(cr, left, top + bottom);
	cairo_line_to(cr, right, top + bottom);
	cairo_stroke(cr);
}

static void draw_screen_outer(double left, double top, double r1, double r2)
{
	double right = MIN_WIDTH - left;
	// double width = MIN_WIDTH - 2 * left;
	double height = VLCD_HEIGHT + 2 * VLCD_IN_VPAD;
	double bottom = top + height;

	// #ad484e
	cairo_set_source_rgb(cr, 0x4d/255.0, 0x48/255.0, 0x4e/255.0);
	cairo_arc(cr, left + r1, top + r1, r1, 2*(M_PI/2), 3*(M_PI/2));
	cairo_arc(cr, right - r1, top + r1, r1, 3*(M_PI/2), 4*(M_PI/2));
	cairo_arc(cr, right - r2, bottom - r2, r2, 0*(M_PI/2), 1*(M_PI/2));
	cairo_arc(cr, left + r1, bottom - r1, r1, 1*(M_PI/2), 2*(M_PI/2));
	cairo_close_path(cr);
	cairo_fill(cr);
}

static void draw_screen_inner(double left, double top)
{
	double width = MIN_WIDTH - 2 * left;
	double height = width * 144.0 / 160.0;

	// #917702
	cairo_set_source_rgb(cr, 0x91/255.0, 0x77/255.0, 0x02/255.0);
	cairo_rectangle(cr, left, top, width, height);
	cairo_fill(cr);
}

static void draw_screen_deco(double left, double top, double dist)
{
	double right = MIN_WIDTH - left;
	double bottom = top + dist;

	cairo_set_line_width(cr, 2.0);

	// #62223a
	cairo_set_source_rgb(cr, 0x62/255.0, 0x22/255.0, 0x3a/255.0);
	cairo_move_to(cr, left, top);
	cairo_line_to(cr, right, top);
	cairo_stroke(cr);

	// #151333
	cairo_set_source_rgb(cr, 0x15/255.0, 0x13/255.0, 0x33/255.0);
	cairo_move_to(cr, left, bottom);
	cairo_line_to(cr, right, bottom);
	cairo_stroke(cr);

	/*
	"DOT MATRIX"
	// #99949a
	cairo_set_source_rgb(cr, 0x99/255.0, 0x94/255.0, 0x9a/255.0);
	*/
}

static void draw_screen_line(double vx, double vy, const char *s)
{
	cairo_move_to(cr, 50 + 8 * vx, 70 + 16 + 16 * vy);
	cairo_show_text(cr, s);
	cairo_stroke(cr);
}

static void draw_screen_linef(double vx, double vy, const char *fmt, ...)
{
	char buf[41];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	buf[sizeof(buf) - 1] = 0;
	draw_screen_line(vx, vy, buf);
}

static void draw_screen_content(struct gbs *gbs)
{
	// #0b433a
	cairo_set_source_rgb(cr, 0x0b/255.0, 0x43/255.0, 0x3a/255.0);
	cairo_select_font_face(cr, "Biwidth", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 16);

	draw_screen_line(0, 0, metadata->title);
	draw_screen_line(0, 1, metadata->author);
	draw_screen_line(0, 2, metadata->copyright);

	draw_screen_linef(0, 4, "Song %3d/%3d", status->subsong+1, status->songs);
	draw_screen_linef(0, 5, "%02ld:%02ld/%02ld:%02ld", displaytime.played_min, displaytime.played_sec, displaytime.total_min, displaytime.total_sec);

	draw_screen_line(0, 7, "[p]revious/[n]ext subsong   [q]uit");
	draw_screen_line(0, 8, "[ ] pause/resume   [1-4] mute ch");
}

static void redraw(struct gbs *gbs)
{
	// #a9988e
	cairo_set_source_rgb(cr, 0xa8/255.0, 0x98/255.0, 0x8e/255.0);
	cairo_paint(cr);

	draw_top(TOP_DECO_PAD, TOP_DECO_SIZE);
	draw_screen_outer(VLCD_OUT_HPAD,
			  TOP_DECO_PAD + TOP_DECO_SIZE + VLCD_OUT_VPAD,
			  15 /* small corner radius */,
			  50 /* big corner radius */);
	draw_screen_deco(VLCD_OUT_HPAD + VLCD_DECO_HPAD,
			 TOP_DECO_PAD + TOP_DECO_SIZE + VLCD_OUT_VPAD + VLCD_DECO_VPAD,
			 8 /* deco height */);
	draw_screen_inner(VLCD_OUT_HPAD + VLCD_IN_HPAD,
			  TOP_DECO_PAD + TOP_DECO_SIZE + VLCD_OUT_VPAD + VLCD_IN_VPAD);
	draw_screen_content(gbs);

	// #191a41
	cairo_set_source_rgb(cr, 0x19/255.0, 0x1a/255.0, 0x41/255.0);
	cairo_move_to(cr, 14, 445 + 30);
	cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 30);
	cairo_show_text(cr, "XGbsplay");
	cairo_stroke(cr);

	cairo_identity_matrix(cr);
	cairo_scale(cr,
		    (float)window_width / (float)MIN_WIDTH,
		    (float)window_height / (float)MIN_HEIGHT);

	cairo_surface_flush(surface);
}

static int handle_button(xcb_button_release_event_t *bev, struct gbs *gbs)
{
	fprintf(stderr, "release root: %d,%d event: %d,%d\n",
		bev->root_x, bev->root_y,
		bev->event_x, bev->event_y);

	/* TODO: Have things to click on */

	return 0;
}

static void icccm_init(xcb_window_t win)
{
	xcb_size_hints_t size_hints;
	static const char name[] = "xgbsplay";
	xcb_atom_t protocols[] = { atomWmDeleteWindow };
	memset(&size_hints, 0, sizeof(size_hints));
	xcb_icccm_set_wm_protocols(conn, win, atomWmProtocols, ARRAY_SIZE(protocols), protocols);
	xcb_icccm_set_wm_name(conn, win, XCB_ATOM_STRING, XCB_STRING_FORMAT, strlen(name), name);
	xcb_icccm_size_hints_set_aspect(&size_hints, MIN_WIDTH, MIN_HEIGHT, MIN_WIDTH, MIN_HEIGHT);
	xcb_icccm_size_hints_set_min_size(&size_hints, MIN_WIDTH, MIN_HEIGHT);
	xcb_icccm_set_wm_size_hints(conn, window, XCB_ATOM_WM_NORMAL_HINTS, &size_hints);
}

static xcb_visualtype_t *find_visual(xcb_screen_t *screen)
{
	xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
	for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
		xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
		for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
			if (screen->root_visual == visual_iter.data->visual_id) {
				return visual_iter.data;
			}
		}
	}

	return NULL;
}

static xcb_intern_atom_cookie_t request_atom(const char *name)
{
	return xcb_intern_atom_unchecked(conn, 0 /* create if it does not exist */, strlen(name), name);
}

static const char *debug_atom_name(xcb_atom_t atom)
{
	xcb_get_atom_name_cookie_t c = xcb_get_atom_name_unchecked(conn, atom);
	xcb_get_atom_name_reply_t *ar = xcb_get_atom_name_reply(conn, c, NULL);
	static char buf[23];
	const char *name;
	size_t name_len;
	if (atom == XCB_ATOM_NONE) {
		return "XCB_ATOM_NONE";
	}
	if (!ar) {
		return "<unknown atom>";
	}
	name = xcb_get_atom_name_name(ar);
	name_len = xcb_get_atom_name_name_length(ar);
	if (name_len > sizeof(buf) - 1) {
		name_len = sizeof(buf) - 1;
	}
	strncpy(buf, name, name_len);
	buf[name_len] = 0;
	free(ar);
	return buf;
}

static xcb_atom_t get_atom(xcb_intern_atom_cookie_t cookie)
{
	xcb_intern_atom_reply_t *r = xcb_intern_atom_reply(conn, cookie, NULL);
	xcb_atom_t atom;
	if (!r) {
		return XCB_ATOM_NONE;
	}
	atom = r->atom;
	free(r);
	return atom;
}

static void setup_atoms()
{
	xcb_intern_atom_cookie_t atomc[3];

	atomc[0] = request_atom("WM_DELETE_WINDOW");
	atomc[1] = request_atom("WM_PROTOCOLS");
	atomc[2] = request_atom("WM_NAME");

	atomWmDeleteWindow = get_atom(atomc[0]);
	atomWmProtocols = get_atom(atomc[1]);
	atomWmName = get_atom(atomc[2]);
}

static void setup_keymap()
{
	const xcb_setup_t *setup = xcb_get_setup(conn);
	xcb_get_keyboard_mapping_cookie_t kmc = xcb_get_keyboard_mapping(
		conn, setup->min_keycode,
		setup->max_keycode - setup->min_keycode + 1);
	keymap = xcb_get_keyboard_mapping_reply(conn, kmc, NULL);
}

static int state_to_plane(uint16_t state)
{
	if (state & XCB_KEY_BUT_MASK_SHIFT) {
		return 1;
	}
	return 0;
}

static xcb_keysym_t lookup_keysym(xcb_keycode_t code, uint16_t state)
{
	const xcb_setup_t *setup = xcb_get_setup(conn);
	xcb_keysym_t *keysyms = xcb_get_keyboard_mapping_keysyms(keymap);
	int          idx;

	if (code > setup->max_keycode || code < setup->min_keycode) {
		return XCB_NO_SYMBOL;
	}

	idx = (code - setup->min_keycode) * keymap->keysyms_per_keycode
		+ state_to_plane(state);
	if (idx >= keymap->length) {
		return XCB_NO_SYMBOL;
	}
	return keysyms[idx];
}

static void handle_user_input(struct gbs *gbs, char c)
{

	switch (c) {
	case 'p':
		play_prev_subsong(gbs);
		break;

	case 'n':
		play_next_subsong(gbs);
		break;
	case 'q':
	case 27:
		quit = 1;
		break;
	case ' ':
		toggle_pause(gbs);
		break;
	case '1':
	case '2':
	case '3':
	case '4':
		gbs_toggle_mute(gbs, c-'1');
		break;
	}
}

int main(int argc, char **argv)
{
	uint32_t value_mask;
	uint32_t value_list[1];
	xcb_generic_event_t *event;
	xcb_visualtype_t *visual;
	struct gbs *gbs;
	struct sigaction sa;

	gbs = common_init(argc, argv);
	metadata = gbs_get_metadata(gbs);

	/* init signal handlers */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = exit_handler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	/* init X11 */
	conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		fprintf(stderr, "Could not connect to X server: XCB error %d\n",
			xcb_connection_has_error(conn));
		exit(1);
	}

	setup_atoms();
	setup_keymap();

	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	visual = find_visual(screen);
	if (visual == NULL) {
		fprintf(stderr, "Could not find visual\n");
		exit(1);
	}
	window = xcb_generate_id(conn);
	value_mask = XCB_CW_EVENT_MASK;
	value_list[0] = XCB_EVENT_MASK_EXPOSURE \
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY \
		| XCB_EVENT_MASK_PROPERTY_CHANGE \
		| XCB_EVENT_MASK_KEY_PRESS \
		| XCB_EVENT_MASK_BUTTON_RELEASE;
	xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root,
			  0, 0, window_width, window_height, 0 /* border_width */,
			  XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  screen->root_visual,
			  value_mask, value_list);
	xcb_map_window(conn, window);
	icccm_init(window);

	surface = cairo_xcb_surface_create(conn, window, visual, window_width, window_height);
	cr = cairo_create(surface);
	cairo_get_matrix(cr, &default_matrix);

	xcb_flush(conn);

	/* main loop */
	while (!quit) {
		if (!step_emulation(gbs)) {
			quit = 1;
			break;
		}

		while ((event = xcb_poll_for_event(conn))) {
			switch (event->response_type & XCB_EVENT_MASK) {

			case XCB_EXPOSE: {
				// xcb_expose_event_t *eev = (xcb_expose_event_t *) event;
				screen_dirty = 1;
				screen_modified = 1;
				break;
				}
			case XCB_KEY_PRESS: {
				xcb_key_press_event_t *kev = (xcb_key_press_event_t *) event;
				xcb_keysym_t sym = lookup_keysym(kev->detail, kev->state);
				fprintf(stderr, "key press (key=%d, state=%d, sym=%04x)\n", kev->detail, kev->state, sym);
				handle_user_input(gbs, sym);
				break;
				}
			case XCB_KEY_RELEASE:
				break;
			case XCB_BUTTON_RELEASE: {
				xcb_button_release_event_t *bev = (xcb_button_release_event_t *) event;
				if (handle_button(bev, gbs))
					quit = 1;
				break;
				}
			case XCB_PROPERTY_NOTIFY: {
				xcb_property_notify_event_t *pnev = (xcb_property_notify_event_t *) event;
				if (pnev->atom == atomWmName) {
					break;
				}
				fprintf(stderr, "property notify: %s\n", debug_atom_name(pnev->atom));
				break;
				}
			case XCB_CLIENT_MESSAGE: {
				int i;
				xcb_client_message_event_t *cmev = (xcb_client_message_event_t *) event;
				if (cmev->type == atomWmProtocols && cmev->data.data32[0] == atomWmDeleteWindow) {
					quit = 1;
					break;
				}
				fprintf(stderr, "client message: %s\n", debug_atom_name(cmev->type));
				for (i=0; i < ARRAY_SIZE(cmev->data.data32); i++) {
					fprintf(stderr, "data%d: %08x (%s)\n",
						i, cmev->data.data32[i],
						debug_atom_name(cmev->data.data32[i]));
				}
				break;
				}
			case XCB_CONFIGURE_NOTIFY: {
				screen_modified = 1;
				screen_dirty = 1;
				break;
				}
			case XCB_MAP_NOTIFY:
				/* Ignored event */
				break;
			default: {
				fprintf(stderr, "unhandled event %d\n", event->response_type & XCB_EVENT_MASK);
				break;
				}
			}
			free(event);
			xcb_flush(conn);
		}

		if (screen_modified) {
			xcb_get_geometry_cookie_t c = xcb_get_geometry(conn, window);
			xcb_get_geometry_reply_t *r = xcb_get_geometry_reply(conn, c, NULL);
			window_width = r->width;
			window_height = r->height;
			free(r);
			cairo_xcb_surface_set_size(surface, window_width, window_height);
			screen_dirty = 1;
			screen_modified = 0;
		}
		if (is_running() && has_status_changed(gbs)) {
			update_title();
			screen_dirty = 1;
		}
		if (screen_dirty) {
			redraw(gbs);
			xcb_flush(conn);
			screen_dirty = 0;
			{
				xcb_get_geometry_cookie_t c = xcb_get_geometry(conn, window);
				xcb_get_geometry_reply_t *r = xcb_get_geometry_reply(conn, c, NULL);
				if (r->width != window_width || r->height != window_height) {
					screen_modified = 1;
				}
			}
		}
	}

	/* stop sound */
	common_cleanup(gbs);

	/* clean up X11 stuff */
	cairo_surface_finish(surface);
	cairo_surface_destroy(surface);
	xcb_disconnect(conn);

	return 0;
}
