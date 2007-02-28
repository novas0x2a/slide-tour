CC=gcc
WINCC:=/opt/xmingw/bin/$(CC).exe
OBJS=main.o xmlnode.o

PKGS=glib-2.0
PKG_CONFIG=pkg-config
CFLAGS=-Wall -W -Wno-unused-parameter\
		`$(PKG_CONFIG) --cflags $(PKGS)` -DG_DISABLE_DEPRECATED\
		`sdl-config --cflags`

WINFLAGS=-I/opt/xmingw/i386-mingw32msvc/include -Dmain=SDL_main
WINLIBS=-L/opt/xmingw/i386-mingw32msvc/bin -lmingw32 -lSDLmain -lSDL\
		-lglib-2.0-0 -lSDL_image -mwindows

LIBS=-Wl,-Bstatic -lSDL_image -lSDL -ltiff -ljpeg \
	`libpng-config --static --libs` `${PKG_CONFIG} --libs $(PKGS)` \
	-Wl,-Bdynamic -L/usr/lib -Wl,-rpath,/usr/lib -lpthread -lm -ldl -lX11 -lXext

PROG=wpi-tour

.PHONY: all clean tags split dist dist-linux dist-win32 dist-data \
		dist-linux-nodata dist-win32-nodata

all: ${PROG}

$(PROG).exe : ${OBJS:.o=-win.o}
	$(WINCC) ${CFLAGS} $(WINFLAGS) $^ -o $@ ${WINLIBS}

$(PROG) : ${OBJS}
	$(CC) ${CFLAGS} $^ -o $@ ${LIBS}

%-win.o : %.c
	$(WINCC) ${CFLAGS} $(WINFLAGS) -c $^ -o $@

%.o : %.c
	$(CC) ${CFLAGS} -c $^ -o $@

clean:
	rm -f $(PROG) $(PROG).exe core core.* *.o

distclean: clean
	rm -fr dist

tags: *.c *.h
	exuberant-ctags *.c *.h

split: dist-linux-nodata dist-win32-nodata dist-data

dist: dist-linux dist-win32

dist-linux: wpi-tour
	mkdir -p dist/wpi-tour
	cd dist &&\
		ln -s ../../cursors wpi-tour &&\
		ln -s ../../data.xml wpi-tour &&\
		ln -s ../../slides wpi-tour &&\
		ln -s ../../wpi-tour wpi-tour &&\
		zip -r wpi-tour-linux.zip wpi-tour -x \*CVS\* &&\
		rm -rf wpi-tour

dist-win32: wpi-tour.exe
	mkdir -p dist/wpi-tour
	cd dist &&\
		ln -s ../../cursors wpi-tour &&\
		ln -s ../../data.xml wpi-tour &&\
		ln -s ../../slides wpi-tour &&\
		ln -s ../../wpi-tour.exe wpi-tour &&\
		cp ../win-libs/*.dll wpi-tour &&\
		zip -r wpi-tour-win32.zip wpi-tour -x \*CVS\* &&\
		rm -rf wpi-tour

dist-data:
	zip -ur wpi-tour-data.zip slides; ret=$$?; \
	if [[ $$ret = 12 || $$ret = 0 ]]; then true else false; fi
	mv wpi-tour-data.zip dist

dist-linux-nodata: wpi-tour
	mkdir -p dist/wpi-tour
	cd dist &&\
		ln -s ../../cursors wpi-tour &&\
		ln -s ../../data.xml wpi-tour &&\
		ln -s ../../wpi-tour wpi-tour &&\
		zip -r wpi-tour-linux-nodata.zip wpi-tour -x \*CVS\* &&\
		rm -rf wpi-tour

dist-win32-nodata: wpi-tour.exe
	mkdir -p dist/wpi-tour
	cd dist &&\
		ln -s ../../cursors wpi-tour &&\
		ln -s ../../data.xml wpi-tour &&\
		ln -s ../../wpi-tour.exe wpi-tour &&\
		cp ../win-libs/*.dll wpi-tour &&\
		zip -r wpi-tour-win32-nodata.zip wpi-tour -x \*CVS\* &&\
		rm -rf wpi-tour

# $Revision: 1.9 $
