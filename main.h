#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#define SLIDEDIR "slides" 
#define CURSORDIR "cursors"

#define warn(...) fprintf(stderr, __VA_ARGS__)
#define die(...) {warn(__VA_ARGS__); exit(-1);}

/* $Revision: 1.2 $ */
#endif
