# ========================================================================================
# Compile flags

CC := gcc
COPT := -O3 -march=native -fanalyzer -D_FORTIFY_SOURCE=2 -fstack-protector-strong

CFLAGS := -Wall -Wextra -Wpedantic -Werror -std=gnu11 -D_GNU_SOURCE -DNEON_OPTS -pthread
BUILD_VERSION := $(shell git describe --dirty --always 2>/dev/null)
ifeq (${BUILD_VERSION},)
	BUILD_VERSION := "untracked"
endif
BUILD_DATE := $(shell date '+%Y-%m-%d_%H:%M:%S')
CFLAGS += -D BUILD_VERSION="\"${BUILD_VERSION}\"" -D BUILD_DATE="\"${BUILD_DATE}\""

BIN := opflag

# ========================================================================================
# Source files

SRCDIR := src

SRCS := $(SRCDIR)/config.c \
		$(SRCDIR)/touch.c \
		$(SRCDIR)/buzzer.c \
		$(SRCDIR)/backlight.c \
		$(SRCDIR)/screen.c \
		$(SRCDIR)/graphics.c \
		$(SRCDIR)/font/font.c \
		$(SRCDIR)/font/dejavu_sans_16.c \
		$(SRCDIR)/font/dejavu_sans_36.c \
		$(SRCDIR)/font/dejavu_sans_48.c \
		$(SRCDIR)/font/dejavu_sans_72.c \
		$(SRCDIR)/clock.c \
		$(SRCDIR)/alarms.c \
		$(SRCDIR)/alarms-http.c \
		$(SRCDIR)/util/ini.c \
		$(SRCDIR)/util/timing.c

# ========================================================================================
# External Libraries

LDFLAGS := -lm -lcurl -llgpio -ljson-c

# ========================================================================================
# Makerules

OBJS := ${SRCS:.c=.o}

all: _print_banner opflag opflag-waitnetwork

opflag: ${OBJS} $(SRCDIR)/main.o
	@echo "  LD     "$@
	@${CC} ${COPT} ${CFLAGS} -o $@ ${OBJS} $(SRCDIR)/main.o ${LDFLAGS}

opflag-waitnetwork: ${OBJS} $(SRCDIR)/main-waitnetwork.o
	@echo "  LD     "$@
	@${CC} ${COPT} ${CFLAGS} -o $@ ${OBJS} $(SRCDIR)/main-waitnetwork.o ${LDFLAGS}

%.o: %.c
	@echo "  CC     "$<
	@${CC} ${COPT} ${CFLAGS} -c -I src/ -MD -fPIC -o $@ $<

debug: COPT = -Og -gdwarf -fno-omit-frame-pointer -D__DEBUG -fanalyzer -D_FORTIFY_SOURCE=2 -fstack-protector-strong
debug: all

_print_banner:
	@echo "Compiling with GCC $(shell $(CC) -dumpfullversion) on $(shell $(CC) -dumpmachine) (${DTMODEL})"
	@echo " - Build Version: [${BUILD_VERSION}] - ${BUILD_DATE}"

clean:
	@rm -rf $(BIN) $(OBJS) $(SRCDIR)/*.o
