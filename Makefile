PROGS   := wpi-tour
CFLAGS  := -DG_DISABLE_DEPRECATED
LIBS    := -lSDL_gfx -lSDL_image -lphysfs
PKGS    := glib-2.0 libpng sdl
STD     := y
DEBUG   := n

wpi-tour_OBJS := main.o xmlnode.o
include c.mk
