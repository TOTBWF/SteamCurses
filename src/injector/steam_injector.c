#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

// Hack to prevent steam UI from actually showing
// This uses LD_PRELOAD to load a dummy version of X11's display
int XMapWindow(Display* display, Window w) {
  return 0;
}

