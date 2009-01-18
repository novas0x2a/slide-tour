ifeq ($(origin CC), default)
CC := gcc
endif

CC ?= gcc

CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wdeclaration-after-statement

ifeq ($(STD),y)
CFLAGS+=-std=c99 -pedantic
endif

ifdef PKGS
    CFLAGS += `pkg-config --cflags $(PKGS)`
    LIBS   += `pkg-config --libs $(PKGS)`
endif

ifeq ($(DEBUG),y)
    CFLAGS += -g2 -ggdb
else
    CFLAGS += -O2
endif

all: $(PROGS)

%.o : %.c
	@#echo -e "Comp $^\n($(CFLAGS))"
	$(CC) $(CFLAGS) -c $^ -o $@

%.o : %.cc
	@#echo -e "Comp $^\n($(CFLAGS))"
	$(CC) $(CFLAGS) -c $^ -o $@

%-win.o : %.c
	$(CC).exe ${CFLAGS} $(WINFLAGS) -c $^ -o $@

clean:
	rm -f $(PROGS) core core.* *.o

tags:
	exuberant-ctags --recurse=yes .

.SECONDEXPANSION:
$(PROGS) : $$($$@_OBJS)
	@#echo "Link $@ ($^)"
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

.PHONY: all clean tags
.SUFFIXES:
.SECONDARY:
%: %,v
%: RCS/%
%: RCS/%,v
%: SCCS/s.%
%: s.%

