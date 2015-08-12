#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

int XMapWindow(Display* display, Window w) {
  printf("Window Mapping Blocked\n");
  return 0;
}

