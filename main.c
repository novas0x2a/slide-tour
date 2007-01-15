#include <stdio.h>

#include <glib.h>
#include <SDL.h>
#include <SDL_image.h>
#include <string.h>

#include "xmlnode.h"
#include "main.h"

#define SWID 800
#define SHGT 533
#define WWID 912
#define WHGT 645
#define OFFX ((WWID-SWID)/2)
#define OFFY ((WHGT-SHGT)/2)
#define EDGE 120
#define ARRX 25
#define ARRY 25

static gboolean debug = FALSE;

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

SDL_Surface *curs[C_COUNT];
gchar *curfiles[C_COUNT] = {"arrow.png", "left.png", "left180.png", "right.png",
                            "right180.png", "up.png", "down.png", "fwd.png"};

SDL_Surface *arrows[C_COUNT];
gchar *arrowfiles[C_COUNT] = {NULL, "arrow-left.png", "arrow-left180.png", "arrow-right.png",
                              "arrow-right180.png", "arrow-up.png", "arrow-down.png", "arrow-fwd.png"};

SDL_Rect arrowrect[C_COUNT] = {{0,0,0,0}, 
                            {(OFFX-ARRX)/2, (WHGT-ARRY)/2, 0, 0}, {(OFFX-ARRX)/2, (WHGT-ARRY)/2, 0, 0},
                            {WWID-((OFFX+ARRX)/2), (WHGT-ARRY)/2, 0, 0}, {WWID-((OFFX+ARRX)/2), (WHGT-ARRY)/2, 0, 0},
                            {WWID/2, EDGE/2, 0, 0}, {WWID/2, SHGT+(EDGE/2), 0, 0},
                            {(WWID-ARRX)/2,(OFFY-ARRY)/2,0,0}};

Region get_region(int x, int y)
{
    x -= OFFX;
    y -= OFFY;

    if (x < 0 || x > SWID)
        return R_OFF;
    if (y < 0 || y > SHGT)
        return R_OFF;

    if (x < EDGE)
        return R_LEFT;
    if (x > SWID-EDGE)
        return R_RIGHT;

    if (y < EDGE)
        return R_TOP;
    if (y > SHGT-EDGE)
        return R_BOTTOM;

    return R_MIDDLE;
}

void slide_free(Slide *s)
{
    g_free(s->id);
    g_free(s->left);
    g_free(s->right);
    g_free(s->forward);
    g_free(s->left180);
    g_free(s->right180);
    g_free(s->up);
    g_free(s->down);
    g_free(s->fn);
    g_free(s);
}

inline gchar *cdata(xmlnode *p, gchar *cname)
{
    xmlnode *x = xmlnode_get_child(p, cname);
    if (x)
        return xmlnode_get_data(x);
    else
        return NULL;
}

void view(gchar *key, Slide *s, gpointer null)
{
    if (debug)
    {
        printf("Slide (id=%s, fn=%s): ", s->id, s->fn);
        #define printif(str) do {\
            if (s->str){printf("(%s %s) ", #str, s->str);}\
        } while (0)
        printif(forward);
        printif(left);
        printif(right);
        printif(left180);
        printif(right180);
        printif(up);
        printif(down);
        printf("\n");
    }
}

GHashTable* make_weak()
{
    gchar *data;
    gint len;
    GError *err = NULL;
    xmlnode *top,*s;
    GHashTable *slides;
    gchar *fn,*path;

    if (!g_file_get_contents("data.xml", &data, &len, &err))
    {
        warn("Couldn't open %s: %s\n", "data.xml", err->message);
        g_error_free(err);
        die("Failed.");
    }

    top = xmlnode_from_str(data, len);
    g_free(data);

    s = xmlnode_get_child(top, "slide");

    slides = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)slide_free);

    while (s)
    {
        gchar *id = cdata(s, "file");
        if (g_hash_table_lookup(slides, id))
        {
            warn("Slide %s was already seen. Ignoring the new one.\n", id);
            g_free(id);
        }
        else
        {
            Slide* tmp = g_new0(Slide, 1);
            xmlnode *next = xmlnode_get_child(s, "next");
            tmp->id = g_strdup(id);
            if (next)
            {
                #define try_insert(slide, node, child) do {\
                    gchar *d = cdata(node, #child);\
                    if (d) {\
                        slide->child = d;\
                        fn = g_strconcat(tmp->id, ".jpg", NULL);\
                        path = g_build_filename(SLIDEDIR, fn, NULL);\
                        if (!g_file_test(path, G_FILE_TEST_EXISTS))\
                            warn("%s does not exist at %s\n", fn, path);\
                        g_free(fn);\
                        slide->fn = path;\
                    }\
                } while (0)
                try_insert(tmp, next, left);
                try_insert(tmp, next, right);
                try_insert(tmp, next, forward);
                try_insert(tmp, next, left180);
                try_insert(tmp, next, right180);
                try_insert(tmp, next, up);
                try_insert(tmp, next, down);
            }
            g_hash_table_insert(slides, id, tmp);
        }

        s = xmlnode_get_next_twin(s);
    }
    xmlnode_free(top);
    Slide* tmp = g_new0(Slide, 1);
    tmp->id = g_strdup("map");
    fn = g_strconcat(tmp->id, ".jpg", NULL);
    path = g_build_filename(SLIDEDIR, fn, NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS))
        warn("%s does not exist at %s\n", fn, path);
    g_free(fn);
    tmp->fn = path;
    g_hash_table_insert(slides, g_strdup(tmp->id), tmp);

    return slides;
}

void draw_slide(SDL_Surface *surface, gchar *fn)
{
    static SDL_Surface *image = NULL;
    static gchar *last = NULL;

    if (last != fn)
    {
        if (image != NULL)
            SDL_FreeSurface(image);
        if (!(image = IMG_Load(fn)))
            die("IMG_Load: %s\n", IMG_GetError());
        last = fn;
    }

    SDL_Rect rcDest = { OFFX, OFFY, 0, 0 };
    SDL_BlitSurface(image, NULL, surface, &rcDest);
}


SDL_Surface* load(const gchar *path)
{
    SDL_Surface *image;
    if (!(image = IMG_Load(path)))
        die("IMG_Load: %s\n", IMG_GetError());
    return image;
}

void init_cursors()
{
    gchar *fn;
    int i;
    for (i = 0; i < C_COUNT; ++i)
    {
        fn = g_build_filename(CURSORDIR, curfiles[i], NULL);
        curs[i] = load(fn);
        g_free(fn);

        if (arrowfiles[i])
        {
            fn = g_build_filename(CURSORDIR, arrowfiles[i], NULL);
            arrows[i] = load(fn);
            g_free(fn);
        }
    }
}

void update_cursor(SDL_Surface *s, Slide *current)
{
    SDL_Surface *image;
    Region next;
    int x,y;

    SDL_GetMouseState(&x, &y);

    next = get_region(x,y);

    image = curs[C_ARROW];
    switch (next)
    {
        case R_LEFT:
            if (current->left180)
                image = curs[C_LEFTSPIN];
            else if (current->left)
                image = curs[C_LEFT];
            break;

        case R_RIGHT:
            if (current->right180)
                image = curs[C_RIGHTSPIN];
            else if (current->right)
                image = curs[C_RIGHT];
            break;

        case R_TOP:
            if (current->up)
                image = curs[C_UP];
            break;

        case R_BOTTOM:
            if (current->down)
                image = curs[C_DOWN];
            break;

        case R_MIDDLE:
            if (current->forward)
                image = curs[C_FWD];
            break;

        default:
            break;
    }

    SDL_Rect rcDest = {x, y, 0, 0};
    SDL_BlitSurface(image, NULL, s, &rcDest);
}

void draw_arrow(SDL_Surface *s, Cursor id)
{
    SDL_Rect *rcDest = &arrowrect[id];
    if (arrows[id])
        SDL_BlitSurface(arrows[id], NULL, s, rcDest);
    else
        SDL_BlitSurface(curs[id], NULL, s, rcDest);
}

void draw_exits(SDL_Surface *s, Slide *current)
{
    if (current->left180)
        draw_arrow(s, C_LEFTSPIN);
    else if (current->left)
        draw_arrow(s, C_LEFT);

    if (current->right180)
        draw_arrow(s, C_RIGHTSPIN);
    else if (current->right)
        draw_arrow(s, C_RIGHT);

    if (current->up)
        draw_arrow(s, C_UP);

    if (current->down)
        draw_arrow(s, C_DOWN);

    if (current->forward)
        draw_arrow(s, C_FWD);
}

Slide *try_move(Slide* cur, Region r, GHashTable *h)
{
    gchar *id = NULL;
    switch(r)
    {
        case R_LEFT:
            if (cur->left180)   id = cur->left180;
            else if (cur->left) id = cur->left;
            break;

        case R_RIGHT:
            if (cur->right180)   id = cur->right180;
            else if (cur->right) id = cur->right;
            break;

        case R_TOP:
            id = cur->up;
            break;

        case R_BOTTOM:
            id = cur->down;
            break;

        case R_MIDDLE:
            id = cur->forward;
            break;

        default:
            break;
    }
    if (id)
    {
        if (debug)
            warn("%s -> %s\n", cur->id, id);
        cur = g_hash_table_lookup(h, id);
        view(NULL, cur, NULL);
    }
    return cur;
}

int main(int argc, char **argv)
{
    SDL_Surface *screen;
    SDL_Event event;
    Slide *cur, *last = NULL;
    GHashTable *h = make_weak();
    gboolean update = 1, draw_mouse = 1;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        die("Unable to init SDL: %s", SDL_GetError());

    if (!(screen = SDL_SetVideoMode(WWID, WHGT, 0, SDL_SWSURFACE)))
        die("Could not get a surface: %s", SDL_GetError());

    SDL_ShowCursor(SDL_DISABLE);
    init_cursors();

    cur = g_hash_table_lookup(h, "S001");
    view(NULL, cur, NULL);
    while(1)
    {
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                Region r;
                case SDL_QUIT:
                    goto bail;
                case SDL_KEYDOWN:
                    r = R_INVALID;
                    switch (event.key.keysym.sym)
                    {
                        case 'd':
                            debug = debug ? FALSE : TRUE;
                            warn("Debug Mode %s\n", debug ? "enabled" : "disabled");
                            goto noadd;

                        case SDLK_TAB:
                            if (last)
                            {
                                if (debug)
                                    warn("%s -> %s\n", cur->id, last->id);
                                cur = last;
                                last = NULL;
                                view(NULL, cur, NULL);
                            }
                            else
                            {
                                last = cur;
                                if (debug)
                                    warn("%s -> %s\n", cur->id, "map");
                                cur = g_hash_table_lookup(h, "map");
                                view(NULL, cur, NULL);
                            }
                            goto noadd;

                        case 'q':
                        case SDLK_ESCAPE:
                            goto bail;
                        case SDLK_LEFT:
                            r = R_LEFT;
                            break;
                        case SDLK_RIGHT:
                            r = R_RIGHT;
                            break;
                        case SDLK_UP:
                            r = R_MIDDLE;
                            break;
                        /* This one breaks the plane paradigm (up is forward,
                         * so down should be backward) so it's getting commented.

                            case SDLK_DOWN:
                            r = R_BOTTOM;
                            break;*/
                        default:
                            break;
                    }
                    if (r != R_INVALID)
                        cur = try_move(cur,r,h);
                    noadd:
                    break;
                case SDL_ACTIVEEVENT:
                    draw_mouse = (event.active.state&SDL_APPMOUSEFOCUS) ? event.active.gain : 1;
                    update     = (event.active.state&SDL_APPACTIVE)     ? event.active.gain : 1;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.state  == SDL_PRESSED &&
                            event.button.button == SDL_BUTTON_LEFT)
                    {
                        Region r = get_region(event.button.x, event.button.y);
                        cur = try_move(cur, r, h);
                    }

                    break;
                default:
                    break;
            }
        }
        if (update)
        {
            SDL_FillRect(screen, NULL, 0);
            draw_slide(screen, cur->fn);
            draw_exits(screen, cur);
            if (draw_mouse)
                update_cursor(screen, cur);
            SDL_UpdateRect(screen, 0, 0, 0, 0);
        }
        else
            SDL_Delay(250);
    }

bail:
    SDL_Quit();
    g_hash_table_destroy(h);
    return 0;
}

/* $Revision: 1.11 $ */
