#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#define SLIDEDIR "slides"
#define CURSORDIR "cursors"

#define warn(...) fprintf(stderr, __VA_ARGS__)
#define die(...) {warn(__VA_ARGS__); exit(-1);}

typedef struct {
    gchar* id;
    gchar* left;
    gchar* right;
    gchar* forward;
    gchar* left180;
    gchar* right180;
    gchar* up;
    gchar* down;
    gchar* fn;
} Slide;

typedef enum {
    R_MIDDLE,
    R_LEFT,
    R_RIGHT,
    R_TOP,
    R_BOTTOM,
    R_OFF,
    R_INVALID
} Region;

typedef enum {
    C_ARROW,
    C_LEFT,
    C_LEFTSPIN,
    C_RIGHT,
    C_RIGHTSPIN,
    C_UP,
    C_DOWN,
    C_FWD,
    C_COUNT
} Cursor;


gboolean exists(const gchar *fn);
char *load_file(const char *path, PHYSFS_sint64 *size);
SDL_Surface *load_image(const char *path);

gchar *cdata(xmlnode *p, gchar *cname);
GHashTable* make_weak();
Region get_region(int x, int y);
Slide *try_move(Slide* cur, Region r, GHashTable *h);
void draw_arrow(SDL_Surface *s, Cursor id);
void draw_exits(SDL_Surface *s, Slide *current);
void draw_slide(SDL_Surface *surface, gchar *fn);
int  init_cursors();
void slide_free(Slide *s);
void try_insert_helper(Slide *slide, char** child, char *childname, xmlnode *node);
void update_cursor(SDL_Surface *s, Slide *current);
void view(gchar *key, Slide *s, gpointer null);

#endif
