/* ----------------------------------------------------------------------------- 
 * init.c
 *
 *     Initialize the wad system.  This sets up a signal handler for catching
 *     SIGSEGV, SIGBUS, and SIGABRT.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

#include "wad.h"

/* Debugging flag */
int    wad_debug_mode = 0;

/* Initialize wad */
void wad_init() {
  static int init = 0;

  wad_memory_init();
  if (getenv("WAD_DEBUG_SEGMENT")) {
    wad_debug_mode |= DEBUG_SEGMENT;
  }
  if (getenv("WAD_DEBUG_SYMBOL")) {
    wad_debug_mode |= DEBUG_SYMBOL;
  }

  if (getenv("WAD_DEBUG_OBJECT")) {
    wad_debug_mode |= DEBUG_OBJECT;
  }

  if (getenv("WAD_DEBUG_FILE")) {
    wad_debug_mode |= DEBUG_FILE;
  }

  if (getenv("WAD_DEBUG_HOLD")) {
    wad_debug_mode |= DEBUG_HOLD;
  }

  if (getenv("WAD_DEBUG_STABS")) {
    wad_debug_mode |= DEBUG_STABS;
  }

  if (getenv("WAD_DEBUG_RETURN")) {
    wad_debug_mode |= DEBUG_RETURN;
  }

  if (getenv("WAD_DEBUG_SYMBOL_SEARCH")) {
    wad_debug_mode |= DEBUG_SYMBOL_SEARCH;
  }

  if (getenv("WAD_DEBUG_INIT")) {
    wad_debug_mode |= DEBUG_INIT;
  }

  if (getenv("WAD_DEBUG_STACK")) {
    wad_debug_mode |= DEBUG_STACK;
  }

  if (getenv("WAD_DEBUG_UNWIND")) {
    wad_debug_mode |= DEBUG_UNWIND;
  }

  if (getenv("WAD_DEBUG_SIGNAL")) {
    wad_debug_mode |= DEBUG_SIGNAL;
  }

  if (getenv("WAD_NOSTACK")) {
    wad_debug_mode |= DEBUG_NOSTACK;
  }

  if (getenv("WAD_ONESHOT")) {
    wad_debug_mode |= DEBUG_ONESHOT;
  }

  if (wad_debug_mode & DEBUG_INIT) {
    printf("WAD: initializing\n");
  }

  if (!init) {
    wad_signal_init();
    wad_object_init();
  }
  init = 1;
}
