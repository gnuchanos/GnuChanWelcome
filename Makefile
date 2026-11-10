CC ?= cc
PKG_CONFIG ?= pkg-config
PREFIX ?= /usr/local
DESTDIR ?=
SYSCONFDIR ?= /etc
BUILD_DIR ?= build
TARGET = $(BUILD_DIR)/GnuChanWelcome
COMMAND = $(DESTDIR)$(PREFIX)/bin/GnuChanWelcome
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags x11 xft)
CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(PKG_CONFIG) --libs x11 xft)

.PHONY: all build test install uninstall clean

all: build

build: $(TARGET)

$(TARGET): src/main.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

test: build
	sh tests/test_cli.sh
	sh tests/test_install.sh

install: build
	install -Dm755 $(TARGET) $(COMMAND)
	install -Dm644 gnuchan-welcome.desktop $(DESTDIR)$(PREFIX)/share/applications/gnuchan-welcome.desktop
	install -Dm644 autostart/gnuchan-welcome.desktop $(DESTDIR)$(SYSCONFDIR)/xdg/autostart/gnuchan-welcome.desktop

uninstall:
	rm -f $(COMMAND)
	rm -f $(DESTDIR)$(PREFIX)/share/applications/gnuchan-welcome.desktop
	rm -f $(DESTDIR)$(SYSCONFDIR)/xdg/autostart/gnuchan-welcome.desktop

clean:
	rm -rf $(BUILD_DIR)
