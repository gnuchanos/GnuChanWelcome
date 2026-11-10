#define _POSIX_C_SOURCE 200809L

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define APP_NAME "GnuChanWelcome"
#define VERSION "0.2.0"
#define PROGRAM_COUNT 24
#define TAB_COUNT 4
#define WINDOW_WIDTH 920
#define WINDOW_HEIGHT 680
#define HEADER_HEIGHT 104
#define FOOTER_HEIGHT 58
#define CARD_GAP 12
#define TEXT_MARGIN 22

typedef struct Program {
    const char *name;
    const char *description;
    const char *config;
    const char *settings;
    int group;
} Program;

typedef struct Palette {
    const char *background;
    const char *sidebar;
    const char *selection;
    const char *panel;
    const char *edge;
    const char *text;
    const char *muted;
    const char *accent;
} Palette;

typedef struct App {
    Display *display;
    int screen;
    Window window;
    GC gc;
    XftDraw *draw;
    XftFont *font;
    XftFont *bold;
    XftColor background;
    XftColor sidebar;
    XftColor selection;
    XftColor panel;
    XftColor edge;
    XftColor text;
    XftColor muted;
    XftColor accent;
    XftColor white;
    XftColor black;
    XftColor active_text;
    Atom wm_delete;
    int width;
    int height;
    int tab;
    int scroll;
    int content_height;
    int running;
    char config_path[PATH_MAX];
} App;

static const Palette purple = {
    "#1a0b2e", "#140826", "#32143f", "#241033",
    "#7b2cbf", "#e0c3fc", "#9d7bba", "#c77dff"
};

static const char *const tab_titles[TAB_COUNT] = {
    "Welcome", "Your desktop", "Programs and tools", "Settings and system"
};

static const char *const tab_descriptions[TAB_COUNT] = {
    "Welcome to GnuChan",
    "The parts of the desktop you see and use every day.",
    "Useful applications for commands, files, images, audio, and system activity.",
    "Configure audio, networking, Bluetooth, and the GnuChan system."
};

static const Program programs[PROGRAM_COUNT] = {
    {"GnuChanDM", "The graphical login screen.", "~/.config/GnuChanDM/GnuChanDM.conf", "GnuChanDM", 1},
    {"GnuChanWM", "The window manager that arranges and manages your windows.", "~/.config/GnuChanWM/GnuChanWM.conf", "GnuChanWM", 1},
    {"GnuChanDock", "The desktop dock for launching and switching between programs.", "~/.config/GnuChanDock/GnuChanDock.conf", "GnuChanDock", 1},
    {"GnuChanDesktop", "Set your wallpaper, background colour, and display layout.", "~/.config/gnuchan-desktop/panel.conf", "GnuChanDesktop", 1},
    {"GnuChanFileManager", "Browse files and folders, including hidden files.", "~/.config/GnuChanFileManager/config.conf", "GnuChanFileManager", 1},
    {"GnuChanImageViewer", "View, organize, and make basic edits to images.", "No user configuration file", "No dedicated Settings category", 2},
    {"GnuChanLockScreen", "Lock the screen and require your password to unlock it (installed as GnuChanSL).", "~/.config/GnuChanSL/GnuChanSL.conf", "GnuChanSL", 1},
    {"GnuChanBrowser", "A lightweight web browser for visiting websites.", "GnuChanBrowser.conf template (not loaded by the browser)", "Not currently available in GnuChanSettings", 2},
    {"GnuChanGUI", "A reusable GUI toolkit/library; it is not a launchable desktop app.", "No standalone config (library)", "No standalone Settings category (library)", 2},
    {"GnuChanTerm", "A terminal for running shell commands.", "~/.config/GnuChanTerm/GnuChanTerm.conf", "GnuChanTerm", 2},
    {"GnuChanFetch", "Show system information in the terminal.", "~/.config/GnuChanFetch/GnuChanFetch.conf", "GnuChanFetch", 2},
    {"GnuChanRunner", "Find and launch installed programs from the keyboard.", "~/.config/GnuChanRunner/config.conf", "GnuChanRunner", 2},
    {"GnuChanSS", "The screen saver.", "~/.config/GnuChanSS/GnuChanSS.conf", "GnuChanSS", 2},
    {"GnuChanNotification", "Display desktop notification bubbles.", "~/.config/GnuChanNotification/GnuChanNotification.conf", "GnuChanNotification", 2},
    {"GnuChanScreenShoot", "Capture screenshots and choose where they are saved.", "~/.config/GnuChanScreenShoot/GnuChanScreenShoot.conf", "GnuChanScreenShoot", 2},
    {"GnuChanSoftwareCenter", "Browse packages and manage GnuChan updates.", "~/.config/GnuChanSoftwareCenter/GnuChanSoftwareCenter.conf", "GnuChanSoftwareCenter", 2},
    {"GnuChanSoundRecord", "Record audio from an available microphone.", "~/.config/GnuChanSoundRecord/config.conf", "No dedicated Settings category", 2},
    {"GnuChanTop", "Monitor system processes and resource use.", "~/.config/GnuChanTop/GnuChanTop.conf", "GnuChanTop", 2},
    {"GnuChanSettings", "The control panel for configuring GnuChan programs and system options.", "~/.config/GnuChanSettings/system.conf", "System", 3},
    {"GnuChanSound", "Control PipeWire audio; its language preference is shared with GnuChan apps.", "~/.config/GnuChanSettings/system.conf", "GnuChanSound", 3},
    {"GnuChanWifi", "Connect to and manage Wi-Fi networks.", "~/.config/GnuChanWifi/config.conf", "GnuChanWifi", 3},
    {"GnuChanNetworkManager", "Configure network interfaces, DNS, and the resolver.", "~/.config/GnuChanNetworkManager/config.conf", "GnuChanNetworkManager", 3},
    {"GnuChanBluetooth", "Discover and manage Bluetooth adapters and paired devices.", "~/.config/GnuChanBluetooth/config.json (optional)", "No dedicated Settings category", 3},
    {"GnuChanWelcome", "This first-login guide introduces GnuChan programs and settings.", "~/.config/GnuChanWelcome/welcome.conf", "GnuChanWelcome", 3}
};

static int color_alloc(App *app, XftColor *color, const char *hex) {
    return XftColorAllocName(app->display, DefaultVisual(app->display, app->screen),
                             DefaultColormap(app->display, app->screen), hex, color);
}

static void set_gc_color(App *app, unsigned long pixel) {
    XSetForeground(app->display, app->gc, pixel);
}

static unsigned long pixel_from_color(App *app, XftColor *color) {
    (void)app;
    return color->pixel;
}

static void fill_rect(App *app, int x, int y, unsigned int width,
                      unsigned int height, XftColor *color) {
    set_gc_color(app, pixel_from_color(app, color));
    XFillRectangle(app->display, app->window, app->gc, x, y, width, height);
}

static void draw_rect(App *app, int x, int y, unsigned int width,
                      unsigned int height, XftColor *color) {
    set_gc_color(app, pixel_from_color(app, color));
    XDrawRectangle(app->display, app->window, app->gc, x, y, width, height);
}

static void text_at(App *app, XftFont *font, XftColor *color, int x, int baseline,
                    const char *text) {
    XftDrawStringUtf8(app->draw, color, font, x, baseline,
                      (const FcChar8 *)text, (int)strlen(text));
}

static int text_width(App *app, XftFont *font, const char *text) {
    XGlyphInfo extents;
    XftTextExtentsUtf8(app->display, font, (const FcChar8 *)text,
                       (int)strlen(text), &extents);
    return (int)extents.xOff;
}

static void draw_wrapped(App *app, const char *text, int x, int baseline,
                         int max_width, XftFont *font, XftColor *color,
                         int line_height) {
    char line[1024];
    size_t used = 0;
    const char *word = text;
    int y = baseline;

    while (*word) {
        while (*word == ' ') word++;
        if (!*word) break;
        const char *end = word;
        while (*end && *end != ' ') end++;
        size_t length = (size_t)(end - word);
        if (length >= sizeof(line)) length = sizeof(line) - 1;
        char candidate[1024];
        size_t prefix = used ? used + 1 : 0;
        if (prefix + length >= sizeof(candidate)) length = sizeof(candidate) - prefix - 1;
        if (used) {
            memcpy(candidate, line, used);
            candidate[used] = ' ';
        }
        memcpy(candidate + prefix, word, length);
        candidate[prefix + length] = '\0';
        if (used && text_width(app, font, candidate) > max_width) {
            line[used] = '\0';
            text_at(app, font, color, x, y, line);
            y += line_height;
            used = 0;
            continue;
        }
        memcpy(line + prefix, word, length);
        used = prefix + length;
        word = end;
    }
    if (used) {
        line[used] = '\0';
        text_at(app, font, color, x, y, line);
    }
}

static int wrapped_line_count(App *app, const char *text, int max_width,
                              XftFont *font) {
    char line[1024];
    size_t used = 0;
    const char *word = text;
    int lines = 1;
    while (*word) {
        while (*word == ' ') word++;
        if (!*word) break;
        const char *end = word;
        while (*end && *end != ' ') end++;
        size_t length = (size_t)(end - word);
        if (length >= sizeof(line)) length = sizeof(line) - 1;
        char candidate[1024];
        size_t prefix = used ? used + 1 : 0;
        if (prefix + length >= sizeof(candidate)) length = sizeof(candidate) - prefix - 1;
        if (used) {
            memcpy(candidate, line, used);
            candidate[used] = ' ';
        }
        memcpy(candidate + prefix, word, length);
        candidate[prefix + length] = '\0';
        if (used && text_width(app, font, candidate) > max_width) {
            lines++;
            used = 0;
            continue;
        }
        memcpy(line + prefix, word, length);
        used = prefix + length;
        word = end;
    }
    return lines;
}

static int card_height(App *app, const Program *program, int width) {
    int line_height = app->font->ascent + app->font->descent + 3;
    int text_width_available = width - 2 * TEXT_MARGIN;
    int description_lines = wrapped_line_count(app, program->description,
                                                text_width_available, app->font);
    int config_lines = wrapped_line_count(app, program->config,
                                           text_width_available, app->font);
    int settings_lines = wrapped_line_count(app, program->settings,
                                             text_width_available, app->font);
    return 2 * TEXT_MARGIN + app->bold->ascent + 8 +
           (description_lines + config_lines + settings_lines + 2) * line_height + 12;
}

static int content_y(void) {
    return HEADER_HEIGHT + 10;
}

static int viewport_height(App *app) {
    int height = app->height - content_y() - FOOTER_HEIGHT;
    return height > 80 ? height : 80;
}

static void measure_content(App *app) {
    int y = 16;
    int width = app->width - 48;
    if (app->tab == 0) {
        app->content_height = viewport_height(app);
        return;
    }
    for (int i = 0; i < PROGRAM_COUNT; i++) {
        if (programs[i].group != app->tab) continue;
        y += card_height(app, &programs[i], width) + CARD_GAP;
    }
    app->content_height = y + 24;
}

static void draw_header(App *app) {
    fill_rect(app, 0, 0, (unsigned int)app->width, HEADER_HEIGHT, &app->background);
    text_at(app, app->bold, &app->text, 24, 34, "Welcome to GnuChan");
    text_at(app, app->font, &app->muted, 24, 58, tab_descriptions[app->tab]);

    int x = 20;
    int y = 72;
    int available = app->width - 40;
    int widths[TAB_COUNT];
    int total = 0;
    for (int i = 0; i < TAB_COUNT; i++) {
        widths[i] = text_width(app, app->bold, tab_titles[i]) + 28;
        total += widths[i];
    }
    int gap = 8;
    if (total + gap * (TAB_COUNT - 1) > available) {
        int excess = total + gap * (TAB_COUNT - 1) - available;
        for (int i = 0; i < TAB_COUNT && excess > 0; i++) {
            int cut = widths[i] - 84;
            if (cut > 0) {
                if (cut > excess) cut = excess;
                widths[i] -= cut;
                excess -= cut;
            }
        }
    }

    for (int i = 0; i < TAB_COUNT; i++) {
        if (i == app->tab) {
            fill_rect(app, x, y, (unsigned int)widths[i], 28, &app->selection);
            draw_rect(app, x, y, (unsigned int)widths[i], 28, &app->edge);
        }
        text_at(app, app->bold, i == app->tab ? &app->accent : &app->text,
                x + 12, y + 19, tab_titles[i]);
        x += widths[i] + gap;
    }
}

static void draw_welcome_page(App *app, int x, int y, int width, int height) {
    (void)height;
    fill_rect(app, x, y, (unsigned int)width, 226, &app->panel);
    draw_rect(app, x, y, (unsigned int)width, 226, &app->edge);
    text_at(app, app->bold, &app->accent, x + TEXT_MARGIN, y + 36,
            "Your GnuChan desktop is ready");
    draw_wrapped(app,
        "Explore the programs and tools included with GnuChan. Select a tab to see what each one does, where its configuration lives, and which GnuChanSettings category controls it.",
        x + TEXT_MARGIN, y + 68, width - 2 * TEXT_MARGIN, app->font,
        &app->text, app->font->ascent + app->font->descent + 5);
    text_at(app, app->font, &app->muted, x + TEXT_MARGIN, y + 154,
            "Mouse wheel scrolls these pages. Page Up and Page Down also work.");
    text_at(app, app->font, &app->muted, x + TEXT_MARGIN, y + 181,
            "Choose Open Settings below to change your desktop theme and system options.");
    char count_text[96];
    snprintf(count_text, sizeof(count_text), "%d GnuChan programs and components",
             PROGRAM_COUNT);
    text_at(app, app->bold, &app->text, x + TEXT_MARGIN, y + 210, count_text);
}

static void draw_program_card(App *app, const Program *program, int x, int y,
                              int width) {
    int height = card_height(app, program, width);
    int margin = TEXT_MARGIN;
    int text_x = x + margin;
    int available = width - 2 * margin;
    int line_height = app->font->ascent + app->font->descent + 3;
    fill_rect(app, x, y, (unsigned int)width, (unsigned int)height, &app->panel);
    draw_rect(app, x, y, (unsigned int)width, (unsigned int)height, &app->edge);
    int baseline = y + margin + app->bold->ascent;
    text_at(app, app->bold, &app->accent, text_x, baseline, program->name);
    baseline += app->bold->descent + 7;
    draw_wrapped(app, program->description, text_x, baseline, available,
                 app->font, &app->text, line_height);
    baseline += wrapped_line_count(app, program->description, available,
                                   app->font) * line_height + 4;
    draw_wrapped(app, "Configuration:", text_x, baseline, available,
                 app->bold, &app->muted, line_height);
    baseline += line_height;
    draw_wrapped(app, program->config, text_x, baseline, available,
                 app->font, &app->muted, line_height);
    baseline += wrapped_line_count(app, program->config, available,
                                   app->font) * line_height + 3;
    draw_wrapped(app, "GnuChanSettings:", text_x, baseline, available,
                 app->bold, &app->muted, line_height);
    baseline += line_height;
    draw_wrapped(app, program->settings, text_x, baseline, available,
                 app->font, &app->muted, line_height);
}

static void draw_footer(App *app) {
    int y = app->height - FOOTER_HEIGHT;
    fill_rect(app, 0, y, (unsigned int)app->width, FOOTER_HEIGHT, &app->sidebar);
    set_gc_color(app, pixel_from_color(app, &app->edge));
    XDrawLine(app->display, app->window, app->gc, 0, y, app->width, y);

    int button_y = y + 11;
    int button_h = 36;
    const char *settings_label = "Open GnuChanSettings";
    int settings_w = text_width(app, app->bold, settings_label) + 28;
    int done_w = text_width(app, app->bold, "Done") + 28;
    int done_x = app->width - 22 - done_w;
    int settings_x = done_x - 10 - settings_w;
    fill_rect(app, settings_x, button_y, (unsigned int)settings_w,
              (unsigned int)button_h, &app->panel);
    draw_rect(app, settings_x, button_y, (unsigned int)settings_w,
              (unsigned int)button_h, &app->edge);
    text_at(app, app->bold, &app->text, settings_x + 14, button_y + 23,
            settings_label);
    fill_rect(app, done_x, button_y, (unsigned int)done_w,
              (unsigned int)button_h, &app->selection);
    draw_rect(app, done_x, button_y, (unsigned int)done_w,
              (unsigned int)button_h, &app->edge);
    text_at(app, app->bold, &app->accent, done_x + 14, button_y + 23, "Done");
    text_at(app, app->font, &app->muted, 22, button_y + 22,
            "Scroll with the mouse wheel or Page Up / Page Down");
}

static void draw_page(App *app) {
    fill_rect(app, 0, 0, (unsigned int)app->width, (unsigned int)app->height,
              &app->background);
    draw_header(app);

    int x = 20;
    int y = content_y();
    int width = app->width - 40;
    int view_height = viewport_height(app);
    fill_rect(app, x, y, (unsigned int)width, (unsigned int)view_height,
              &app->background);

    XRectangle clip = { (short)x, (short)y, (unsigned short)width,
                        (unsigned short)view_height };
    XftDrawSetClipRectangles(app->draw, 0, 0, &clip, 1);
    if (app->tab == 0) {
        draw_welcome_page(app, x, y, width, view_height);
    } else {
        int item_y = y + 16 - app->scroll;
        for (int i = 0; i < PROGRAM_COUNT; i++) {
            if (programs[i].group != app->tab) continue;
            int height = card_height(app, &programs[i], width);
            if (item_y + height >= y && item_y <= y + view_height) {
                draw_program_card(app, &programs[i], x, item_y, width);
            }
            item_y += height + CARD_GAP;
        }
    }
    XftDrawSetClip(app->draw, NULL);
    draw_footer(app);
    XFlush(app->display);
}

static void change_tab(App *app, int tab) {
    if (tab < 0 || tab >= TAB_COUNT) return;
    app->tab = tab;
    app->scroll = 0;
    measure_content(app);
    draw_page(app);
}

static void scroll_by(App *app, int amount) {
    int maximum = app->content_height - viewport_height(app);
    if (maximum < 0) maximum = 0;
    app->scroll += amount;
    if (app->scroll < 0) app->scroll = 0;
    if (app->scroll > maximum) app->scroll = maximum;
    draw_page(app);
}

static int config_path(App *app, const char *override) {
    if (override) {
        if (snprintf(app->config_path, sizeof(app->config_path), "%s", override)
            >= (int)sizeof(app->config_path)) return -1;
        return 0;
    }
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    if (xdg && xdg[0]) {
        return snprintf(app->config_path, sizeof(app->config_path),
                        "%s/GnuChanWelcome/welcome.conf", xdg)
                   < (int)sizeof(app->config_path) ? 0 : -1;
    }
    if (!home || !home[0]) return -1;
    return snprintf(app->config_path, sizeof(app->config_path),
                    "%s/.config/GnuChanWelcome/welcome.conf", home)
               < (int)sizeof(app->config_path) ? 0 : -1;
}

static int config_read(const char *path, int *complete, int report) {
    FILE *file = fopen(path, "r");
    *complete = 0;
    if (!file) {
        if (errno == ENOENT) return 0;
        if (report) fprintf(stderr, "GnuChanWelcome: cannot read %s: %s\n",
                            path, strerror(errno));
        return -1;
    }
    char *line = NULL;
    size_t capacity = 0;
    unsigned long line_number = 0;
    int result = 0;
    while (getline(&line, &capacity, file) >= 0) {
        line_number++;
        char *start = line;
        while (*start == ' ' || *start == '\t') start++;
        if (*start == '\0' || *start == '\n' || *start == '#') continue;
        char *equals = strchr(start, '=');
        if (!equals) {
            if (report) fprintf(stderr, "GnuChanWelcome: %s:%lu: expected Key = value\n",
                                path, line_number);
            result = -1;
            break;
        }
        char *key_end = equals;
        while (key_end > start && (key_end[-1] == ' ' || key_end[-1] == '\t'))
            key_end--;
        char *value = equals + 1;
        while (*value == ' ' || *value == '\t') value++;
        char *end = value + strlen(value);
        while (end > value && (end[-1] == '\n' || end[-1] == '\r' ||
                               end[-1] == ' ' || end[-1] == '\t')) end--;
        *end = '\0';
        if (key_end == start) {
            if (report) fprintf(stderr, "GnuChanWelcome: %s:%lu: empty setting name\n",
                                path, line_number);
            result = -1;
            break;
        }
        if ((size_t)(end - value) >= 2 &&
            ((*value == '"' && end[-1] == '"') ||
             (*value == '\'' && end[-1] == '\''))) {
            value++;
            end[-1] = '\0';
        }
        if ((size_t)(key_end - start) == strlen("FirstRunComplete") &&
            strncmp(start, "FirstRunComplete", strlen("FirstRunComplete")) == 0) {
            *complete = strcmp(value, "true") == 0 ||
                        strcmp(value, "TRUE") == 0 ||
                        strcmp(value, "yes") == 0 ||
                        strcmp(value, "1") == 0;
        }
    }
    free(line);
    if (ferror(file)) {
        if (report) fprintf(stderr, "GnuChanWelcome: cannot read %s: %s\n",
                            path, strerror(errno));
        result = -1;
    }
    fclose(file);
    return result;
}

static int make_directories(const char *path) {
    char copy[PATH_MAX];
    if (snprintf(copy, sizeof(copy), "%s", path) >= (int)sizeof(copy)) return -1;
    for (char *cursor = copy + 1; *cursor; cursor++) {
        if (*cursor != '/') continue;
        *cursor = '\0';
        if (mkdir(copy, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "GnuChanWelcome: cannot create %s: %s\n",
                    copy, strerror(errno));
            *cursor = '/';
            return -1;
        }
        *cursor = '/';
    }
    return 0;
}

static int save_complete(const char *path) {
    char directory[PATH_MAX];
    if (snprintf(directory, sizeof(directory), "%s", path) >= (int)sizeof(directory))
        return -1;
    char *slash = strrchr(directory, '/');
    if (!slash) {
        memcpy(directory, ".", sizeof("."));
    } else if (slash == directory) {
        slash[1] = '\0';
    } else {
        *slash = '\0';
    }
    if (make_directories(directory) != 0) return -1;
    char temporary[PATH_MAX];
    if (snprintf(temporary, sizeof(temporary), "%s.new.%ld",
                 path, (long)getpid()) >= (int)sizeof(temporary)) return -1;
    FILE *file = fopen(temporary, "w");
    if (!file) {
        fprintf(stderr, "GnuChanWelcome: cannot write %s: %s\n",
                temporary, strerror(errno));
        return -1;
    }
    int okay = fprintf(file, "# GnuChan Welcome first-run state\n"
                             "FirstRunComplete = true\n") >= 0;
    if (fclose(file) != 0) okay = 0;
    if (!okay || rename(temporary, path) != 0) {
        int saved_errno = errno;
        unlink(temporary);
        fprintf(stderr, "GnuChanWelcome: cannot save %s: %s\n",
                path, strerror(saved_errno));
        return -1;
    }
    return 0;
}

static void open_settings(App *app) {
    pid_t child = fork();
    if (child == 0) {
        pid_t grandchild = fork();
        if (grandchild < 0) _exit(127);
        if (grandchild > 0) _exit(0);
        close(ConnectionNumber(app->display));
        execlp("GnuChanSettings", "GnuChanSettings", (char *)NULL);
        fprintf(stderr, "GnuChanWelcome: could not start GnuChanSettings: %s\n",
                strerror(errno));
        _exit(127);
    }
    if (child < 0) {
        fprintf(stderr, "GnuChanWelcome: could not start GnuChanSettings: %s\n",
                strerror(errno));
    } else {
        while (waitpid(child, NULL, 0) < 0 && errno == EINTR) {}
    }
}

static int init_xft(App *app) {
    if (!color_alloc(app, &app->background, purple.background) ||
        !color_alloc(app, &app->sidebar, purple.sidebar) ||
        !color_alloc(app, &app->selection, purple.selection) ||
        !color_alloc(app, &app->panel, purple.panel) ||
        !color_alloc(app, &app->edge, purple.edge) ||
        !color_alloc(app, &app->text, purple.text) ||
        !color_alloc(app, &app->muted, purple.muted) ||
        !color_alloc(app, &app->accent, purple.accent) ||
        !color_alloc(app, &app->white, "#ffffff") ||
        !color_alloc(app, &app->black, "#000000")) return -1;
    app->font = XftFontOpenName(app->display, app->screen,
                                "sans:pixelsize=14");
    app->bold = XftFontOpenName(app->display, app->screen,
                                "sans:bold:pixelsize=16");
    if (!app->font) app->font = XftFontOpenName(app->display, app->screen, "fixed");
    if (!app->bold) app->bold = XftFontOpenName(app->display, app->screen, "fixed");
    return app->font && app->bold ? 0 : -1;
}

static void free_xft(App *app) {
    if (app->draw) XftDrawDestroy(app->draw);
    if (app->font) XftFontClose(app->display, app->font);
    if (app->bold) XftFontClose(app->display, app->bold);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->background);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->sidebar);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->selection);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->panel);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->edge);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->text);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->muted);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->accent);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->white);
    XftColorFree(app->display, DefaultVisual(app->display, app->screen),
                 DefaultColormap(app->display, app->screen), &app->black);
}

static void on_click(App *app, int x, int y) {
    if (y >= 72 && y < HEADER_HEIGHT) {
        int current_x = 20;
        for (int i = 0; i < TAB_COUNT; i++) {
            int width = text_width(app, app->bold, tab_titles[i]) + 28;
            if (x >= current_x && x < current_x + width) {
                change_tab(app, i);
                return;
            }
            current_x += width + 8;
        }
    }
    int footer_y = app->height - FOOTER_HEIGHT;
    if (y < footer_y) return;
    const char *settings_label = "Open GnuChanSettings";
    int settings_w = text_width(app, app->bold, settings_label) + 28;
    int done_w = text_width(app, app->bold, "Done") + 28;
    int done_x = app->width - 22 - done_w;
    int settings_x = done_x - 10 - settings_w;
    if (x >= settings_x && x < settings_x + settings_w) {
        open_settings(app);
    } else if (x >= done_x && x < done_x + done_w) {
        app->running = 0;
    }
}

static int run_window(App *app) {
    app->display = XOpenDisplay(NULL);
    if (!app->display) {
        fprintf(stderr, "GnuChanWelcome: cannot open the X display\n");
        return 1;
    }
    app->screen = DefaultScreen(app->display);
    app->width = WINDOW_WIDTH;
    app->height = WINDOW_HEIGHT;
    app->window = XCreateSimpleWindow(
        app->display, RootWindow(app->display, app->screen), 0, 0,
        (unsigned int)app->width, (unsigned int)app->height, 0,
        BlackPixel(app->display, app->screen), BlackPixel(app->display, app->screen));
    XStoreName(app->display, app->window, "Welcome to GnuChan");
    XSizeHints hints;
    memset(&hints, 0, sizeof(hints));
    hints.flags = PMinSize;
    hints.min_width = 700;
    hints.min_height = 500;
    XSetWMNormalHints(app->display, app->window, &hints);
    app->wm_delete = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->window, &app->wm_delete, 1);
    XSelectInput(app->display, app->window,
                 ExposureMask | StructureNotifyMask | ButtonPressMask |
                 KeyPressMask);
    app->gc = XCreateGC(app->display, app->window, 0, NULL);
    if (init_xft(app) != 0) {
        fprintf(stderr, "GnuChanWelcome: cannot initialize Xft fonts or colors\n");
        XFreeGC(app->display, app->gc);
        XDestroyWindow(app->display, app->window);
        XCloseDisplay(app->display);
        return 1;
    }
    app->draw = XftDrawCreate(app->display, app->window,
                              DefaultVisual(app->display, app->screen),
                              DefaultColormap(app->display, app->screen));
    if (!app->draw) {
        fprintf(stderr, "GnuChanWelcome: cannot create the Xft drawing context\n");
        free_xft(app);
        XFreeGC(app->display, app->gc);
        XDestroyWindow(app->display, app->window);
        XCloseDisplay(app->display);
        return 1;
    }
    XMapWindow(app->display, app->window);
    app->running = 1;
    measure_content(app);

    while (app->running) {
        XEvent event;
        XNextEvent(app->display, &event);
        switch (event.type) {
        case Expose:
            if (event.xexpose.count == 0) draw_page(app);
            break;
        case ConfigureNotify:
            if (event.xconfigure.width != app->width ||
                event.xconfigure.height != app->height) {
                app->width = event.xconfigure.width;
                app->height = event.xconfigure.height;
                measure_content(app);
                draw_page(app);
            }
            break;
        case ButtonPress:
            if (event.xbutton.button == Button4) scroll_by(app, -48);
            else if (event.xbutton.button == Button5) scroll_by(app, 48);
            else if (event.xbutton.button == Button1)
                on_click(app, event.xbutton.x, event.xbutton.y);
            break;
        case KeyPress: {
            KeySym key = XLookupKeysym(&event.xkey, 0);
            if (key == XK_Escape) app->running = 0;
            else if (key == XK_Page_Down || key == XK_space)
                scroll_by(app, viewport_height(app) * 3 / 4);
            else if (key == XK_Page_Up)
                scroll_by(app, -viewport_height(app) * 3 / 4);
            else if (key == XK_Home) scroll_by(app, -app->content_height);
            else if (key == XK_End) scroll_by(app, app->content_height);
            else if (key == XK_1) change_tab(app, 0);
            else if (key == XK_2) change_tab(app, 1);
            else if (key == XK_3) change_tab(app, 2);
            else if (key == XK_4) change_tab(app, 3);
            break;
        }
        case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == app->wm_delete)
                app->running = 0;
            break;
        }
    }

    int status = save_complete(app->config_path) == 0 ? 0 : 1;
    free_xft(app);
    XFreeGC(app->display, app->gc);
    XDestroyWindow(app->display, app->window);
    XCloseDisplay(app->display);
    return status;
}

int main(int argc, char **argv) {
    App app;
    memset(&app, 0, sizeof(app));
    int first_run = 0;
    int check_config = 0;
    const char *override = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("%s %s\n", APP_NAME, VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--first-run") == 0) {
            first_run = 1;
            continue;
        }
        if (strcmp(argv[i], "--check-config") == 0) {
            check_config = 1;
            continue;
        }
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            override = argv[++i];
            continue;
        }
        fprintf(stderr, "Usage: %s [--first-run] [--check-config] [--config PATH] [--version]\n",
                argv[0]);
        return 2;
    }
    if (config_path(&app, override) != 0) {
        fprintf(stderr, "GnuChanWelcome: HOME or XDG_CONFIG_HOME is missing, or config path is too long\n");
        return 1;
    }
    int complete = 0;
    if (config_read(app.config_path, &complete, 1) != 0) return 1;
    if (check_config) {
        printf("Welcome config is valid: %s\n", app.config_path);
        return 0;
    }
    if (first_run && complete) return 0;
    return run_window(&app);
}
