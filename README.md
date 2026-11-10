# GnuChanWelcome

GnuChanWelcome is a native C application drawn directly with X11 and Xft. It
has no Python, Tk, or GTK runtime dependency. Its tabs describe the supplied
GnuChan programs, their configuration paths, and the matching GnuChanSettings
categories. The purple palette matches GnuChanSettings' default theme.

The program is built and installed as `GnuChanWelcome`. It uses mouse-wheel
scrolling, Page Up/Page Down, Home/End, and keys 1-4
to navigate. The Open GnuChanSettings button launches the settings program.

## Build and test

Build dependencies on Debian/Ubuntu:

```sh
sudo apt install build-essential pkg-config libx11-dev libxft-dev
```

Build and run focused tests with:

```sh
make
make test
```

`make test` validates command-line/configuration behavior, staged
install/uninstall, and (when available) the X11 window under Xvfb. To run the
application:

```sh
build/GnuChanWelcome
```

Command-line options:

```text
GnuChanWelcome [--first-run] [--check-config] [--config PATH] [--version]
```

## Install

Install system-wide with:

```sh
sudo sh install.sh
```

The binary is `/usr/local/bin/GnuChanWelcome`; the desktop-menu entry and
first-login autostart entry are installed under `/usr/local/share/applications`
and `/etc/xdg/autostart`. `PREFIX`, `SYSCONFDIR`, and `DESTDIR` are supported
for packaging and staging. Remove the program and entries with
`sudo sh uninstall.sh`.

GnuChanWM starts programs from its own `gcl_autostart.all` list rather than
reading XDG autostart entries. The distro's default GnuChanWM configuration
therefore also starts `GnuChanWelcome --first-run`. Subsequent first-run
launches exit after the welcome guide has been dismissed.

## Configuration and first run

The per-user state file is
`~/.config/GnuChanWelcome/welcome.conf`, or
`$XDG_CONFIG_HOME/GnuChanWelcome/welcome.conf` when that variable is set:

```conf
FirstRunComplete = true
```

The application creates this state after the window is closed. `--config PATH`
selects another state file, and `--check-config` validates it without opening
an X11 window. GnuChanSettings provides a GnuChanWelcome category for this
file.

## Included programs

The guide describes the 24 supplied projects:
GnuChanBrowser, GnuChanDesktop, GnuChanDM, GnuChanDock, GnuChanFetch,
GnuChanFileManager, GnuChanGUI, GnuChanImageViewer, GnuChanLockScreen,
GnuChanNetworkManager, GnuChanNotification, GnuChanRunner, GnuChanScreenShoot,
GnuChanSettings, GnuChanSoftwareCenter, GnuChanSound, GnuChanSoundRecord,
GnuChanSS, GnuChanTerm, GnuChanTop, GnuChanWelcome, GnuChanWifi, GnuChanWM,
and GnuChanBluetooth.

GnuChanGUI is a reusable toolkit/library, not a launchable program. The
GnuChanBrowser card identifies its config as an unapplied source template. The
screen locker project is GnuChanLockScreen and its installed executable and
settings category are named GnuChanSL. GnuChanSound and GnuChanSettings share
`~/.config/GnuChanSettings/system.conf`.
