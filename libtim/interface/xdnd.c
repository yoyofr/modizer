/* 
 * Copyright (C) 2000-2004 the xine project
 * 
 * This file is part of xine, a unix video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * $Id: xdnd.c,v 1.1 2008/04/01 18:57:31 skeishi Exp $
 *
 *
 * Thanks to Paul Sheer for his nice xdnd implementation in cooledit.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* NetBSD needs this */
#endif
#include "common.h"
#include "xdnd.h"

#define XDND_VERSION 3

/* #undef DEBUG_DND */

/*
 * PRIVATES
 */

static int _is_atom_match(DndClass *xdnd, Atom **atom) {
  int i, j;
  
  for(i = 0; (*atom)[i] != 0; i++) {
    for(j = 0; j <= MAX_SUPPORTED_TYPE; j++) {
      if((*atom)[i] == xdnd->supported[j])
	return i;
    }
  }

  return -1;
}

/*
 * Send XdndFinished to 'window' from target 'from'
 */
static void _dnd_send_finished (DndClass *xdnd, Window window, Window from) {
  XEvent xevent;

  if((xdnd == NULL) || (window == None) || (from == None))
    return;

  memset(&xevent, 0, sizeof(xevent));
  xevent.xany.type                  = ClientMessage;
  xevent.xany.display               = xdnd->display;
  xevent.xclient.window             = window;
  xevent.xclient.message_type       = xdnd->_XA_XdndFinished;
  xevent.xclient.format             = 32;
  XDND_FINISHED_TARGET_WIN(&xevent) = from;

  XLockDisplay(xdnd->display);
  XSendEvent(xdnd->display, window, 0, 0, &xevent);
  XUnlockDisplay(xdnd->display);
  xdnd->in_progress = False;
}

static void unescape_string(char *src, char *dest) {
  char *s, *d;

  if((src == NULL) || (dest == NULL)) {
    fprintf(stderr, "unescape got NULL argument(s)\n");
    return;
  }

  if(!strlen(src))
    return;

  memset(dest, 0, sizeof(dest));
  s = src;
  d = dest;

  while(*s != '\0') {

    switch(*s) {
    case '%':
      if ((*(s) == '%') && (*(s + 1) != '%')) {
	char    buffer[5] = { '0', 'x', *(s + 1) , *(s + 2), '\0' };
	char   *p         = buffer;
	int     character = strtol(p, &p, 16);
	
	*d = character;
	s += 2;
      }
      else {
	*d++ = '%';
	*d = '%';
      }
      break;

    default:
      *d = *s;
      break;
    }
    s++;
    d++;
  }
  *d = '\0';
}

/*
 * WARNING: X unlocked function 
 */
static int _dnd_paste_prop_internal(DndClass *xdnd, Window from, 
				    Window insert, Atom prop, Boolean delete_prop) {
  long           nread;
  unsigned long  nitems;
  unsigned long  bytes_after;

  nread = 0;

  do {
    Atom      actual_type;
    int       actual_fmt;
    unsigned  char *s = NULL;

    if (XGetWindowProperty(xdnd->display, insert, prop, 
			nread / (sizeof(unsigned char *)), 65536,
			delete_prop, AnyPropertyType, 
			&actual_type, &actual_fmt, &nitems, &bytes_after,
			&s) != Success) 
	{
		if (s) XFree(s);
		return 1;
	}

    nread += nitems;
    if (!nread) {
      XFree(s);
      return 1;
    }

    /* Okay, got something, handle */
    if (*s != '\0') {
      /*
       * from manpage:
       * XGetWindowProperty always allocates one extra byte in prop_return
       * (even if the property is zero length) and sets it to zero so that
       * simple properties consisting of characters do not have to be copied
       * into yet another string before use.
       */
      char *p;
      int   plen;

      /* Extract all data, '\n' separated */
      p = strtok((char *)s, "\n");
      while (p != NULL) {

        plen = strlen(p) - 1;

        /* Cleanup end of string */
        while ((plen >= 0) && ((p[plen] == 10) || (p[plen] == 12) ||
                (p[plen] == 13)))
          p[plen--] = '\0';

        if (plen) {
          char *obuf;
          if (!strncmp(p, "file:", 5)) {
            obuf = (char *)safe_malloc(2*plen + 1);
            unescape_string(p, obuf);
          }
          else obuf = p;

#ifdef DEBUG_DND
          printf("GOT '%s'\n", obuf);
#endif

          if (xdnd->callback) xdnd->callback(obuf);
          if (obuf != p) {
            free(obuf);
            obuf = NULL;
          }
        }
        p = strtok(NULL, "\n");
      }
    }

    XFree(s);
  } while(bytes_after);

  return 0;
}

/*
 * Getting selections, using INCR if possible.
 */
static void _dnd_get_selection (DndClass *xdnd, Window from, Atom prop,
                                Window insert) {
  struct timeval  tv, tv_start;
  long            nread;
  unsigned long   bytes_after;
  Atom            actual_type;
  int             actual_fmt;
  unsigned long   nitems;
  unsigned char  *s = NULL;

  if((xdnd == NULL) || (prop == None))
    return;

  nread = 0;

  XLockDisplay(xdnd->display);
  if(XGetWindowProperty(xdnd->display, insert, prop, 0, 8, False, AnyPropertyType, 
			&actual_type, &actual_fmt, &nitems, &bytes_after, &s) != Success) {
    XFree(s);
    XUnlockDisplay(xdnd->display);
    return;
  }

  XFree(s);

  if(actual_type != xdnd->_XA_INCR) {
    (void) _dnd_paste_prop_internal(xdnd, from, insert, prop, True);
    XUnlockDisplay(xdnd->display);
    return;
  }

  XDeleteProperty(xdnd->display, insert, prop);
  gettimeofday(&tv_start, 0);

  for(;;) {
    long    t;
    fd_set  r;
    XEvent  xe;

    if(XCheckMaskEvent(xdnd->display, PropertyChangeMask, &xe)) {
      if((xe.type == PropertyNotify) && (xe.xproperty.state == PropertyNewValue)) {

	/* time between arrivals of data */
	gettimeofday (&tv_start, 0);

	if(_dnd_paste_prop_internal(xdnd, from, insert, prop, True))
	  break;
      }
    } else {
      tv.tv_sec  = 0;
      tv.tv_usec = 10000;
      FD_ZERO(&r);
      FD_SET(ConnectionNumber(xdnd->display), &r);
      select(ConnectionNumber(xdnd->display) + 1, &r, 0, 0, &tv);

      if(FD_ISSET(ConnectionNumber(xdnd->display), &r))
	continue;
    }
    gettimeofday(&tv, 0);
    t = (tv.tv_sec - tv_start.tv_sec) * 1000000L + (tv.tv_usec - tv_start.tv_usec);

    /* No data for five seconds, so quit */
    if(t > 5000000L) {
      XUnlockDisplay(xdnd->display); 
      return;
    }
  }

  XUnlockDisplay(xdnd->display);
}

/*
 * Get list of type from window (more than 3).
 */
static void _dnd_get_type_list (DndClass *xdnd, Window window, Atom **typelist) {
  Atom            type, *a;
  int             format;
  unsigned long   count, remaining, i;
  unsigned char  *data = NULL;

  *typelist = NULL;

  if((xdnd == NULL) || (window == None))
    return;

  XLockDisplay(xdnd->display);
  XGetWindowProperty(xdnd->display, window, xdnd->_XA_XdndTypeList, 0, 0x8000000L, 
		     False, XA_ATOM, &type, &format, &count, &remaining, &data);

  XUnlockDisplay(xdnd->display);

  if((type != XA_ATOM) || (format != 32) || (count == 0) || (!data)) {

    if(data) {
      XLockDisplay(xdnd->display);
      XFree(data);
      XUnlockDisplay(xdnd->display);
    }

    fprintf(stderr, "xdnd.c@%d: XGetWindowProperty failed in "
                    "xdnd_get_type_list - dnd->_XA_XdndTypeList = %ld\n",
                    __LINE__, xdnd->_XA_XdndTypeList);
    return;
  }

  *typelist = (Atom *)safe_malloc((count + 1) * sizeof(Atom));
  a = (Atom *) data;

  for(i = 0; i < count; i++)
    (*typelist)[i] = a[i];

  (*typelist)[count] = 0;

  XLockDisplay(xdnd->display);
  XFree(data);
  XUnlockDisplay(xdnd->display);
}

/*
 * Get list of type from window (3).
 */
static void _dnd_get_three_types (XEvent * xevent, Atom **typelist) {
  int i;

  *typelist = (Atom *)safe_malloc((XDND_THREE + 1) * sizeof(Atom) );

  for(i = 0; i < XDND_THREE; i++)
    (*typelist)[i] = XDND_ENTER_TYPE(xevent, i);
  /* although (*typelist)[1] or (*typelist)[2] may also be set to nill */
  (*typelist)[XDND_THREE] = 0;	
}

/*
 * END OF PRIVATES
 */

/*
 * Initialize Atoms, ...
 */
void init_dnd(Display *display, DndClass *xdnd) {
char *prop_names[_XA_ATOMS_COUNT] = {
  "XdndAware", /* _XA_XdndAware */
  "XdndEnter", /* _XA_XdndEnter */
  "XdndLeave", /* _XA_XdndLeave */
  "XdndDrop", /* _XA_XdndDrop */
  "XdndPosition", /* _XA_XdndPosition */
  "XdndStatus", /* _XA_XdndStatus */
  "XdndSelection", /* _XA_XdndSelection */
  "XdndFinished", /* _XA_XdndFinished */
  "XdndTypeList", /* _XA_XdndTypeList */
  "INCR", /* _XA_INCR */
  "WM_DELETE_WINDOW", /* _XA_WM_DELETE_WINDOW */
  "TiMidityXSelWindowProperty" /* TIMIDITY_PROTOCOL_ATOM */
};

char *mime_names[MAX_SUPPORTED_TYPE] = {
 "text/uri-list", /* supported[0] */
 "text/plain"  /* supported[1] */
};


  XLockDisplay(display);

  XInternAtoms(display, prop_names, _XA_ATOMS_COUNT, False, xdnd->Atoms);
  XInternAtoms(display, mime_names, MAX_SUPPORTED_TYPE, False, xdnd->supported);

  XUnlockDisplay(display);

  xdnd->display			= display;
  xdnd->version			= XDND_VERSION;
  xdnd->callback		= NULL;
  xdnd->dragger_typelist	= NULL;
  xdnd->desired			= 0;
  xdnd->in_progress		= False;
}

/*
 * Add/Replace the XdndAware property of given window. 
 */
int make_window_dnd_aware(DndClass *xdnd, Window window,
                          dnd_callback_t cb) {
  Status        status;
  /* Because we don't install an alternate error handler,
   * we'll never get the error messages.
   */

  if(!xdnd->display)
    return 0;

  XLockDisplay(xdnd->display);
  status = XChangeProperty(xdnd->display, window, xdnd->_XA_XdndAware, XA_ATOM,
			   32, PropModeReplace, (unsigned char *)&xdnd->version, 1);
  XUnlockDisplay(xdnd->display);
  
  if((status == BadAlloc) || (status == BadAtom) || 
     (status == BadMatch) || (status == BadValue) || (status == BadWindow)) {
    fprintf(stderr, "XChangeProperty() failed.\n");
    return 0;
  }
  
  XLockDisplay(xdnd->display);
  XChangeProperty(xdnd->display, window, xdnd->_XA_XdndTypeList, XA_ATOM, 32,
		  PropModeAppend, (unsigned char *)&xdnd->supported, 1);
  XUnlockDisplay(xdnd->display);
  
  if((status == BadAlloc) || (status == BadAtom) || 
     (status == BadMatch) || (status == BadValue) || (status == BadWindow)) {
    fprintf(stderr, "XChangeProperty() failed.\n");
    return 0;
  }

  xdnd->callback = cb;
  xdnd->win = window; /* xdnd_listener will overwrite this */

  return 1;
}

/*
 * Handle ClientMessage/SelectionNotify events.
 */
int process_client_dnd_message(DndClass *xdnd, XEvent *event) {
  int retval = 0;

  if((xdnd == NULL) || (event == NULL))
    return 0;

  if(event->type == ClientMessage) {

    if((xdnd->in_progress == True) && (event->xclient.format == 32) && 
       (XDND_ENTER_SOURCE_WIN(event) == xdnd->_XA_WM_DELETE_WINDOW)) {
      XEvent xevent;

#ifdef DEBUG_DND
      printf("ClientMessage KILL\n");
#endif

      memset(&xevent, 0, sizeof(xevent));
      xevent.xany.type                 = DestroyNotify;
      xevent.xany.display              = xdnd->display;
      xevent.xdestroywindow.type       = DestroyNotify;
      xevent.xdestroywindow.send_event = True;
      xevent.xdestroywindow.display    = xdnd->display;
      xevent.xdestroywindow.event      = xdnd->win;
      xevent.xdestroywindow.window     = xdnd->win;

      XLockDisplay(xdnd->display);
      XSendEvent(xdnd->display, xdnd->win, True, 0L, &xevent);
      XUnlockDisplay(xdnd->display);

      retval = 1;
    }
    else if(event->xclient.message_type == xdnd->_XA_XdndEnter) {

#ifdef DEBUG_DND
      printf("XdndEnter\n");
#endif

      if((XDND_ENTER_VERSION(event) < 3) || 
         (xdnd->in_progress == True)) {
	return 0;
      }
      else xdnd->in_progress = True;

      xdnd->dragger_window   = XDND_ENTER_SOURCE_WIN(event);
      xdnd->dropper_toplevel = event->xany.window;
      xdnd->dropper_window   = None;

      free(xdnd->dragger_typelist);
      xdnd->dragger_typelist = NULL;

      if(XDND_ENTER_THREE_TYPES(event)) {
#ifdef DEBUG_DND
	printf("Three types only\n");
#endif
	_dnd_get_three_types(event, &xdnd->dragger_typelist);
      } 
      else {
#ifdef DEBUG_DND
	printf("More than three types - getting list\n");
#endif
	_dnd_get_type_list(xdnd, xdnd->dragger_window, &xdnd->dragger_typelist);
      }

      if(xdnd->dragger_typelist) {
	int atom_match;
#ifdef DEBUG_DND
	{
	  int   i;
	  for(i = 0; xdnd->dragger_typelist[i] != 0; i++) {
	    XLockDisplay(xdnd->display);
	    printf("%d: '%s' ", i, XGetAtomName(xdnd->display, xdnd->dragger_typelist[i]));
	    XUnlockDisplay(xdnd->display);
	    printf("\n");
	  }
	}
#endif

	if((atom_match = _is_atom_match(xdnd, &xdnd->dragger_typelist)) >= 0) {
	  xdnd->desired = xdnd->dragger_typelist[atom_match];
	}

      }
      else {
	fprintf(stderr,
        "xdnd.c@%d: xdnd->dragger_typelist is zero length!\n", __LINE__);
	/* Probably doesn't work */
	if ((event->xclient.data.l[1] & 1) == 0) {
	  xdnd->desired = (Atom) event->xclient.data.l[1];
	}
      }
      retval = 1;
    }
    else if(event->xclient.message_type == xdnd->_XA_XdndLeave) {
#ifdef DEBUG_DND
      printf("XdndLeave\n");
#endif

      if((event->xany.window == xdnd->dropper_toplevel) && (xdnd->dropper_window != None))
	event->xany.window = xdnd->dropper_window;

      if(xdnd->dragger_window == XDND_LEAVE_SOURCE_WIN(event)) {
	free(xdnd->dragger_typelist);
	xdnd->dragger_typelist = NULL;
	xdnd->dropper_toplevel = xdnd->dropper_window = None;
	xdnd->desired = 0;
        xdnd->in_progress = False;
      }

      retval = 1;
    } 
    else if(event->xclient.message_type == xdnd->_XA_XdndDrop) {
      Window  win;

#ifdef DEBUG_DND
      printf("XdndDrop\n");
#endif
      if((xdnd->dragger_window == XDND_DROP_SOURCE_WIN(event))) {

      if(xdnd->desired != 0) {

	if((event->xany.window == xdnd->dropper_toplevel) && (xdnd->dropper_window != None))
	  event->xany.window = xdnd->dropper_window;
	
	  xdnd->time = XDND_DROP_TIME (event);

	  XLockDisplay(xdnd->display);
	  if(!(win = XGetSelectionOwner(xdnd->display,
                                        xdnd->_XA_XdndSelection)))
          {
	    fprintf(stderr,
                      "xdnd.c@%d: XGetSelectionOwner() failed.\n", __LINE__);
	    XUnlockDisplay(xdnd->display);
	    return 0;
	  }

	  XConvertSelection(xdnd->display, xdnd->_XA_XdndSelection,
          xdnd->desired, xdnd->TIMIDITY_PROTOCOL_ATOM,
          xdnd->dropper_toplevel, xdnd->time);
	  XUnlockDisplay (xdnd->display);
	}

         _dnd_send_finished(xdnd, xdnd->dragger_window,
         xdnd->dropper_toplevel);
      }

      retval = 1;
    }
    else if(event->xclient.message_type == xdnd->_XA_XdndPosition) {
      XEvent  xevent;
      Window  parent, child, toplevel, new_child;

#ifdef DEBUG_DND
      printf("XdndPosition\n");
#endif

      XLockDisplay(xdnd->display);

      toplevel = event->xany.window;
      parent   = DefaultRootWindow(xdnd->display);
      child    = xdnd->dropper_toplevel;

      for(;;) {
	int xd, yd;
	
	new_child = None;
	if(!XTranslateCoordinates (xdnd->display, parent, child, 
				   XDND_POSITION_ROOT_X(event), XDND_POSITION_ROOT_Y(event),
				   &xd, &yd, &new_child))
	  break;

	if(new_child == None)
	  break;

	child = new_child;
      }

      XUnlockDisplay(xdnd->display);

      xdnd->dropper_window = event->xany.window = child;

      xdnd->x    = XDND_POSITION_ROOT_X(event);
      xdnd->y    = XDND_POSITION_ROOT_Y(event);
      xdnd->time = XDND_POSITION_TIME(event);

      memset (&xevent, 0, sizeof(xevent));
      xevent.xany.type            = ClientMessage;
      xevent.xany.display         = xdnd->display;
      xevent.xclient.window       = xdnd->dragger_window;
      xevent.xclient.message_type = xdnd->_XA_XdndStatus;
      xevent.xclient.format       = 32;

      XDND_STATUS_TARGET_WIN(&xevent) = xdnd->dropper_toplevel;
      XDND_STATUS_WILL_ACCEPT_SET(&xevent, True);
      XDND_STATUS_WANT_POSITION_SET(&xevent, True);
      XDND_STATUS_RECT_SET(&xevent, xdnd->x, xdnd->y, 1, 1);
      XDND_STATUS_ACTION(&xevent) = XDND_POSITION_ACTION(event);

      XLockDisplay(xdnd->display);
      XSendEvent(xdnd->display, xdnd->dragger_window, 0, 0, &xevent);
      XUnlockDisplay(xdnd->display);
    }

    retval = 1;
  }
  else if(event->type == SelectionNotify) {

#ifdef DEBUG_DND
      printf("SelectionNotify\n");
#endif

    if(event->xselection.property == xdnd->TIMIDITY_PROTOCOL_ATOM) {
      _dnd_get_selection(xdnd, xdnd->dragger_window, 
			 event->xselection.property, event->xany.window);
      _dnd_send_finished(xdnd, xdnd->dragger_window, xdnd->dropper_toplevel);
    } 

    free(xdnd->dragger_typelist);
    xdnd->dragger_typelist = NULL;

    retval = 1;
  }

  return retval;
}
