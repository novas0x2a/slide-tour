#include <stdio.h>

#include <glib.h>
#include <SDL.h>
#include <SDL_image.h>
#include <string.h>
#include <physfs.h>
#include <assert.h>

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

SDL_Surface *curs[C_COUNT];
static const gchar *curfiles[C_COUNT] = {"arrow.png", "left.png", "left180.png", "right.png",
                                         "right180.png", "up.png", "down.png", "fwd.png"};

SDL_Surface *arrows[C_COUNT];
static const gchar *arrowfiles[C_COUNT] = {NULL, "arrow-left.png", "arrow-left180.png", "arrow-right.png",
                                           "arrow-right180.png", "arrow-up.png", "arrow-down.png", "arrow-fwd.png"};

static SDL_Rect arrowrect[C_COUNT] = {{0,0,0,0},
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

gchar *cdata(xmlnode *p, gchar *cname)
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

gboolean exists(const gchar *fn)
{
    return PHYSFS_exists(fn) ? 1 : 0;
}

void try_insert_helper(Slide *slide, char** child, char *childname, xmlnode *node)
{
    gchar *fn;
    gchar *d = cdata(node, childname);
    if (d) {
        *child = d;
        fn   = g_strconcat(SLIDEDIR, G_DIR_SEPARATOR_S, slide->id, ".jpg", NULL);
        if (!exists(fn))
            warn("%s does not exist\n", fn);
        slide->fn = fn;
    }
}

#define try_insert(slide, node, child) try_insert_helper(slide, &(slide->child), #child, node)

GHashTable* make_weak()
{
    gchar *data;
    PHYSFS_sint64 len;
    xmlnode *top,*s;
    GHashTable *slides;
    Slide *tmp;

    if (!(data = load_file("data.xml", &len)))
        exit(1);

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
            xmlnode *next;
            tmp  = g_new0(Slide, 1);
            next = xmlnode_get_child(s, "next");
            tmp->id = g_strdup(id);
            if (next)
            {
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
    tmp = g_new0(Slide, 1);
    tmp->id = g_strdup("map");
    tmp->fn = g_strconcat(SLIDEDIR, G_DIR_SEPARATOR_S, tmp->id, ".jpg", NULL);
    if (!exists(tmp->fn))
        warn("%s does not exist\n", tmp->fn);
    g_hash_table_insert(slides, g_strdup(tmp->id), tmp);

    return slides;
}

char *load_file(const char *path, PHYSFS_sint64 *size)
{
    gchar *buf = NULL;
    PHYSFS_sint64 cnt = 0;
    PHYSFS_file *f = PHYSFS_openRead(path);

    if (!f) {
        warn("Error loading %s: %s\n", path, PHYSFS_getLastError());
        goto load_file_out;
    }

    buf = g_malloc(PHYSFS_fileLength(f));
    cnt = PHYSFS_read(f, buf, 1, PHYSFS_fileLength(f));

    if (PHYSFS_fileLength(f) != cnt) {
        warn("Incomplete read of %s: %s\n", path, PHYSFS_getLastError());
        g_free(buf);
        buf = NULL;
        cnt = 0;
    }

    PHYSFS_close(f);

load_file_out:
    if (size)
        *size = cnt;
    return buf;
}

SDL_Surface *load_image(const char *path)
{
    SDL_Surface *img;
    PHYSFS_sint64 size;

    gchar *buf = load_file(path, &size);
    if (!buf)
        return NULL;

    if (!(img = IMG_Load_RW(SDL_RWFromConstMem(buf,size), 1))) {
        warn("Error loading image [%s]: %s\n", path, IMG_GetError());
        img = NULL;
    }

    g_free(buf);

    return img;
}

void draw_slide(SDL_Surface *surface, gchar *fn)
{
    static SDL_Surface *image = NULL;
    static gchar *last = NULL;
    SDL_Rect rcDest = { OFFX, OFFY, 0, 0 };

    if (last != fn)
    {
        if (image != NULL)
            SDL_FreeSurface(image);
        image = load_image(fn);
        last = fn;
    }

    SDL_BlitSurface(image, NULL, surface, &rcDest);
}

int init_cursors()
{
    gchar *fn;
    int i;
    int error = 0;

    for (i = 0; i < C_COUNT; ++i)
    {
        fn = g_build_filename(CURSORDIR, curfiles[i], NULL);
        curs[i] = load_image(fn);
        g_free(fn);
        if (!curs[i])
            error++;

        if (arrowfiles[i])
        {
            fn = g_build_filename(CURSORDIR, arrowfiles[i], NULL);
            arrows[i] = load_image(fn);
            g_free(fn);
            if (!arrows[i])
                error++;
        }
    }
    return error == 0;
}

void update_cursor(SDL_Surface *s, Slide *current)
{
    SDL_Surface *image;
    SDL_Rect rcDest;
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
    rcDest = (struct SDL_Rect) {x, y, 0, 0};

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
    GHashTable *h = NULL;
    gboolean update = 1, draw_mouse = 1;
    char **files;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        die("Unable to init SDL: %s", SDL_GetError());

    if (!(screen = SDL_SetVideoMode(WWID, WHGT, 0, SDL_SWSURFACE)))
        die("Could not get a surface: %s", SDL_GetError());

    PHYSFS_init(argv[0]);

    PHYSFS_addToSearchPath(".", 0);
    PHYSFS_addToSearchPath("data", 0);

    files = PHYSFS_enumerateFiles("");
    for (char **i = files; *i != NULL; ++i)
        if (g_str_has_suffix(*i, ".zip"))
        {
            printf("Loading %s\n", *i);
            PHYSFS_addToSearchPath(*i, 1);
        }

    PHYSFS_freeList(files);

    SDL_ShowCursor(SDL_DISABLE);
    if (!init_cursors())
        goto bail;

    h = make_weak();
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
    PHYSFS_deinit();
    SDL_Quit();
    if (h)
        g_hash_table_destroy(h);
    return 0;
}
