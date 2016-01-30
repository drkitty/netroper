MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

.SECONDEXPANSION:


EXE_SRC := netroper.c
SRC := $(EXE_SRC) fail.c lib/common/wpa_ctrl.c lib/utils/os_unix.c

OBJ := $(SRC:%.c=%.o)
EXE := $(EXE_SRC:%.c=%)

CC := gcc
CFLAGS := -std=c99 -pedantic -g -Wall -Wextra -Werror -Wno-unused-function \
	-Wno-unused-parameter -I lib/ -I lib/common/ -I lib/utils/ \
	-D CONFIG_CTRL_IFACE -D CONFIG_CTRL_IFACE_UNIX


all: $(EXE) $(EXTRA_EXE)

$(OBJ): $$(patsubst %.o,%.c,$$@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXE) $(EXTRA_EXE):
	$(CC) -o $@ $^

$(EXE): $$@.o

clean:
	rm -f $(OBJ) $(EXE)


fail.o: common.h fail.h
lib/common/wpa_ctrl.o: lib/common/wpa_ctrl.h
lib/utils/os_unix.o: lib/utils/os.h
netroper.o: common.h fail.h lib/common/wpa_ctrl.h

netroper: fail.o lib/common/wpa_ctrl.o lib/utils/os_unix.o


.DEFAULT_GOAL := all
.PHONY: all clean
