PROGS   := slide-tour
CFLAGS  := -DG_DISABLE_DEPRECATED
LIBS    := -lSDL_gfx -lSDL_image -lphysfs
PKGS    := glib-2.0 libpng sdl
STD     := y
DEBUG   := n

slide-tour_OBJS := main.o xmlnode.o
include c.mk
